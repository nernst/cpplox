#include "lox/value.hpp"
#include "lox/object.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace lox
{
    ValueArray::~ValueArray()
    {
        for (auto& value : data_)
        {
            Object* obj;
            if (value.try_get(obj))
                delete obj;
        }
    }
  
    bool Value::operator==(Value const& other) const
    {
        if (value_.index() != other.value_.index())
            return false;

        return std::visit([&other](auto&& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr(std::is_same_v<T, double>)
                return v == std::get<double>(other.value_);
            else if constexpr(std::is_same_v<T, nullptr_t>)
                return true;
            else if constexpr(std::is_same_v<T, bool>)
                return v == std::get<bool>(other.value_);
            else if constexpr(std::is_same_v<T, Object*>)
            {
                Object* other_obj = std::get<Object*>(other.value_);
                if (v->type() != other_obj->type())
                    return false;

                switch(v->type())
                {
                    case ObjectType::STRING:
                        return v == other_obj;
                        //return static_cast<String const*>(v)->str() == static_cast<String const*>(other_obj)->str();
                    default:
                        unreachable();
                        return false;
                }
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");

        }, value_);
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
                String* s = dynamic_cast<String*>(v);
                if (s)
                {
                    stream << s->view();
                }
                else
                {
                    fmt::print(stream, "<Object @{}>", static_cast<void*>(v));
                }
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