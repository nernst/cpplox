#include "lox/vm.hpp"
#ifdef DEBUG_TRACE_EXECUTION
#include "lox/debug.hpp"
#endif
#include <fmt/format.h>

namespace lox
{
    VM::Result VM::interpret()
    {
        ip_start_ = ip_ = chunk_.code().data();
        ip_end_ = ip_ + chunk_.code().size();
        return run();
    }

    VM::Result VM::run()
    {
        while (true)
        {
#ifdef DEBUG_TRACE_EXECUTION
            fmt::print("PC: {}\n", pc());
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

                case OpCode::OP_RETURN:
                    print_value(pop());
                    fmt::print("\n");
                    return Result::OK;
            }
        }
    }
}
