#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#define DEBUG_COMPILE
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

namespace lox
{
	using byte = std::uint8_t;
	using std::size_t;


	template<typename T>
	void ignore(T&&...) { }

	[[noreturn]] inline void unreachable()
	{
		assert(false);
		abort();
	}

	template<class> inline constexpr bool always_false_v = false;
	template<class> inline constexpr bool always_true_v = true;

	inline std::string strip(std::string value)
	{
		const char* const whitespace = " \t\n";
		auto start = value.find_first_not_of(whitespace);
		auto end = value.find_last_not_of(whitespace);
		if (end != std::string::npos)
			++end;

		if (start == end || start == std::string::npos)
			return {};
		
		if (start == 0 && end == value.size())
			return value;
		
		return value.substr(start, end - start);
	}
}

