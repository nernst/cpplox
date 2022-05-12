#pragma once

#include <chrono>

#include "../callable.hpp"
#include "../utility.hpp"

namespace lox::builtin
{
	struct clock final : callable::builtin
	{
		std::size_t arity() const override { return 0; }

		object call(interpreter& inter, std::vector<object> const& args) override;

		std::string name() const override { return "clock"; }
	};
}

