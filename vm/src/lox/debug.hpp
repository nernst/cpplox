#pragma once

#include "chunk.hpp"
#include <string_view>
#include <iosfwd>

namespace lox {
    void disassemble(std::ostream& stream, Chunk const& chunk, std::string_view name);
    size_t disassemble(std::ostream& stream, Chunk const& chunk, size_t offset);
}
