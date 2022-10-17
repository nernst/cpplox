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
        
        virtual ObjectType type() const = 0;

    private:
        // for GC
        Object* next_;
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
