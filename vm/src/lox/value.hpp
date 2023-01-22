#pragma once

#include "common.hpp"
#include <cassert>
#include <type_traits>
#include <variant>
#include <vector>
#include <iosfwd>

namespace lox
{
    class Object;

    template<typename T>
    inline constexpr bool is_obj_ptr_convertible_v = 
        std::is_pointer_v<T>
        && std::is_convertible_v<T, Object*>;

    class Value
    {
        using value_t = std::variant<
            std::nullptr_t,
            bool,
            double,
            Object*
        >;

    public:
        Value()
        : value_{nullptr}
        { }

        explicit Value(bool value)
        : value_{value}
        { }

        explicit Value(double value)
        : value_{value}
        { }

        template <typename Integer,
                std::enable_if_t<std::is_integral<Integer>::value, bool> = true
        >
        explicit Value(Integer value)
        : value_{static_cast<double>(value)}
        {}

        explicit Value(Object* object)
        : value_{object}
        { }

        Value(Value const& copy)
        : value_{copy.value_}
        {}

        Value(Value&& from)
        : value_{std::move(from.value_)}
        {}

        ~Value() = default;

        Value& operator=(Value const& copy)
        {
            if (this != &copy)
                value_ = copy.value_;
            return *this;
        }
        Value& operator=(Value&& from)
        {
            if (this != &from)
                value_ = std::move(from.value_);
            return *this;
        }

        explicit operator bool() const {
            return apply([](auto&& value) -> bool {
                using T = std::decay_t<decltype(value)>;
                if constexpr(std::is_same_v<T, nullptr_t>)
                    return false;
                else if constexpr(std::is_same_v<T, bool>)
                    return value;
                else
                    return true;
            });

        }

        bool is_true() const { return bool{*this}; }
        bool is_false() const { return !is_true(); }
        bool is_object() const { return is<Object*>(); }
        bool is_object_type(ObjectType type) const;

        bool is_string() const { return is_object_type(ObjectType::STRING); }
        bool is_function() const { return is_object_type(ObjectType::FUNCTION); }
        bool is_number() const { return is<double>(); }
        bool is_nil() const { return is<nullptr_t>(); }

        template<typename T>
        T* as_obj() const
        {
            Object* obj{nullptr};
            if (try_get(obj)) {
                return dynamic_cast<T*>(obj);
            }
            return nullptr;
        }

        bool operator==(Value const& other) const;

        bool operator!=(Value const& other) const
        { return value_ != other.value_; }

        template<typename T>
        bool is() const
        {
            if constexpr(is_obj_ptr_convertible_v<T>)
            {
                if (!std::holds_alternative<Object*>(value_))
                    return false;

                if constexpr(std::is_same_v<T, Object*>)
                    return true;
                else
                {
                    auto ptr = std::get<Object*>(value_);
                    return nullptr != dynamic_cast<T>(ptr);
                }
            }
            else
                return std::holds_alternative<T>(value_);
        }

        template<typename T>
        T get() const 
        {
            if constexpr (is_obj_ptr_convertible_v<T>)
            {
                Object* ptr = std::get<Object*>(value_);
                return dynamic_cast<T>(ptr);
            }
            else
                return std::get<T>(value_);
        }

        template<typename T>
        bool try_get(T& out) const {
            if (is<T>()) {
                out = get<T>();
                return true;
            }
            return false;
        }

        template<class Visitor>
        auto apply(Visitor&& vis) const -> decltype(std::visit(vis, value_t{}))
        { return std::visit(vis, value_); }

        std::string type_name() const;

    private:
        value_t value_;
    };

    void print_value(Value const& value);
    void print_value(std::ostream& stream, Value const& value);
 
}