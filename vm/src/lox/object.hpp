#pragma once

#include "common.hpp"

namespace lox
{
    enum class ObjectType
    {
        STRING,
    };

    class Object;
    class VM;

    void print_object(Object const& object);

    class Object
    {
        friend class VM;

    public:
        Object()
        : next_{nullptr}
        { }

        Object(Object const&) = delete;
        Object(Object&&) = delete;

        virtual ~Object() = 0;

        Object& operator=(Object const&) = delete;
        Object& operator=(Object&&) = delete;

        bool operator==(Object const& other) const
        { return equals(other); }

        bool operator!=(Object const& other) const
        { return !equals(other); }
        
        virtual ObjectType type() const = 0;
        virtual bool equals(Object const& other) const {
            return this == &other;
        }

    private:
        // for GC
        Object* next_;
    };

    class String : public Object
    {
    public:
        String()
        : data_{}
        , hash_{hash_str({})}
        {}

        template<class StringType>
        String(StringType&& value)
        : data_{std::forward<StringType>(value)}
        , hash_{hash_str(data_)}
        { }

        String(String const&) = default;
        String(String&&) = default;

        ~String() override { }

        String& operator=(String const&) = default;
        String& operator=(String&&) = default;

        ObjectType type() const override { return ObjectType::STRING; }

        size_t length() const { return data_.length(); }
        const char* c_str() const { return data_.c_str(); }
        std::string const& str() const { return data_; }

        bool equals(Object const& other) const override {
            if (other.type() != ObjectType::STRING)
                return false;

            String const& other_str = static_cast<String const&>(other);
            return str() == other_str.str();
        }

        size_t hash() const { return hash_; }

        static constexpr size_t hash_str(std::string_view data) {
            const size_t fnv_offset_basis = 14695981039346656037ul;
            const size_t fnv_prime = 1099511628211ul;
            size_t hash{fnv_offset_basis};

            for (char c : data) {
                hash ^= c;
                hash *= fnv_prime;
            }
            return hash;
        }

    private:
        std::string data_;
        size_t hash_;
    };
}
