#pragma once

#include "common.hpp"
#include "chunk.hpp"
#include <string>
#include <iosfwd>

namespace lox
{
    bool compile(std::string const& source, Chunk& chunk, std::ostream& stderr);
}
