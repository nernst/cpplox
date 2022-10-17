#include "lox/value.hpp"
#include "lox/object.hpp"
#include <fmt/format.h>

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
                        return static_cast<String const*>(v)->str() == static_cast<String const*>(other_obj)->str();
                    default:
                        unreachable();
                        return false;
                }
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");

        }, value_);
    }

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
            else if constexpr(std::is_same_v<T, Object*>)
            {
                fmt::print("<Object @{}>", static_cast<void*>(v));
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
        });
    }
}