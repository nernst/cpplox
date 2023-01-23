#include "lox/compiler.hpp"
#include "lox/vm.hpp"
#include "lox/types/objclass.hpp"
#include "lox/types/objinstance.hpp"
#include "lox/types/objboundmethod.hpp"
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

    void VM::do_mark_objects(GC& gc)
    {
        #ifdef DEBUG_LOG_GC
        fmt::print(std::cerr, "VM::do_mark_objects {} begin\n", static_cast<void*>(this));
        #endif
        gc.mark_object(init_str_);

        for (auto slot = stack_.data(); slot != stack_top_; ++slot)
        {
            gc.mark_value(*slot);
        }

        for (size_t i = 0; i < frame_count_; ++i)
        {
            gc.mark_object(frames_[i].closure_);
        }

        for (auto upvalue = open_upvalues_; upvalue != nullptr; upvalue = upvalue->next_)
        {
            gc.mark_object(upvalue);
        }

        #ifdef DEBUG_LOG_GC
        fmt::print(std::cerr, "VM::do_mark_objects {} end\n", static_cast<void*>(this));
        #endif
    }

    void VM::init_globals()
    {
        init_str_ = String::create("init");
        define_native("clock", &clock_native);
    }

    VM::Result VM::interpret(std::string const& source)
    {
        auto function = compile(source, stderr());
        if (!function)
            return Result::COMPILE_ERROR;

        gcroot<Closure> closure{new Closure(function.get())};
        push(Value{closure.get()});
        call(closure.get(), 0);
        return run();
    }

    bool VM::call_value(Value callee, byte arg_count)
    {
        Object* obj{nullptr};
        if (callee.try_get(obj))
        {
            switch(obj->type())
            {
                case ObjectType::NATIVE_FUNCTION:
                {
                    auto native = dynamic_cast<NativeFunction*>(obj);
                    assert(native);
                    auto result = native->invoke(arg_count, stack_top_ - arg_count);
                    stack_top_ -= arg_count + 1;
                    push(result);
                    return true;
                }

                case ObjectType::CLOSURE:
                    return call(dynamic_cast<Closure*>(obj), arg_count);

                case ObjectType::OBJCLASS:
                {
                    ObjClass* klass = dynamic_cast<ObjClass*>(obj);
                    assert(klass);

                    stack_top_[-arg_count - 1] = Value{new ObjInstance{klass}};

                    Value initializer;
                    if (klass->methods().get(init_str_, initializer))
                    {
                        return call(initializer.get<Closure*>(), arg_count);
                    }
                    else if (arg_count != 0)
                    {
                        runtime_error("Expected 0 arguments but got {}.", arg_count);
                        return false;
                    }
                    return true;
                }

                case ObjectType::OBJBOUNDMETHOD:
                {
                    auto bound = dynamic_cast<ObjBoundMethod*>(obj);
                    assert(bound);
                    stack_top_[-arg_count - 1] = bound->receiver();
                    return call(bound->method(), arg_count);
                }
                
                default:
                    break; // non-callable
            }
        }

        runtime_error("Can only call functions and classes. type: {}", callee.type_name());
        return false;
    }

    ObjUpvalue* VM::capture_upvalue(Value* local)
    {
        assert(local);

        ObjUpvalue* prev = nullptr;
        ObjUpvalue* upvalue = open_upvalues_;

        while (upvalue != nullptr && upvalue->location() > local)
        {
            prev = upvalue;
            upvalue = upvalue->next_;
        }
        
        if (upvalue != nullptr && upvalue->location() == local)
            return upvalue;

        auto new_upvalue = new ObjUpvalue{local};
        new_upvalue->next_ = upvalue;

        if (prev == nullptr)
            open_upvalues_ = new_upvalue;
        else
            prev->next_ = new_upvalue;

        return new_upvalue;
    }

    void VM::close_upvalues(Value* last)
    {
        while (open_upvalues_ != nullptr && open_upvalues_->location() >= last)
        {
            auto upvalue = open_upvalues_;
            upvalue->close();
            open_upvalues_ = upvalue->next_;
        }
    }

    bool VM::call(Closure* closure, byte arg_count)
    {
        assert(closure);

        if (arg_count != closure->function()->arity())
        {
            runtime_error("Expected {} arguments but got {}.", closure->function()->arity(), arg_count);
            return false;
        }
        if (frame_count_ == frames_max)
        {
            runtime_error("Stack overflow.");
            return false;
        }

        push_frame(closure, arg_count);
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

                        if (lhs.is_string() && rhs.is_string())
                        {

                            String* s_lhs = dynamic_cast<String*>(lhs.get<Object*>()); 
                            String* s_rhs = dynamic_cast<String*>(rhs.get<Object*>());
                            assert(s_lhs && "Expected String*!");
                            assert(s_rhs && "Expected String*!");

                            push(Value(String::create(s_lhs->str() + s_rhs->str())));
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

                case OpCode::OP_GET_UPVALUE:
                {
                    byte slot = read_byte();
                    push(*frame->closure()->upvalues()[slot]->location());
                    break;
                }

                case OpCode::OP_SET_UPVALUE:
                {
                    byte slot = read_byte();
                    *frame->closure()->upvalues()[slot]->location() = peek(0);
                    break;
                }

                case OpCode::OP_GET_PROPERTY:
                {
                    ObjInstance* instance = nullptr;
                    if (!peek(0).try_get(instance))
                    {
                        runtime_error("Only instances have properties.");
                        return Result::RUNTIME_ERROR;
                    }
                    auto name = read_string();

                    Value value;
                    if (instance->fields().get(name, value))
                    {
                        pop(); // instance
                        push(value);
                        break;
                    }

                    if (!bind_method(instance->obj_class(), name))
                    {
                        return Result::RUNTIME_ERROR;
                    }

                    break;
                }
            
                case OpCode::OP_SET_PROPERTY:
                {
                    ObjInstance* instance = nullptr;
                    if (!peek(1).try_get(instance))
                    {
                        runtime_error("Only instances have fields, not {}.", peek(1).type_name());
                        return Result::RUNTIME_ERROR;
                    }
                    instance->fields().add(read_string(), peek(0));
                    Value value{pop()}; // value
                    pop(); // instance
                    push(value);
                    break;
                }

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

                case OpCode::OP_CLOSURE:
                {
                    auto value = read_constant();
                    Function* function = value.get<Function*>();
                    Closure* closure{new Closure(function)};
                    push(Value{closure});
                    for (unsigned i = 0; i < closure->upvalue_count(); ++i)
                    {
                        byte is_local = read_byte();
                        byte index = read_byte();
                        closure->upvalues()[i] = is_local
                            ? capture_upvalue(frame->slots() + index)
                            : frame->closure()->upvalues()[index];
                    }
                    break;
                }

                case OpCode::OP_CLOSE_UPVALUE:
                    close_upvalues(stack_top_ - 1);
                    pop();
                    break;

                case OpCode::OP_RETURN:
                {
                    auto result = pop();
                    close_upvalues(frame->slots());
                    frame = pop_frame();
                    if (frame == nullptr)
                    {
                        pop();
                        return Result::OK;
                    }
                    push(result);
                    break;
                }

                case OpCode::OP_CLASS:
                {
                    push(Value{new ObjClass{read_string()}});
                    break;
                }

                case OpCode::OP_METHOD:
                    define_method(read_string());
                    break;
            }
        }
    }

    void VM::define_native(std::string_view name, NativeFunction::NativeFn function)
    {
        push(Value{String::create(name)});
        push(Value{new NativeFunction{function}});
        auto&& s = static_cast<String*>(peek(1).get<Object*>());
        globals_.add(s, peek(0));
        pop();
        pop();
    }

    void VM::define_method(String* name)
    {
        auto method = peek(0);
        peek(1).get<ObjClass*>()->methods().add(name, method);
        pop(); // method
    }

    bool VM::bind_method(ObjClass* klass, String* name)
    {
        Value method;
        if (!klass->methods().get(name, method))
        {
            runtime_error("Undefined property '%s'.", name->view());
            return false;
        }

        auto bound{new ObjBoundMethod(peek(0), method.get<Closure*>())};
        pop();
        push(Value{bound});
        return true;
    }
}
