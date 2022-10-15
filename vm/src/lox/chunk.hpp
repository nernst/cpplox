#pragma once

#include "common.hpp"
#include <string_view>
#include <vector>

namespace lox
{
	enum class OpCode : byte
	{
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

		void write(byte value) { data_.push_back(value); }
		void write(OpCode value) { write(static_cast<byte>(value)); }

		void disassemble(std::string_view name) const;
		size_t disassemble(size_t offset) const;
		size_t simple(std::string_view name, size_t offset) const;

	private:
		std::vector<byte> data_;
	};
}

