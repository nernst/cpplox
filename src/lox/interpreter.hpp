#pragma once

#include <iostream>

#include "environment.hpp"
#include "exceptions.hpp"
#include "expr.hpp"
#include "statement.hpp"
#include "utility.hpp"

namespace lox {

class interpreter
	: public expression::visitor
	, public statement::visitor
{
public:

	using statement_vec = std::vector<statement_ptr>;

	void interpret(statement_vec const& statements)
	{
		for (auto&& s : statements)
			execute(*s);
	}

	// statements

	void visit(expression_stmt const& stmt) override
	{
		evaluate(stmt.expr());
	}

	void visit(print_stmt const& stmt) override
	{
		auto value{evaluate(stmt.expr())};
		std::cout << value.str() << std::endl;
	}

	void visit(var_stmt const& stmt) override
	{
		object value;
		if (stmt.initializer())
			value = evaluate(*stmt.initializer());

		environment_.define(std::string{stmt.name().lexeme()}, std::move(value));
	}


	// expressions
	object const& result() const { return result_; }


	void visit(unary const& unary) override
	{
		auto right = evaluate(unary.right());

		switch(unary.op_token().type())
		{
			case token_type::BANG:
				result_ = !right;
				break;
				
			case token_type::MINUS:
				result_ = -right;
				break;
		
			default:
				LOX_THROW(programming_error, fmt::format("unsupported token: {}", unary.op_token().type()));		
		}
	}

	void visit(binary const& binary) override
	{
		auto left{evaluate(binary.left())};
		auto right{evaluate(binary.right())};

		switch(binary.op_token().type())
		{
			case token_type::BANG_EQUAL:
				result_ = object{left != right};
				break;

			case token_type::EQUAL_EQUAL:
				result_ = object{left == right};
				break;

			case token_type::MINUS:
				result_ = left - right;
				break;

			case token_type::PLUS:
				result_ = left + right;
				break;

			case token_type::SLASH:
				result_ = left / right;
				break;

			case token_type::STAR:
				result_ = left * right;
				break;

			case token_type::GREATER:
				result_ = object{left > right};
				break;

			case token_type::GREATER_EQUAL:
				result_ = object{left >= right};
				break;

			case token_type::LESS:
				result_ = object{left < right};
				break;

			case token_type::LESS_EQUAL:
				result_ = object{left <= right};
				break;

			default:
				LOX_THROW(programming_error, fmt::format("unhandled binary operator: '{}'", binary.op_token().lexeme()));
		}
	}

	void visit(grouping const& grouping) override
	{
		result_ = evaluate(grouping.expr());
	}

	void visit(literal const& literal) override
	{
		result_ = literal.value();
	}

	void visit(variable const& variable) override
	{
		result_ = environment_.get(variable.name());
	}

	void visit(assign const& expr) override
	{
		auto value = evaluate(expr.value());
		environment_.assign(expr.name(), value);
		result_ = value;
	}

private:
	object result_;
	environment environment_;

	object evaluate(expression const& expr)
	{
		expr.accept(*this);
		return result();
	}

	void execute(statement const& stmt)
	{
		stmt.accept(*this);
	}
};

} // namespace lox

