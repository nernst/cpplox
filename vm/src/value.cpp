#include "lox/value.hpp"
#include "lox/object.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace lox
{

    namespace detail {
        std::string_view obj_type_name(Object const* obj)
        { return obj->type_name(); }

        ObjectType obj_type(Object const* obj)
        { return obj->type(); }
    }

    void print_value(std::ostream& stream, Value const& value)
    {
        value.apply([&stream](auto&& v) -> void
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr(std::is_same_v<T, double>)
                fmt::print(stream, "{:g}", v);
            else if constexpr(std::is_same_v<T, nullptr_t>)
                stream << "nil";
            else if constexpr(std::is_same_v<T, bool>)
            {
                if (v)
                    stream << "true";
                else
                    stream << "false";
            }
            else if constexpr(std::is_same_v<T, Object*>)
            {
                print_object(stream, *v);
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
        });
    }

    void print_value(Value const& value)
    {
        print_value(std::cout, value);
    }
}