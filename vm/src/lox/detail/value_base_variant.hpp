#pragma once

#include "../common.hpp"
#include <cassert>
#include <type_traits>
#include <variant>
#include <vector>
#include <iosfwd>

namespace lox { namespace detail {

    class ValueBaseVariant
    {
        using value_t = std::variant<
            std::nullptr_t,
            bool,
            double,
            Object*
        >;

    public:
        ValueBaseVariant()
        : value_{nullptr}
        { }

        explicit ValueBaseVariant(bool value)
        : value_{value}
        { }

        explicit ValueBaseVariant(double value)
        : value_{value}
        { }

        template <typename Integer,
                std::enable_if_t<std::is_integral<Integer>::value, bool> = true
        >
        explicit ValueBaseVariant(Integer value)
        : value_{static_cast<double>(value)}
        {}

        explicit ValueBaseVariant(Object* object)
        : value_{object}
        { }

        ValueBaseVariant(ValueBaseVariant const& copy)
        : value_{copy.value_}
        {}

        ValueBaseVariant(ValueBaseVariant&& from)
        : value_{std::exchange(from.value_, value_t{})}
        {}

        ~ValueBaseVariant() = default;

        ValueBaseVariant& operator=(ValueBaseVariant const& copy)
        {
            value_ = copy.value_;
            return *this;
        }

        ValueBaseVariant& operator=(ValueBaseVariant&& from)
        {
            value_ = std::exchange(from.value_, value_t{});
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

        bool is_nil() const { return is<nullptr_t>(); }
        bool is_bool() const { return is<bool>(); }
        bool is_true() const { return bool{*this}; }
        bool is_false() const { return !is_true(); }
        bool is_number() const { return is<double>(); }
        bool is_object() const { return is<Object*>(); }
        bool is_object_type(ObjectType type) const
        {
            Object* obj;
            if (try_get(obj))
                return detail::obj_type(obj) == type;
            return false;
        }

        bool is_string() const { return is_object_type(ObjectType::STRING); }
        bool is_function() const { return is_object_type(ObjectType::FUNCTION); }

        Object* as_obj() const
        {
            if (std::holds_alternative<Object*>(value_))
                return std::get<Object*>(value_);
            return nullptr;
        }

        bool operator==(ValueBaseVariant const& other) const
        { return value_ == other.value_; }

        bool operator!=(ValueBaseVariant const& other) const
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

                if constexpr (std::is_same_v<T, Object*>)
                    return ptr;
                else
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

        std::string type_name() const
        {
            return apply([](auto&& v) -> std::string {
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
                    return fmt::format("Object<{}>", detail::obj_type_name(v));
                }
                else
                    static_assert(always_false_v<T>, "non-exhaustive visitor!");
            });
        }

    private:
        value_t value_;
    };
 
} } // lox::detail
