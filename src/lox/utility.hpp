#pragma once

#include <iostream>

namespace lox {

	template<class... Types>
	void ignore_unused(Types&&...) { }


	template<class>
	constexpr bool always_false_v = false;


	template<typename T>
	T trim(T const& str)
	{
		static char whitespace[] = " \t\n\r\f";
		auto start = str.find_first_not_of(whitespace);
		if (start == T::npos)
			return T{};

		auto end = str.find_last_not_of(whitespace);
		if (end == T::npos)
			return T{};
		++end;

		return str.substr(start, end - start);
	}

}

