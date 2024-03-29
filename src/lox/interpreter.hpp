#pragma once

#include <iostream>
#include <ranges>

#include "environment.hpp"
#include "exceptions.hpp"
#include "expr.hpp"
#include "statement.hpp"
#include "utility.hpp"
#include "callable.hpp"

namespace lox {

class interpreter
	: public expression::visitor
	, public statement::visitor
{
	std::istream* stdin_;
	std::ostream* stdout_;
	std::ostream* stderr_;

public:

	struct return_value
	{
		object value_;
	};

	using statement_vec = std::vector<statement_ptr>;

	interpreter(
		std::istream* stdin,
		std::ostream* stdout,
		std::ostream* stderr
	)
		: stdin_{stdin}
		, stdout_{stdout}
		, stderr_{stderr}
	{
		assert(stdin_);
		assert(stdout_);
		assert(stderr_);

		for (auto&& callable : callable::builtins())
		{
			global_env().define(callable.name(), object{std::move(callable)});
		}
	}

	scope_stack& stack() { return stack_; }
	environment& global_env() { return stack_.global(); }
	environment& current_env() { return stack_.current(); }

	void resolve(expression const& expr, size_t depth)
	{
		locals_.insert_or_assign(&expr, depth);
	}

	void interpret(statement_vec const& statements)
	{
		execute_block(statements);
	}

	void execute_block(block_stmt::statements_t const& statements)
	{
		for (auto&& stmt : statements)
			execute(*stmt);
	}


	// statements

	void visit(expression_stmt const& stmt) override
	{
		evaluate(stmt.expr());
	}

	void visit(print_stmt const& stmt) override
	{
		auto value{evaluate(stmt.expr())};
		*stdout_ << value.str() << std::endl;
	}

	void visit(var_stmt const& stmt) override
	{
		object value;
		if (stmt.initializer())
			value = evaluate(*stmt.initializer());

		current_env().define(std::string{stmt.name().lexeme()}, std::move(value));
	}

	void visit(block_stmt const& stmt) override
	{
		scope s{&stack_};
		ignore_unused(s);

		execute_block(stmt.statements());
	}

	void visit(if_stmt const& stmt) override
	{
		bool condition{evaluate(stmt.condition())};
		if (condition)
			execute(stmt.then_branch());
		else if (auto&& else_branch = stmt.else_branch())
			execute(*else_branch);
	}

	void visit(while_stmt const& stmt) override
	{
		while (static_cast<bool>(evaluate(stmt.condition())))
			execute(stmt.body());
	}

	void visit(func_stmt const& stmt) override
	{
		ignore_unused(stmt);
		auto func{callable::make_lox_function(stmt, stack_.current().shared_from_this())};
		current_env().define(std::string{stmt.name().lexeme()}, object{func});
	}

	[[noreturn]]
	void visit(return_stmt const& stmt) override
	{
		object value;

		auto expr{stmt.value()};
		if (expr)
			value = evaluate(*expr);

		throw return_value{std::move(value)};
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
				throw programming_error{
					fmt::format(
						"unsupported token: {}",
						static_cast<int>(unary.op_token().type())
					)
				};		
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

	void visit(call const& call) override
	{
		const auto nargs{call.arguments().size()};
		auto callee{evaluate(call.callee())};
		std::vector<object> args;
		args.reserve(nargs);

		std::ranges::copy(
			std::views::transform(call.arguments(), [this](auto const& exp) { return evaluate(*exp); }),
			std::back_inserter(args)
		);

		callable func;

		try
		{
			func = callee.get<callable>();
		}
		catch(type_error const&)
		{
			throw type_error{"Only functions and classes are callable."};
		}

		assert(func.valid());

		if (nargs != func.arity())
			throw runtime_error{
				fmt::format("Exepcted {} arguments but got {}.", func.arity(), nargs)
			};
			
		result_ = func(*this, args);
	}

	void visit(logical const& logical) override
	{
		object left{evaluate(logical.left())};

		if (logical.op_token().type() == token_type::OR)
		{
			if (static_cast<bool>(left))
			{
				result_ = left;
				return;
			}
		}
		else
		{
			if (!static_cast<bool>(left))
			{
				result_ = left;
				return;
			}
		}

		result_ = evaluate(logical.right());
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
		result_ = lookup_variable(variable.name_token(), variable);
	}

	void visit(assign const& expr) override
	{
		auto value = evaluate(expr.value());
		auto i{locals_.find(&expr)};
		if (i == locals_.end())
			global_env().assign(expr.name(), std::move(value));
		else
			current_env().assign_at(i->second, expr.name_token(), std::move(value));

		result_ = value;
	}

private:
	object result_;
	scope_stack stack_;

	using locals_t = std::unordered_map<expression const*, std::size_t>;
	locals_t locals_;

	object lookup_variable(token const& name, expression const& expr)
	{
		auto i{locals_.find(&expr)};
		if (i == locals_.end())
			return global_env().get(std::string{name.lexeme()});
		else
			return current_env().get_at(i->second, std::string{name.lexeme()});
	}

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

