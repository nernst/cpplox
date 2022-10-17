#pragma once

#include "common.hpp"
#include "chunk.hpp"
#include <string>

namespace lox
{
    bool compile(std::string const& source, Chunk& chunk);
}
