#include "lox/debug.hpp"
#include <fmt/format.h>

namespace lox 
{
    namespace
    {
        size_t simple(Chunk const& chunk, std::string_view name, size_t offset)
        {
            ignore(chunk);

            fmt::print("{}\n", name);
            return offset + 1;
        }

        size_t constant(Chunk const& chunk, std::string_view name, size_t offset)
        {
            byte constant = chunk.code()[offset + 1];
            fmt::print("{:<16} {:4} '", name, constant);
            print_value(chunk.constants()[constant]);
            fmt::print("'\n");
            return offset + 2;
        }
    }

    void disassemble(Chunk const& chunk, std::string_view name)
    {
        fmt::print("== {} ==\n", name);

        for (size_t offset = 0; offset < chunk.code().size(); )
        {
            offset = disassemble(chunk, offset);
        }

        fmt::print("== {} - {} constants==\n", name, chunk.constants().size());
        for (size_t offset = 0; offset < chunk.constants().size(); ++offset)
        {
            fmt::print("{:3}: [", offset);
            print_value(chunk.constants()[offset]);
            fmt::print("]\n");
        }
    }

    size_t disassemble(Chunk const& chunk, size_t offset)
    {
        fmt::print("{:04} ", offset);

        if (offset && chunk.lines()[offset] == chunk.lines()[offset - 1])
            fmt::print("   | ");
        else
            fmt::print("{:4} " , chunk.lines()[offset]);

        byte instruction{chunk.code()[offset]};

        switch (static_cast<OpCode>(instruction))
        {
            case OpCode::OP_CONSTANT:
                return constant(chunk, "OP_CONSTANT", offset);

            case OpCode::OP_NIL:
                return simple(chunk, "OP_NIL", offset);
            
            case OpCode::OP_TRUE:
                return simple(chunk, "OP_TRUE", offset);
            
            case OpCode::OP_FALSE:
                return simple(chunk, "OP_FALSE", offset);

            case OpCode::OP_POP:
                return simple(chunk, "OP_POP", offset);

            case OpCode::OP_GET_GLOBAL:
                return constant(chunk, "OP_GET_GLOBAL", offset);

            case OpCode::OP_DEFINE_GLOBAL:
                return constant(chunk, "OP_DEFINE_GLOBAL", offset);

            case OpCode::OP_SET_GLOBAL:
                return constant(chunk, "OP_SET_GLOBAL", offset);

            case OpCode::OP_EQUAL:
                return simple(chunk, "OP_EQUAL", offset);

            case OpCode::OP_GREATER:
                return simple(chunk, "OP_GREATER", offset);

            case OpCode::OP_LESS:
                return simple(chunk, "OP_LESS", offset);

            case OpCode::OP_ADD:
                return simple(chunk, "OP_ADD", offset);

            case OpCode::OP_SUBTRACT:
                return simple(chunk, "OP_SUBTRACT", offset);

            case OpCode::OP_MULTIPLY:
                return simple(chunk, "OP_MULTIPLE", offset);

            case OpCode::OP_DIVIDE:
                return simple(chunk, "OP_DIVIDE", offset);

            case OpCode::OP_NOT:
                return simple(chunk, "OP_NOT", offset);

            case OpCode::OP_NEGATE:
                return simple(chunk, "OP_NEGATE", offset);

            case OpCode::OP_PRINT:
                return simple(chunk, "OP_PRINT", offset);

            case OpCode::OP_RETURN:
                return simple(chunk, "OP_RETURN", offset);
            
            default:
                fmt::print("Unknown op code: {}\n", instruction);
                return offset + 1;
        }
    }
}