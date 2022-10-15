#include "lox/chunk.hpp"
#include <fmt/format.h>

using namespace lox;


void Chunk::disassemble(std::string_view name) const
{
    fmt::print("== {} ==\n", name);

    for (size_t offset = 0; offset < data_.size(); )
    {
        offset = disassemble(offset);
    }
}

size_t Chunk::disassemble(size_t offset) const
{
    fmt::print("{:04} ", offset);

    byte instruction{data_[offset]};

    switch (static_cast<OpCode>(instruction))
    {
        case OpCode::OP_RETURN:
            return simple("OP_RETURN", offset);
        
        default:
            fmt::print("Unknown op code: {}\n", instruction);
            return offset + 1;
    }
}

size_t Chunk::simple(std::string_view name, size_t offset) const
{
    fmt::print("{}\n", name);
    return offset + 1;
}

