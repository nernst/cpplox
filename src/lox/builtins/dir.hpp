#pragma once

#include "../callable.hpp"
#include "../utility.hpp"

namespace lox::builtin
{
	struct dir final : callable::builtin
	{
		std::size_t arity() const override { return 0; }
		object call(interpreter& inter, std::vector<object> const& args) override;
		std::string name() const override { return "dir"; }
	};
}
