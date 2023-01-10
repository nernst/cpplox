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
}

