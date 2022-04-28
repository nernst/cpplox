#pragma once

#include <memory>
#include "expr.hpp"

namespace lox {

	class statement;
	using statement_ptr = std::unique_ptr<statement>;

	class statement
	{
	public:
		statement() = delete;
		statement(statement const&) = delete;
		statement(statement&&) = default;

		explicit statement(expression_ptr&& expr)
		: expr_{std::move(expr)}
		{ }

		statement& operator=(statement const&) = delete;
		statement& operator=(statement&&) = default;

		expression const& expr() const { return *expr_; }

	private:
		expression_ptr expr_;
	};

	class print_statement
	{
	};

} // namespace lox

