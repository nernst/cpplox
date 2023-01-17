#pragma once

#include "common.hpp"
#include "chunk.hpp"
#include <string>
#include <iosfwd>

namespace lox
{
    class Function;

    Function* compile(std::string const& source, std::ostream& stderr);
}
