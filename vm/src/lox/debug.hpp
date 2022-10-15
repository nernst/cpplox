#pragma once

#include "chunk.hpp"
#include <string_view>

namespace lox {
    void disassemble(Chunk const& chunk, std::string_view name);
}
