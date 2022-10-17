#pragma once

#include "chunk.hpp"
#include <cassert>
#include <string>
#include <fmt/format.h>

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

        VM() = default;

        VM(Chunk&& chunk)
        : chunk_(std::move(chunk))
        , ip_{nullptr}
        , objects_{nullptr}
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

        template<typename ObjectType, typename... Args>
        ObjectType* allocate(Args&&... args)
        {
            ObjectType* obj = new ObjectType(std::forward<Args>(args)...);
            obj->next_ = objects_;
            objects_ = obj;
            return obj;
        }

    private:
        Chunk chunk_;
        byte const* ip_;
        byte const* ip_start_;
        byte const* ip_end_;
        std::vector<Value> stack_;
        Object* objects_;

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

        template<typename... Args>
        constexpr void runtime_error(fmt::format_string<Args...> format, Args... args)
        {
            size_t instruction = pc() - 1;
            size_t line = chunk_.lines()[instruction];

            fmt::print(stderr, format, args...);
            fmt::print(stderr, "\n[line {}] in script\n", line);
        }
    };
}
