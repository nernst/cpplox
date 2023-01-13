#include "lox/debug.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace lox 
{
    namespace
    {
        size_t simple(std::ostream& stream, Chunk const& chunk, std::string_view name, size_t offset)
        {
            ignore(chunk);

            fmt::print(stream, "{}\n", name);
            return offset + 1;
        }

        size_t constant(std::ostream& stream, Chunk const& chunk, std::string_view name, size_t offset)
        {
            byte constant = chunk.code()[offset + 1];
            fmt::print(stream, "{:<16} {:4} '", name, constant);
            print_value(stream, chunk.constants()[constant]);
            fmt::print(stream, "'\n");
            return offset + 2;
        }

        size_t byte_instruction(std::ostream& stream, Chunk const& chunk, std::string_view name, size_t offset)
        {
            byte slot = chunk.code()[offset + 1];
            fmt::print(stream, "{:<16} {:4}\n", name, slot);
            return offset + 2;
        }

        size_t jump(std::ostream& stream, Chunk const& chunk, std::string_view name, int sign, size_t offset)
        {
            uint16_t jump = static_cast<uint16_t>(chunk.code()[offset + 1]) << 8;
            jump |= chunk.code()[offset + 2];
            fmt::print(stream, "{:<16} {:4} -> {}\n", name, offset, offset + 3 + sign * jump);
            return offset + 3;
        }
    }

    void disassemble(std::ostream& stream, Chunk const& chunk, std::string_view name)
    {
        fmt::print(stream, "== {} ==\n", name);

        for (size_t offset = 0; offset < chunk.code().size(); )
        {
            offset = disassemble(stream, chunk, offset);
        }

        fmt::print(stream, "== {} - {} constants==\n", name, chunk.constants().size());
        for (size_t offset = 0; offset < chunk.constants().size(); ++offset)
        {
            fmt::print(stream, "{:3}: [", offset);
            print_value(stream, chunk.constants()[offset]);
            fmt::print(stream, "]\n");
        }
    }

    size_t disassemble(std::ostream& stream, Chunk const& chunk, size_t offset)
    {
        fmt::print(stream, "{:04} ", offset);

        if (offset && chunk.lines()[offset] == chunk.lines()[offset - 1])
            fmt::print(stream, "   | ");
        else
            fmt::print(stream, "{:4} " , chunk.lines()[offset]);

        byte instruction{chunk.code()[offset]};

        switch (static_cast<OpCode>(instruction))
        {
            case OpCode::OP_CONSTANT:
                return constant(stream, chunk, "OP_CONSTANT", offset);

            case OpCode::OP_NIL:
                return simple(stream, chunk, "OP_NIL", offset);
            
            case OpCode::OP_TRUE:
                return simple(stream, chunk, "OP_TRUE", offset);
            
            case OpCode::OP_FALSE:
                return simple(stream, chunk, "OP_FALSE", offset);

            case OpCode::OP_POP:
                return simple(stream, chunk, "OP_POP", offset);

            case OpCode::OP_GET_LOCAL:
                return byte_instruction(stream, chunk, "OP_GET_LOCAL", offset);

            case OpCode::OP_SET_LOCAL:
                return byte_instruction(stream, chunk, "OP_SET_LOCAL", offset);

            case OpCode::OP_GET_GLOBAL:
                return constant(stream, chunk, "OP_GET_GLOBAL", offset);

            case OpCode::OP_DEFINE_GLOBAL:
                return constant(stream, chunk, "OP_DEFINE_GLOBAL", offset);

            case OpCode::OP_SET_GLOBAL:
                return constant(stream, chunk, "OP_SET_GLOBAL", offset);

            case OpCode::OP_EQUAL:
                return simple(stream, chunk, "OP_EQUAL", offset);

            case OpCode::OP_GREATER:
                return simple(stream, chunk, "OP_GREATER", offset);

            case OpCode::OP_LESS:
                return simple(stream, chunk, "OP_LESS", offset);

            case OpCode::OP_ADD:
                return simple(stream, chunk, "OP_ADD", offset);

            case OpCode::OP_SUBTRACT:
                return simple(stream, chunk, "OP_SUBTRACT", offset);

            case OpCode::OP_MULTIPLY:
                return simple(stream, chunk, "OP_MULTIPLE", offset);

            case OpCode::OP_DIVIDE:
                return simple(stream, chunk, "OP_DIVIDE", offset);

            case OpCode::OP_NOT:
                return simple(stream, chunk, "OP_NOT", offset);

            case OpCode::OP_NEGATE:
                return simple(stream, chunk, "OP_NEGATE", offset);

            case OpCode::OP_PRINT:
                return simple(stream, chunk, "OP_PRINT", offset);

            case OpCode::OP_JUMP:
                return jump(stream, chunk, "OP_JUMP", 1, offset);

            case OpCode::OP_JUMP_IF_FALSE:
                return jump(stream, chunk, "OP_JUMP_IF_FALSE", 1, offset);

            case OpCode::OP_LOOP:
                return jump(stream, chunk, "OP_LOOP", -1, offset);

            case OpCode::OP_CALL:
                return byte_instruction(stream, chunk, "OP_CALL", offset);

            case OpCode::OP_RETURN:
                return simple(stream, chunk, "OP_RETURN", offset);
            
            default:
                fmt::print(stream, "Unknown op code: {}\n", instruction);
                return offset + 1;
        }
    }
}