#include "lox/compiler.hpp"
#include "lox/vm.hpp"
#ifdef DEBUG_TRACE_EXECUTION
#include "lox/debug.hpp"
#endif
#include <fmt/format.h>

namespace lox
{
    VM::Result VM::interpret(std::string const& source)
    {
        Chunk chunk;
        if (!compile(source, chunk))
            return Result::COMPILE_ERROR;

        chunk_ = std::move(chunk);
        return interpret();
    }

    VM::Result VM::interpret()
    {
        ip_start_ = ip_ = chunk_.code().data();
        ip_end_ = ip_ + chunk_.code().size();
        return run();
    }

    VM::Result VM::run()
    {
        #define BINARY_OP(op) do { \
            if (!peek(0).is<double>() || !peek(1).is<double>()) { \
                runtime_error("Operands must be numbers."); \
                return Result::RUNTIME_ERROR; \
            } \
            Value a = pop(); \
            Value b = pop(); \
            push(Value(a.get<double>() op b.get<double>())); \
        } while (false)

        while (true)
        {
#ifdef DEBUG_TRACE_EXECUTION
            fmt::print("PC: {}\n", pc());
            fmt::print("Stack: ");
            for (auto const& value : stack_)
            {
                fmt::print("[ ");
                print_value(value);
                fmt::print(" ]");
            }
            fmt::print("\n");
            disassemble(chunk_, static_cast<size_t>(ip_ - chunk_.code().data()));
#endif
            auto instruction = read_instruction();
            switch (instruction)
            {
                case OpCode::OP_CONSTANT:
                    {
                        auto value = read_constant();
                        push(value);
                    }
                    break;

                case OpCode::OP_NIL: push(Value()); break;
                case OpCode::OP_TRUE: push(Value(true)); break;
                case OpCode::OP_FALSE: push(Value(false)); break;

                case OpCode::OP_ADD:
                    {
                        if (peek(0).is_string() && peek(1).is_string())
                        {
                            Value v_lhs{pop()};
                            Value v_rhs{pop()};
                            String* lhs = dynamic_cast<String*>(v_lhs.get<Object*>()); 
                            String* rhs = dynamic_cast<String*>(v_rhs.get<Object*>());
                            assert(lhs && "Expected String*!");
                            assert(rhs && "Expected String*!");

                            push(Value(allocate<String>(lhs->str() + rhs->str())));
                        }
                        else if (peek(0).is_number() && peek(1).is_number())
                        {
                            Value lhs{pop()};
                            Value rhs{pop()};
                            push(Value{lhs.get<double>() + rhs.get<double>()});
                        }
                        else
                        {
                            runtime_error("Operands must be two numbers or two strings.");
                            return Result::RUNTIME_ERROR;
                        }

                    }
                    break;

                case OpCode::OP_SUBTRACT: BINARY_OP(-); break;
                case OpCode::OP_MULTIPLY: BINARY_OP(*); break;
                case OpCode::OP_DIVIDE: BINARY_OP(/); break;

                case OpCode::OP_NOT: push(Value(!bool{pop()})); break;
                case OpCode::OP_EQUAL: push(Value(pop() == pop())); break;
                case OpCode::OP_LESS: BINARY_OP(<); break;
                case OpCode::OP_GREATER: BINARY_OP(>); break;

                case OpCode::OP_NEGATE:
                    if (!peek(0).is<double>()) {
                        runtime_error("Operand must be a number.");
                        return Result::RUNTIME_ERROR;
                    }
                    push(Value(-pop().get<double>()));
                    break;

                case OpCode::OP_RETURN:
                    print_value(pop());
                    fmt::print("\n");
                    return Result::OK;
            }
        }
        #undef BINARY_OP
    }
}