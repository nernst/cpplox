#pragma once

#include <cstddef>
#include <cstdint>

namespace lox
{
	using byte = std::uint8_t;
	using std::size_t;


	template<typename T>
	void ignore(T&&...) { }

}

