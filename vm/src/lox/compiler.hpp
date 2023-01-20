#pragma once

#include "common.hpp"
#include "chunk.hpp"
#include "gc.hpp"
#include <string>
#include <iosfwd>

namespace lox
{
    class Function;

    gcroot<Function> compile(std::string const& source, std::ostream& stderr);
}
