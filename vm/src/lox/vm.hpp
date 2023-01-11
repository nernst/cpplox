#pragma once

#include "chunk.hpp"
#include "map.hpp"
#include <cassert>
#include <string>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace lox
{
    class VM
    {
    public:
        enum class Result {
            OK,
            COMPILE_ERROR,
            RUNTIME_ERROR,
        };

        VM(std::ostream& stdout = std::cout, std::ostream& stderr = std::cerr)
        : stdout_{&stdout}
        , stderr_{&stderr}
        { }

        VM(Chunk&& chunk, std::ostream& stdout = std::cout, std::ostream& stderr = std::cerr)
        : chunk_(std::move(chunk))
        , ip_{nullptr}
        , objects_{nullptr}
        , stdout_{&stdout}
        , stderr_{&stderr}
        { }

        VM(VM const&) = delete;
        VM(VM&&) = default;

        ~VM()
        {
            free_objects();
        }

        VM& operator=(VM const&) = delete;
        VM& operator=(VM&&) = default;

        Result interpret(std::string const& source);
        Result interpret();

        std::ostream& stdout() const { return *stdout_; }
        void stdout(std::ostream& stdout) { stdout_ = &stdout; }

        std::ostream& stderr() const { return *stderr_; }
        void stderr(std::ostream& stderr) { stderr_ = &stderr; }

        template<typename ObjectType, typename... Args>
        ObjectType* allocate(Args&&... args)
        {
            ObjectType* obj = new ObjectType(std::forward<Args>(args)...);
            obj->next_ = objects_;
            objects_ = obj;
            return obj;
        }

        template<typename T>
        String* allocate_str(T&& arg)
        {
            String* str;
            if (strings_.get(std::forward<T&>(arg), str))
            {
                return str;
            }
            else
            {
                str = new String(std::forward<T&&>(arg));
                strings_.add(str, nullptr);
            }
        }

    private:
        Chunk chunk_;
        byte const* ip_ = nullptr;
        byte const* ip_start_ = nullptr;
        byte const* ip_end_ = nullptr;
        std::vector<Value> stack_;
        Object* objects_ = nullptr;
        Map strings_;
        Map globals_;
        std::ostream* stdout_ = &std::cout;
        std::ostream* stderr_ = &std::cerr;

        void free_objects() {
            Object* object = objects_;
            while (object != nullptr) {
                Object* next = object->next_;
                delete object;
                object = next;
            }
        }

        Value const& peek(size_t distance) const {
            assert(distance < stack_.size());
            return stack_[stack_.size() - 1 - distance];
        }

        void push(Value value){ stack_.push_back(value); }

        Value pop() {
            assert(!stack_.empty());
            Value tmp{std::move(stack_.back())};
            stack_.pop_back();
            return tmp;
        }

        Result run();

        size_t pc() const { return static_cast<size_t>(ip_ - ip_start_); }

        byte read_byte() {
            assert(ip_ != ip_end_);
            return *ip_++;
        }

        OpCode read_instruction() {
            return static_cast<OpCode>(read_byte());
        }

        Value read_constant() {
            const byte offset{read_byte()};
            return chunk_.constants()[offset]; 
        }

        String* read_string()
        {
            auto value = read_constant();
            return dynamic_cast<String*>(value.get<Object*>());
        }

        template<typename... Args>
        void runtime_error(fmt::format_string<Args...>&& format, Args&&... args)
        {
            size_t instruction = pc() - 1;
            size_t line = chunk_.lines()[instruction];

            stderr() << fmt::format(
                std::forward<decltype(format)>(format),
                std::forward<Args>(args)...
            );
            fmt::print(*stderr_, "\n[line {}] in script\n", line);
        }
    };
}
