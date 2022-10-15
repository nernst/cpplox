#pragma once

#include "chunk.hpp"
#include <string>

namespace lox
{
    Chunk compile(std::string const& source);
}
