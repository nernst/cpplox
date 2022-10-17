#pragma once

#include "common.hpp"

namespace lox
{
    enum class ObjectType
    {
        STRING,
    };

    class Object;

    void print_object(Object const& object);

    class Object
    {
    public:

        virtual ~Object() = 0;
        
        virtual ObjectType type() const = 0;

    private:
    };

    class String : public Object
    {
    public:
        String() {}

        template<class StringType>
        String(StringType&& value)
        : data_{std::forward<StringType>(value)}
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

    private:
        std::string data_;
    };
}
