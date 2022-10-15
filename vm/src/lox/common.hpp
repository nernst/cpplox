#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define DEBUG_TRACE_EXECUTION

namespace lox
{
	using byte = std::uint8_t;
	using std::size_t;


	template<typename T>
	void ignore(T&&...) { }

}

