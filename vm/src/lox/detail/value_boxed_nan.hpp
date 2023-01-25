#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace lox {

    class Object;
    
    namespace detail {

    class ValueBaseBoxedNan
    {
        static constexpr std::uint64_t sign_bit    = 0x8000000000000000ull;
        static constexpr std::uint64_t qnan        = 0x7ffc000000000000ull;
        static constexpr std::uint64_t tag_nil     = 0x01ull;
        static constexpr std::uint64_t tag_false   = 0x02ull;
        static constexpr std::uint64_t tag_true    = 0x03ull;

        static constexpr std::uint64_t nil_v = qnan | tag_nil;
        static constexpr std::uint64_t false_v = qnan | tag_false;
        static constexpr std::uint64_t true_v = qnan | tag_true;
        static constexpr std::uint64_t object_mask = sign_bit | qnan;

        static constexpr std::uint64_t to_uint64(double num)
        {
            std::uint64_t value;
            std::memcpy(&value, &num, sizeof(num));
            return value;
        }

        static constexpr double from_uint64(std::uint64_t value)
        {
            double num;
            std::memcpy(&num, &value, sizeof(num));
            return num;
        }

    public:
        constexpr ValueBaseBoxedNan()
        : value_{nil_v}
        { }

        constexpr explicit ValueBaseBoxedNan(bool value)
        : value_{value ? true_v : false_v}
        { }

        constexpr explicit ValueBaseBoxedNan(double value)
        : value_{to_uint64(value)}
        { }

        template <typename Integer,
                std::enable_if_t<std::is_integral<Integer>::value, bool> = true
        >
        constexpr explicit ValueBaseBoxedNan(Integer value)
        : value_{to_uint64(static_cast<double>(value))}
        {}

        constexpr explicit ValueBaseBoxedNan(Object* object)
        : value_{reinterpret_cast<std::uint64_t>(object) | object_mask}
        { }

        constexpr ValueBaseBoxedNan(ValueBaseBoxedNan const& copy)
        : value_{copy.value_}
        {}

        constexpr ValueBaseBoxedNan(ValueBaseBoxedNan&& from)
        : value_{std::exchange(from.value_, nil_v)}
        {}

        ~ValueBaseBoxedNan() = default;

        ValueBaseBoxedNan& operator=(ValueBaseBoxedNan const& copy)
        {
            value_ = copy.value_;
            return *this;
        }

        ValueBaseBoxedNan& operator=(ValueBaseBoxedNan&& from)
        {
            value_ = std::exchange(from.value_, nil_v);
            return *this;
        }

        constexpr explicit operator bool() const { return is_true(); }
        constexpr bool is_nil() const { return value_ == nil_v; }
        constexpr bool is_bool() const { return (value_ | 1) == true_v; }
        constexpr bool is_true() const
        {
            if (is_nil())
                return false;
            else if (is_bool())
            {
                return value_ == true_v;
            }
            else
                return true;
        }

        constexpr bool is_false() const { return !is_true(); }
        constexpr bool is_number() const { return (value_ & qnan) != qnan; }
        constexpr bool is_object() const { return (value_ & object_mask) == object_mask; }

        bool is_object_type(ObjectType type) const
        {
            Object* obj;
            if (try_get(obj))
                return detail::obj_type(obj) == type;
            return false;
        }
        bool is_string() const { return is_object_type(ObjectType::STRING); }
        bool is_function() const { return is_object_type(ObjectType::FUNCTION); }

        constexpr Object* as_obj() const
        {
            assert(is_object());
            return reinterpret_cast<Object*>(value_ & ~object_mask);
        }

        constexpr bool as_bool() const
        {
            assert(is_bool());
            return value_ == true_v;
        }

        constexpr double as_num() const
        {
            assert(is_number());
            return from_uint64(value_);
        }

        constexpr nullptr_t as_nil() const
        {
            assert(is_nil());
            return nullptr;
        }

        bool operator==(ValueBaseBoxedNan const& other) const
        {
            // to handle NaN comparisons
            if (is_number() && other.is_number())
                return as_num() == other.as_num();
            return value_ == other.value_;
        }

        bool operator!=(ValueBaseBoxedNan const& other) const
        { return !(*this == other); }

        template<typename T>
        bool is() const
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                return is_bool();
            }
            else if constexpr(std::is_same_v<T, nullptr_t>)
            {
                return is_nil();
            }
            else if constexpr(std::is_same_v<T, double>)
            {
                return is_number();
            }
            else if constexpr(is_obj_ptr_convertible_v<T>)
            {
                if (!is_object())
                    return false;

                if constexpr (std::is_same_v<T, Object*>)
                    return true;
                else
                    return nullptr != dynamic_cast<T>(as_obj());
            }
            else
                static_assert(always_false_v<T>, "unsupported type conversion.");
        }

        template<typename T>
        T get() const 
        {
            if constexpr (is_obj_ptr_convertible_v<T>)
            {
                Object* ptr = as_obj();

                if constexpr (std::is_same_v<T, Object*>)
                    return ptr;
                else
                    return dynamic_cast<T>(ptr);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return as_bool();
            }
            else if constexpr (std::is_same_v<T, nullptr_t>)
            {
                return as_nil();
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return as_num();
            }
            else
            {
                static_assert(always_false_v<T>, "unsupported type conversion.");
            }
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
        auto apply(Visitor&& vis) const -> decltype(vis(nullptr))
        {
            if (is_nil())
                return vis(nullptr);
            else if (is_number())
                return vis(as_num());
            else if (is_bool())
                return vis(as_bool());
            else if (is_object())
                return vis(as_obj());
            else
                unreachable();
        }

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
        std::uint64_t value_;
    };
 
} } // lox::detail