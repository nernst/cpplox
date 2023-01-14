#include "lox/compiler.hpp"
#include "lox/vm.hpp"
#include <fmt/format.h>
#include <ctime>
#include <functional>

#ifdef DEBUG_TRACE_EXECUTION
#include "lox/debug.hpp"
#endif

namespace lox
{
    namespace {
        Value clock_native(int, Value*)
        {
            return Value{static_cast<double>(std::clock()) / CLOCKS_PER_SEC};
        }

    }

    void VM::init_globals()
    {
        define_native("clock", &clock_native);
    }

    VM::Result VM::interpret(std::string const& source)
    {
        Function* function = compile(source, stderr());
        if (function == nullptr)
            return Result::COMPILE_ERROR;
        
        push(Value{function});
        call(function, 0);
        return run();
    }

    bool VM::call_value(Value callee, byte arg_count)
    {
        Object* obj{nullptr};
        if (callee.try_get(obj))
        {
            switch(obj->type())
            {
                case ObjectType::FUNCTION:
                    return call(dynamic_cast<Function*>(obj), arg_count);

                case ObjectType::NATIVE_FUNCTION:
                {
                    auto native = dynamic_cast<NativeFunction*>(obj);
                    auto result = native->invoke(arg_count, stack_top_ - arg_count);
                    stack_top_ -= arg_count + 1;
                    push(result);
                    return true;
                }
                
                default:
                    break; // non-callable
            }
        }

        runtime_error("Can only call functions and classes. type: {}", callee.type_name());
        return false;
    }

    bool VM::call(Function* function, byte arg_count)
    {
        assert(function);

        if (arg_count != function->arity())
        {
            runtime_error("Expected {} arguments but got {}.", function->arity(), arg_count);
            return false;
        }
        if (frame_count_ == frames_max)
        {
            runtime_error("Stack overflow.");
            return false;
        }

        push_frame(function, arg_count);
        return true;
    }

