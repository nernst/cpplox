#pragma once

#include "common.hpp"
#include "value.hpp"

#include <cassert>
#include <string_view>
#include <vector>

namespace lox
{
	enum class OpCode : byte
	{
		OP_CONSTANT,
		OP_NIL,
		OP_TRUE,
		OP_FALSE,
		OP_POP,
		OP_GET_LOCAL,
		OP_SET_LOCAL,
		OP_DEFINE_GLOBAL,
		OP_GET_GLOBAL,
		OP_SET_GLOBAL,
		OP_GET_UPVALUE,
		OP_SET_UPVALUE,
		OP_GET_PROPERTY,
		OP_SET_PROPERTY,
		OP_EQUAL,
		OP_GREATER,
		OP_LESS,
		OP_ADD,
		OP_SUBTRACT,
		OP_MULTIPLY,
		OP_DIVIDE,
		OP_NOT,
		OP_NEGATE,
		OP_PRINT,
		OP_JUMP,
		OP_JUMP_IF_FALSE,
		OP_LOOP,
		OP_CALL,
		OP_CLOSURE,
		OP_CLOSE_UPVALUE,
		OP_RETURN,
		OP_CLASS,
		OP_METHOD,
	};


	class Chunk
	{
	public:
		Chunk() = default;
		Chunk(Chunk const&) = default;
		Chunk(Chunk&&) = default;

		~Chunk() = default;

		Chunk& operator=(Chunk const& copy) = default;
		Chunk& operator=(Chunk&& other) = default;

		void write(byte value, size_t line)
		{
			code_.push_back(value);
			lines_.push_back(line);
		}

		void write(OpCode value, size_t line)
		{ 
			write(static_cast<byte>(value), line);
		}

		size_t add_constant(Value value)
		{
			assert(constants_.size() < 256);

			auto iter = std::find(std::begin(constants_), std::end(constants_), value);
			if (iter == std::end(constants_))
			{
				constants_.push_back(value);
				return constants_.size() - 1;
			};
			return iter - std::begin(constants_);
		}

		std::vector<byte> const& code() const { return code_; }
		std::vector<byte>& code() { return code_; }
		std::vector<Value> const& constants() const { return constants_; }
		std::vector<size_t> const& lines() const { return lines_; }

		byte* begin() { return &code_.front(); }
		byte* end() { return begin() + code_.size(); }

		byte const* begin() const { return &code_.front(); }
		byte const* end() const { return begin() + code_.size(); }


	private:
		std::vector<byte> code_;
		std::vector<Value> constants_;
		std::vector<size_t> lines_;
	};
}

