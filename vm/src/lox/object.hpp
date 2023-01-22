#pragma once

#include "common.hpp"
#include "chunk.hpp"
#include "gc.hpp"
#include <iosfwd>
#include <memory>

namespace lox
{
    class Object;
    class ObjUpvalue;
    class ObjClass;
    class ObjInstance;
    class VM;

    void print_object(std::ostream& stream, Object const& object);

    template<typename It>
    constexpr size_t hash(It begin, It end)
    {
        constexpr size_t fnv_offset_basis = 14695981039346656037ul;
        constexpr size_t fnv_prime = 1099511628211ul;

        size_t hash{fnv_offset_basis};

        while (begin != end)
        {
            hash ^= *begin++;
            hash *= fnv_prime;
        }
        return hash;
    }

    inline constexpr size_t hash(std::string_view data)
    {
        constexpr std::span<byte> empty{};

        if (data.empty())
            return hash(std::begin(empty), std::end(empty));

        auto begin_ = reinterpret_cast<byte const*>(&data.front()); 
        auto end_ = begin_ + data.size();

        return hash(begin_, end_);
    }

    class Object
    {
    public:
        Object();
        Object(Object const&) = delete;
        Object(Object&&) = delete;

        virtual ~Object() noexcept = 0;

        Object& operator=(Object const&) = delete;
        Object& operator=(Object&&) = delete;

        bool operator==(Object const& other) const
        { return equals(other); }

        bool operator!=(Object const& other) const
        { return !equals(other); }
        
        virtual ObjectType type() const = 0;
        virtual const char* type_name() const = 0;

        virtual bool equals(Object const& other) const {
            return this == &other;
        }

        virtual size_t hash() const { return reinterpret_cast<size_t>(this); }

    protected:
        friend class Trackable;
        friend class GC;
        Object* gc_next_ = nullptr;
        bool gc_marked_ = false;

        virtual void gc_blacken(GC& gc);
    };

    class String : public Object
    {
        String()
        : Object{}
        , data_{}
        , hash_{lox::hash(std::string_view{})}
        {}

        template<class StringType>
        explicit String(StringType&& value)
        : Object{}
        , data_{std::forward<StringType>(value)}
        , hash_{lox::hash(data_)}
        { }

        template<class StringType>
        static String* do_create(StringType&& value);

    public:

        String(String const&) = delete;
        String(String&&) = delete;

        ~String() noexcept override { }

        static String* create(const char* value);
        static String* create(std::string_view value);
        static String* create(std::string&& value);

        String& operator=(String const&) = default;
        String& operator=(String&&) = default;

        ObjectType type() const override { return ObjectType::STRING; }
        const char* type_name() const override { return "String"; }

        size_t length() const { return data_.length(); }
        const char* c_str() const { return data_.c_str(); }
        std::string const& str() const { return data_; }
        std::string_view view() const { return data_; }

        bool equals(Object const& other) const override {
            if (other.type() != ObjectType::STRING)
                return false;

            String const& other_str = static_cast<String const&>(other);
            return str() == other_str.str();
        }

        size_t hash() const override { return hash_; }

    private:
        std::string data_;
        size_t hash_;
    };

    class Function : public Object
    {
    public:
        static constexpr unsigned max_parameters = 255;
        static constexpr unsigned max_upvalues = max_parameters;

        Function() = default;
        Function(unsigned arity, unsigned upvalues, Chunk&& chunk, String* name)
        : Object{}
        , arity_{arity}
        , chunk_{std::move(chunk)}
        , name_{name}
        , upvalues_{upvalues}
        { }

        explicit Function(String* name)
        : Object{}
        , name_{name}
        { }

        Function(Function const&) = delete;
        Function(Function&&) = delete;

        ~Function() noexcept override {}

        Function& operator=(Function const&) = delete;
        Function& operator=(Function&&) = delete;

        ObjectType type() const override { return ObjectType::FUNCTION; }
        const char* type_name() const override { return "Function"; }

        unsigned arity() const { return arity_; }

        void arity(unsigned value)
        {
            assert(value <= max_parameters);
            arity_ = value;
        }

        unsigned upvalues() const { return upvalues_; }

        void upvalues(unsigned value)
        {
            assert(value <= max_upvalues);
            upvalues_ = value;
        }


        String* name() const { return name_; }
        Chunk const& chunk() const { return chunk_; }
        Chunk& chunk() { return chunk_; }

    private:
        unsigned arity_ = 0;
        Chunk chunk_;
        String* name_ = nullptr;
        unsigned upvalues_ = 0;

        void gc_blacken(GC& gc) override;
    };

    class NativeFunction : public Object
    {
    public:
        using NativeFn = Value (*)(int, Value*);

        NativeFunction() = delete;
        NativeFunction(NativeFunction const&) = delete;
        NativeFunction(NativeFunction&&) = delete;

        explicit NativeFunction(NativeFn native)
        : Object{}
        , native_{native}
        {}

        ~NativeFunction() noexcept override {}

        NativeFunction& operator=(NativeFunction const&) = delete;
        NativeFunction& operator=(NativeFunction&&) = delete;

        ObjectType type() const override { return ObjectType::NATIVE_FUNCTION; }
        const char* type_name() const override { return "NativeFunction"; }

        Value invoke(int arg_count, Value* args) const
        { return native_(arg_count, args); }

    private:
        NativeFn native_;
    };


    class Closure : public Object
    {
    public:
        explicit Closure(Function* function)
        : Object{}
        , function_{function}
        , upvalue_count_{assert_not_null(function)->upvalues()}
        , upvalues_{upvalue_count_ == 0 ? nullptr : new ObjUpvalue*[upvalue_count_]{nullptr}}
        { }

        Closure() = delete;
        Closure(Closure const&) = delete;
        Closure(Closure&&) = delete;

        ~Closure() noexcept override {}

        Closure& operator=(Closure const&) = delete;
        Closure& operator=(Closure&&) = delete;

        Function* function() const { return function_; }
        unsigned upvalue_count() const { return upvalue_count_; }
        ObjUpvalue** upvalues() const { return upvalues_.get(); }

        ObjectType type() const override { return ObjectType::CLOSURE; }
        const char* type_name() const override { return "Closure"; }

    private:
        friend class VM;

        Function* function_;
        unsigned upvalue_count_;
        std::unique_ptr<ObjUpvalue*[]> upvalues_;

        void gc_blacken(GC& gc) override;
    };

    class ObjUpvalue : public Object
    {
    public:
        explicit ObjUpvalue(Value* location)
        : Object{}
        , location_{location}
        { assert(location); }

        ObjUpvalue() = delete;
        ObjUpvalue(ObjUpvalue const&) = delete;
        ObjUpvalue(ObjUpvalue&&) = delete;

        ~ObjUpvalue() noexcept override {}

        ObjUpvalue& operator=(ObjUpvalue const&) = delete;
        ObjUpvalue& operator=(ObjUpvalue&&) = delete;

        Value* location() const { return location_; }

        void close()
        {
            if (location_ == &closed_)
                return;

            closed_ = std::move(*location_);
            location_ = &closed_;
        }

        ObjectType type() const override { return ObjectType::OBJUPVALUE; }
        const char* type_name() const override { return "ObjUpvalue"; }

    private:
        Value* location_; 
        Value closed_;
        ObjUpvalue* next_ = nullptr;

        friend class VM;

        void gc_blacken(GC& gc) override;
    };
}
