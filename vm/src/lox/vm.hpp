#pragma once

#include "chunk.hpp"
#include "map.hpp"
#include "object.hpp"
#include <cassert>
#include <string>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

namespace lox
{
    class CallFrame
    {
        Function* function_;
        byte* ip_;
        byte* ip_begin_;
        byte* ip_end_;
        Value* slots_;

    public:
        explicit CallFrame(Function* function = nullptr, Value* slots = nullptr)
        : function_{function}
        , ip_{nullptr}
        , ip_begin_{nullptr}
        , ip_end_{nullptr}
        , slots_{slots}
        {
            if (function)
            {
                auto& chunk = function->chunk();
                ip_ = ip_begin_ = chunk.begin();
                ip_end_ = chunk.end();
                assert(slots);
            }
        }

        CallFrame(CallFrame const&) = default;
        CallFrame(CallFrame&&) = default;

        ~CallFrame() = default;

        CallFrame& operator=(CallFrame const&) = default;
        CallFrame& operator=(CallFrame&&) = default;

        Value* slots() const { return slots_; }
        byte*& ip() { return ip_; }
        size_t pc() const { return static_cast<size_t>(ip_ - ip_begin_); }

        Chunk& chunk() const
        {
            assert(function_);
            return function_->chunk();
        }

        byte read_byte()
        {
            assert(ip_ != ip_end_);
            return *ip_++;
        }

        uint16_t read_short()
        {
            assert(ip_ + 2 <= ip_end_);
            ip_ += 2;
            return static_cast<uint16_t>((ip_[-2] << 8) | ip_[-1]);
        }

        OpCode read_instruction() {
            return static_cast<OpCode>(read_byte());
        }

        Value read_constant() {
            const byte offset{read_byte()};
            return function_->chunk().constants()[offset]; 
        }

        String* read_string()
        {
            auto value = read_constant();
            return dynamic_cast<String*>(value.get<Object*>());
        }
    };

    class VM
    {
    public:
        enum class Result {
            OK,
            COMPILE_ERROR,
            RUNTIME_ERROR,
        };

        static constexpr size_t frames_max = 64;
        static constexpr size_t stack_max = frames_max * 256;

        VM(std::ostream& stdout = std::cout, std::ostream& stderr = std::cerr)
        : stdout_{&stdout}
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

        std::ostream& stdout() const { return *stdout_; }
        void stdout(std::ostream& stdout) { stdout_ = &stdout; }

        std::ostream& stderr() const { return *stderr_; }
        void stderr(std::ostream& stderr) { stderr_ = &stderr; }

        template<typename ObjectType, typename... Args>
        ObjectType* allocate(Args&&... args)
        {
            return new ObjectType(std::forward<Args>(args)...);
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

        void reset()
        {
            stack_top_ = stack_.begin();
            frame_count_ = 0;
        }

    private:
        std::array<CallFrame, frames_max> frames_;
        size_t frame_count_ = 0;
        CallFrame* frame_ = nullptr;
        std::array<Value, stack_max> stack_;
        Value* stack_top_ = &stack_[0];

        Object* objects_ = nullptr;
        Map strings_;
        Map globals_;
        std::ostream* stdout_ = &std::cout;
        std::ostream* stderr_ = &std::cerr;

        CallFrame& current_frame()
        {
            assert(frame_count_ != 0);
            return frames_[frame_count_ - 1];
        }

        void free_objects()
        {
            Object::run_gc();
        }

        Value const& peek(size_t distance) const {
            assert(distance < static_cast<size_t>(stack_top_ - stack_.data()));
            return *(stack_top_ - distance - 1);
        }

        void push(Value value)
        {
            assert(stack_top_ != stack_.end());
            *stack_top_++ = value;
        }

        Value pop() {
            assert(stack_top_ != stack_.begin());
            Value tmp{std::move(*(stack_top_ - 1))};
            --stack_top_;
            return tmp;
        }

        Result run();

        byte read_byte()
        {
            assert(frame_);
            return frame_->read_byte();
        }

        uint16_t read_short()
        {
            assert(frame_);
            return frame_->read_short();
        }

        OpCode read_instruction()
        {
            return static_cast<OpCode>(read_byte());
        }

        Value read_constant()
        {
            assert(frame_);
            return frame_->read_constant();
        }

        String* read_string()
        {
            assert(frame_);
            return frame_->read_string();
        }

        template<typename... Args>
        void runtime_error(fmt::format_string<Args...>&& format, Args&&... args)
        {
            auto& frame = current_frame();
            size_t instruction = frame.ip() - frame.chunk().begin() - 1;
            size_t line = frame.chunk().lines()[instruction];

            stderr() << fmt::format(
                std::forward<decltype(format)>(format),
                std::forward<Args>(args)...
            );
            fmt::print(*stderr_, "\n[line {}] in script\n", line);
        }
    };
}
