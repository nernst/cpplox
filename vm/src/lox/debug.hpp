#pragma once

#include "chunk.hpp"
#include <string_view>

namespace lox {
    void disassemble(Chunk const& chunk, std::string_view name);
    size_t disassemble(Chunk const& chunk, size_t offset);
}
