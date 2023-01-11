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
		OP_GET_GLOBAL,
		OP_DEFINE_GLOBAL,
		OP_SET_GLOBAL,
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
		OP_RETURN,
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

			constants_.write(value);
			return constants_.size() - 1;
		}

		std::vector<byte> const& code() const { return code_; }
		ValueArray const& constants() const { return constants_; }
		std::vector<size_t> const& lines() const { return lines_; }

	private:
		std::vector<byte> code_;
		ValueArray constants_;
		std::vector<size_t> lines_;
	};
}

