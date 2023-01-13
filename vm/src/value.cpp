#include "lox/value.hpp"
#include "lox/object.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace lox
{
    bool Value::is_object_type(ObjectType type) const
    {
        Object* obj{nullptr};
        if (try_get(obj)) {
            return obj->type() == type;
        }
        return false;
    }

    ValueArray::~ValueArray()
    { }

    std::string Value::type_name() const
    {
        return std::visit([](auto&& v) -> std::string {
            using T = std::decay_t<decltype(v)>;

            if constexpr(std::is_same_v<T, double>)
                return "number";
            else if constexpr(std::is_same_v<T, bool>)
                return "bool";
            else if constexpr(std::is_same_v<T, nullptr_t>)
                return "nil";
            else if constexpr(std::is_same_v<T, Object*>)
            {
                assert(v != nullptr);
                return fmt::format("Object<{}>", v->type_name());
            }
            else
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }, value_);
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
                    case ObjectType::FUNCTION:
                        return v == other_obj;

                    case ObjectType::STRING:
                        return v == other_obj;
                    
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
                switch (v->type())
                {
                    case ObjectType::STRING:
                        stream << dynamic_cast<String&>(*v).view();
                        break;

                    case ObjectType::FUNCTION:
                    {
                        Function& f = dynamic_cast<Function&>(*v);
                        if (f.name() == nullptr)
                        {
                            stream << "<script>";
                            return;
                        }
                        fmt::print(stream, "<fn {}>", f.name()->view());
                        break;
                    }
                    
                    default:
                        fmt::print(stream, "<Object @{}>", static_cast<void*>(v));
                        break;
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