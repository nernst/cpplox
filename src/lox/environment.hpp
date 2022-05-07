#pragma once

#include <unordered_map>
#include <fmt/format.h>

#include "object.hpp"

namespace lox {


class environment
{
	using value_map = std::unordered_map<std::string, object>;

	template<typename T>
	[[noreturn]]
	static void undefined(T const& name)
	{
		throw runtime_error{fmt::format("Undefined variable '{}'.", name)};
	}

public:
	environment() {}

	template<typename T, typename U>
	void define(T&& name, U&& value)
	{
		values_.insert_or_assign(
			std::forward<T>(name),
			std::forward<U>(value)
		);
	}

	template<typename U>
	void assign(std::string const& name, U&& value)
	{
		auto i = values_.find(name);
		if (i == end(values_))
			undefined(name);

		i->second = std::forward<U>(value);
	}

	object get(std::string const& name) const
	{
		auto i = values_.find(name);
		if (i == values_.end())
			undefined(name);

		return i->second;
	}

private:
	value_map values_;
};

} // namespace lox
