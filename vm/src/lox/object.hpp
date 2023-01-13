#pragma once

#include "common.hpp"
#include "chunk.hpp"

namespace lox
{
    class Object;
    class VM;

    void print_object(Object const& object);

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
        friend class VM;

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

        virtual size_t hash() const = 0;

    private:
        // for GC
        Object* gc_next_;

        static Object* gc_root();
        static void run_gc();
    };

    class String : public Object
    {
    public:
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

        String(String const&) = default;
        String(String&&) = default;

        ~String() noexcept override { }

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

        Function()
        : Object{}
        , arity_{0}
        , chunk_{}
        , name_{nullptr}
        { }
        
        Function(unsigned arity, Chunk&& chunk, String* name)
        : Object{}
        , arity_{arity}
        , chunk_{std::move(chunk)}
        , name_{name}
        { }

        explicit Function(String* name)
        : Object{}
        , arity_{0}
        , chunk_{}
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

        String* name() const { return name_; }
        Chunk const& chunk() const { return chunk_; }
        Chunk& chunk() { return chunk_; }

        size_t hash() const override { return reinterpret_cast<size_t>(this); }

    private:
        unsigned arity_;
        Chunk chunk_;
        String* name_;
    };
}
