#include "lox/value.hpp"
#include <fmt/format.h>

namespace lox
{
    void print_value(Value value) {
        value.apply([](auto&& v) -> void {
            using T = std::decay_t<decltype(v)>;

            if constexpr(std::is_same_v<T, double>)
                fmt::print("{:g}", v);
            else if constexpr(std::is_same_v<T, nullptr_t>)
                fmt::print("nil");
            else if constexpr(std::is_same_v<T, bool>)
            {
                if (v)
                    fmt::print("true");
                else
                    fmt::print("false");
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
        });
    }
}