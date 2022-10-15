#pragma once

#include "chunk.hpp"
#include <cassert>

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
        { }

        VM(VM const&) = delete;
        VM(VM&&) = default;

        ~VM() = default;
        VM& operator=(VM const&) = delete;
        VM& operator=(VM&&) = default;

        Result interpret();

    private:
        Chunk chunk_;
        byte const* ip_;
        byte const* ip_start_;
        byte const* ip_end_;
        std::vector<Value> stack_;

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
    };
}
