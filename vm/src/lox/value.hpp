#include "common.hpp"
#include "object.hpp"
#include <cassert>
#include <type_traits>
#include <variant>
#include <vector>

namespace lox
{
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
        bool is_object_type(ObjectType type) const {
            Object* obj{nullptr};
            if (try_get(obj)) {
                return obj->type() == type;
            }
            return false;
        }

        bool is_string() const { return is_object_type(ObjectType::STRING); }
        bool is_number() const { return is<double>(); }
        bool is_nil() const { return is<nullptr_t>(); }

        bool operator==(Value const& other) const;

        bool operator!=(Value const& other) const
        { return value_ != other.value_; }

        template<typename T>
        bool is() const { return std::holds_alternative<T>(value_); }

        template<typename T>
        T get() const { return std::get<T>(value_); }

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

    private:
        value_t value_;
    };

    void print_value(Value value);

    class ValueArray
    {
    public:
        ValueArray() = default;
        ValueArray(ValueArray const&) = default;
        ValueArray(ValueArray&&) = default;

        ~ValueArray() = default;

        ValueArray& operator=(ValueArray const&) = default;
        ValueArray& operator=(ValueArray&&) = default;

        void write(Value value) { data_.push_back(value); }
        size_t size() const { return data_.size(); }

        Value operator[](size_t index) const
        {
            assert(index < data_.size());
            return data_[index];
        }

    private:
        std::vector<Value> data_;
    };
 
}