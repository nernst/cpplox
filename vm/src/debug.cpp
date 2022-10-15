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

        size_t disassemble(Chunk const& chunk, size_t offset)
        {
            fmt::print("{:04} ", offset);

            if (offset && chunk.lines()[offset] != chunk.lines()[offset - 1])
                fmt::print("   | ");
            else
                fmt::print("{:4} " , chunk.lines()[offset]);

            byte instruction{chunk.code()[offset]};

            switch (static_cast<OpCode>(instruction))
            {
                case OpCode::OP_CONSTANT:
                    return constant(chunk, "OP_CONSTANT", offset);

                case OpCode::OP_RETURN:
                    return simple(chunk, "OP_RETURN", offset);
                
                default:
                    fmt::print("Unknown op code: {}\n", instruction);
                    return offset + 1;
            }
        }
    }

    void disassemble(Chunk const& chunk, std::string_view name)
    {
        fmt::print("== {} ==\n", name);

        for (size_t offset = 0; offset < chunk.code().size(); )
        {
            offset = disassemble(chunk, offset);
        }
    }
}