    VM::Result VM::run()
    {
        auto frame = current_frame();

        while (true)
        {
#ifdef DEBUG_TRACE_EXECUTION
            fmt::print(stderr(), "PC: {}\n", frame->pc());

            stderr() << "Globals: " << globals_ << std::endl;

            stderr() << "Stack: { ";
            for (auto* p = stack_.data(); p != stack_top_; ++p)
            {
                stderr() << "[";
                print_value(stderr(), *p);
                stderr() << "], ";
            }
            stderr() << "}\n";

            stderr() << "Window: { ";
            for (auto* p = frame->slots(); p != stack_top_; ++p)
            {
                stderr() << "[";
                print_value(stderr(), *p);
                stderr() << "], ";
            }
            stderr() << "}\n";

            disassemble(
                stderr(), 
                frame->chunk(), 
                static_cast<size_t>(frame->ip() - frame->chunk().code().data()));
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
                case OpCode::OP_POP: pop(); break;

                case OpCode::OP_GET_LOCAL:
                {
                    byte slot = read_byte();
                    push(frame->slots()[slot]);
                    break;
                }

                case OpCode::OP_SET_LOCAL:
                {
                    byte slot = read_byte();
                    frame->slots()[slot] = peek(0);
                    break;
                }

                case OpCode::OP_GET_GLOBAL:
                {
                    auto name = read_string();
                    Value value;
                    if (!globals_.get(name, value))
                    {
                        runtime_error("Undefined variable '{}'.", name->view());
                        return Result::RUNTIME_ERROR;
                    }
                    push(value);
                    break;
                }

                case OpCode::OP_DEFINE_GLOBAL:
                {
                    auto name = read_string();
                    globals_.add(name, peek(0));
                    pop();
                    break;
                }

                case OpCode::OP_SET_GLOBAL:
                {
                    auto name = read_string();
                    if (globals_.add(name, peek(0)))
                    {
                        globals_.remove(name);
                        runtime_error("Undefined variable '{}'.", name->view());
                        return Result::RUNTIME_ERROR;
                    }
                    break;
                }

                case OpCode::OP_ADD:
                    {
                        // ORDER MATTERS!
                        Value rhs{pop()};
                        Value lhs{pop()};

                        #ifdef DEBUG_TRACE_EXECUTION
                        // fmt::print(stderr(), "** OP_ADD - begin\n");
                        #endif

                        if (lhs.is_string() && rhs.is_string())
                        {

                            String* s_lhs = dynamic_cast<String*>(lhs.get<Object*>()); 
                            String* s_rhs = dynamic_cast<String*>(rhs.get<Object*>());
                            assert(s_lhs && "Expected String*!");
                            assert(s_rhs && "Expected String*!");

                            #ifdef DEBUG_TRACE_EXECUTION
                            // fmt::print(stderr(), "** OP_ADD, lhs=[{}], rhs=[{}]\n", lhs->str(), rhs->str());
                            #endif

                            push(Value(allocate<String>(s_lhs->str() + s_rhs->str())));
                        }
                        else if (lhs.is_number() && rhs.is_number())
                        {
                            push(Value{lhs.get<double>() + rhs.get<double>()});
                        }
                        else
                        {
                            runtime_error(
                                "Operands must be two numbers or two strings, not {} and {}.", 
                                lhs.type_name(), 
                                rhs.type_name()
                            );
                            return Result::RUNTIME_ERROR;
                        }

                        #ifdef DEBUG_TRACE_EXECUTION
                        // fmt::print(stderr(), "** OP_ADD end\n");
                        #endif

                    }
                    break;

                case OpCode::OP_SUBTRACT:
                    if (!numeric_binary_op(std::minus<>()))
                        return Result::RUNTIME_ERROR;
                    break;

                case OpCode::OP_MULTIPLY:
                    if (!numeric_binary_op(std::multiplies<>()))
                        return Result::RUNTIME_ERROR;
                    break;

                case OpCode::OP_DIVIDE:
                    if (!numeric_binary_op(std::divides<>()))
                        return Result::RUNTIME_ERROR;
                    break;


                case OpCode::OP_NOT:
                    push(Value(!bool{pop()}));
                    break;

                case OpCode::OP_EQUAL:
                    push(Value(pop() == pop()));
                    break;

                case OpCode::OP_LESS:
                    if (!numeric_binary_op(std::less<>()))
                        return Result::RUNTIME_ERROR;
                    break;

                case OpCode::OP_GREATER:
                    if (!numeric_binary_op(std::greater<>()))
                        return Result::RUNTIME_ERROR;
                    break;

                case OpCode::OP_NEGATE:
                    if (!peek(0).is<double>()) {
                        runtime_error("Operand must be a number.");
                        return Result::RUNTIME_ERROR;
                    }
                    push(Value(-pop().get<double>()));
                    break;

                case OpCode::OP_PRINT:
                    print_value(stdout(), pop());
                    stdout() << "\n";
                    break;

                case OpCode::OP_JUMP:
                {
                    auto offset = read_short();
                    frame->ip() += offset;
                    break;
                }

                case OpCode::OP_JUMP_IF_FALSE:
                {
                    auto offset = read_short();
                    if (peek(0).is_false())
                        frame->ip() += offset;
                    break;
                }

                case OpCode::OP_LOOP:
                {
                    auto offset = read_short();
                    frame->ip() -= offset;
                    break;
                }

                case OpCode::OP_CALL:
                {
                    byte arg_count = read_byte();
                    if (!call_value(peek(arg_count), arg_count))
                        return Result::RUNTIME_ERROR;

                    frame = current_frame();
                    break;
                }


                case OpCode::OP_RETURN:
                {
                    auto result = pop();
                    frame = pop_frame();
                    if (frame == nullptr)
                    {
                        pop();
                        return Result::OK;
                    }
                    push(result);
                    break;
                }
            }
        }
    }

    void VM::define_native(std::string_view name, NativeFunction::NativeFn function)
    {
        push(Value{new String{name}});
        push(Value{new NativeFunction{function}});
        auto&& s = static_cast<String*>(peek(1).get<Object*>());
        globals_.add(s, peek(0));
        pop();
        pop();
    }
}
