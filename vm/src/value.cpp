#include "lox/value.hpp"
#include <fmt/format.h>

namespace lox
{
    void print_value(Value value) {
        fmt::print("{:g}", value);
    }
}