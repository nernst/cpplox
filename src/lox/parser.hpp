#pragma once

#include <source_location>
#include <vector>
#include "exceptions.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace lox {

using token_vec = std::vector<token>;

class parser
{
public:

	explicit parser(token_vec&& tokens, std::ostream* error = &std::cerr)
	: tokens_{std::move(tokens)}
	, error_(error)
	, current_{0}
	, end_{tokens_.size()}
	, had_error_{false}
	{ }

	parser(parser const&) = delete;
	parser(parser&&) = default;

	parser& operator=(parser const&) = delete;
	parser& operator=(parser&&) = default;

	using statement_vec = std::vector<statement_ptr>;

	bool had_error() const { return had_error_; }

	std::tuple<bool, statement_vec> parse()
	{
		statement_vec statements;
		
		while (!is_at_end())
		{
			statements.push_back(declaration());
		}

		return std::make_tuple(had_error_, std::move(statements));
	}


private:
	token_vec tokens_;
	std::ostream* error_;
	std::size_t current_;
	std::size_t end_;
	bool had_error_;

	template<class T, class... Args>
	static expression_ptr make_expr(Args&&... args)
	{ return expression::make<T>(std::forward<Args>(args)...); }

	template<class T, class... Args>
	static statement_ptr make_stmt(Args&&... args)
	{ return statement::make<T>(std::forward<Args>(args)...); }

	template<token_type... types>
	bool match()
	{
		static_assert(sizeof...(types) > 0);
		static constexpr token_type to_match[] = {types...};

		for (auto type : to_match)
		{
			if (check(type))
			{
				advance();
				return true;
			}
		}

		return false;
	}

	bool check(token_type type)
	{
		if (is_at_end())
			return false;

		return peek().type() == type;
	}

	token const& advance()
	{
		if (!is_at_end())
			++current_;

		return previous();
	}

	bool is_at_end() const { return peek().type() == token_type::END_OF_FILE; }
	token const& peek() const { return tokens_[current_]; }

	token const& previous() const
	{
		assert(current_ != 0);
		return tokens_[current_ - 1];
	}

	parse_error on_error(
		token const& tok, 
		std::string_view message
	)
	{
		auto const& loc = tok.source_location();
		auto [line_no, line_off, line] = loc.get_line();

		*error_ << "in " << loc.where().name() << " (" << line_no << ':' << line_off << "): " << message << '\n';
		*error_ << fmt::format("{:>5} |", line_no) << line << '\n';
		*error_ << "      |";
		for (size_t count = line_off; count; --count)
			*error_ << ' ';
		*error_ << "^\n";

		// log error
		return parse_error{tok.type(), message};
	}


	token const& consume(token_type type, std::string_view message)
	{
		if (check(type))
			return advance();

		throw on_error(peek(), message);
	}

	void synchronize()
	{
		had_error_ = true;
		advance();

		while (!is_at_end())
		{
			if (previous().type() == token_type::SEMICOLON)
				return;

			switch(peek().type())
			{
				case token_type::CLASS:
				case token_type::FUN:
				case token_type::VAR:
				case token_type::FOR:
				case token_type::IF:
				case token_type::WHILE:
				case token_type::PRINT:
				case token_type::RETURN:
					return;

				default:
					break;
			}

			advance();
		}
	}
	
	expression_ptr expr()
	{
		try
		{
			return assignment();
		}
		catch(parse_error const&)
		{
			return expression_ptr{};
		}
	}

	expression_ptr assignment()
	{
		auto expr{logical_or()};

		if (match<token_type::EQUAL>())
		{
			token equals{previous()};
			auto value = assignment();

			auto var = dynamic_cast<variable const*>(expr.get());
			if (var != nullptr)
				return make_expr<assign>(var->name_token(), std::move(value));

			on_error(equals, "Invalid assignment target.");
		}

		return expr;
	}

	expression_ptr logical_or()
	{
		auto expr{logical_and()};

		while (match<token_type::OR>())
		{
			token op{previous()};
			auto right{logical_and()};
			expr = make_expr<logical>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr logical_and()
	{
		auto expr{equality()};

		while (match<token_type::AND>())
		{
			token op{previous()};
			auto right{equality()};
			expr = make_expr<logical>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr equality()
	{
		auto expr = comparison();

		while (match<token_type::BANG_EQUAL, token_type::EQUAL_EQUAL>())
		{
			token op = previous();
			auto right = comparison();
			expr = make_expr<binary>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr comparison()
	{
		auto expr = term();

		while (match<token_type::GREATER, token_type::GREATER_EQUAL, token_type::LESS, token_type::LESS_EQUAL>())
		{
			token op = previous();
			auto right = term();
			expr = make_expr<binary>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr term()
	{
		auto expr = factor();

		while (match<token_type::MINUS, token_type::PLUS>())
		{
			token op = previous();
			auto right = factor();
			expr = make_expr<binary>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr factor()
	{
		auto expr = unary();

		while (match<token_type::SLASH, token_type::STAR>())
		{
			token op = previous();
			auto right = unary();
			expr = make_expr<binary>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr unary()
	{
		if (match<token_type::BANG, token_type::MINUS>())
		{
			token op = previous();
			auto right = unary();
			return make_expr<::lox::unary>(std::move(op), std::move(right));
		}

		return primary();
	}

	expression_ptr primary()
	{
		if (match<token_type::FALSE_L, token_type::TRUE_L, token_type::NIL, token_type::NUMBER, token_type::STRING>())
			return make_expr<literal>(previous().literal());

		if (match<token_type::IDENTIFIER>()) return make_expr<variable>(previous());
		
		if (match<token_type::LEFT_PAREN>())
		{
			auto e{expr()};
			consume(token_type::RIGHT_PAREN, "Expect ')' after expression.");

			return make_expr<grouping>(std::move(e));
		}

		throw on_error(peek(), "Expect expression.");
		return nullptr;
	}

	statement_ptr declaration()
	{
		try
		{
			if (match<token_type::VAR>())
				return var_declaration();

			return statement();
		}
		catch(parse_error const&)
		{
			synchronize();
			return nullptr;
		}
	}

	statement_ptr var_declaration()
	{
		auto name{consume(token_type::IDENTIFIER, "Expect variable name.")};
		expression_ptr initializer;

		if (match<token_type::EQUAL>())
			initializer = expr();

		consume(token_type::SEMICOLON, "Expect ';' after variable declaration.");
		return make_stmt<var_stmt>(std::move(name), std::move(initializer));
	}

	statement_ptr statement()
	{
		if (match<token_type::FOR>())
			return for_statement();

		if (match<token_type::IF>())
			return if_statement();

		if (match<token_type::PRINT>())
			return print_statement();

		if (match<token_type::WHILE>())
			return while_statement();

		if (match<token_type::LEFT_BRACE>())
			return make_stmt<block_stmt>(block());

		return expression_statement();
	}

	statement_ptr for_statement()
	{
		consume(token_type::LEFT_PAREN, "Expect '(' after 'for'.");

		statement_ptr initializer;

		if (match<token_type::SEMICOLON>())
		{
			// no-op - initializer remains null.
		}
		else if (match<token_type::VAR>())
		{
			initializer = var_declaration();
		}
		else
		{
			initializer = expression_statement();
		}

		expression_ptr condition;

		if (!check(token_type::SEMICOLON))
			condition = expr();
		else
			condition = make_expr<literal>(true);

		consume(token_type::SEMICOLON, "Expect ';' after for-loop condition.");

		expression_ptr increment;
		if (!check(token_type::RIGHT_PAREN))
			increment = expr();

		consume(token_type::RIGHT_PAREN, "Expect ')' after for clauses.");

		auto body{statement()};

		if (increment)
		{
			statement_vec statements;
			statements.push_back(std::move(body));
			statements.push_back(make_stmt<expression_stmt>(std::move(increment)));
			body = make_stmt<block_stmt>(std::move(statements));
		}

		body = make_stmt<while_stmt>(std::move(condition), std::move(body)); 

		if (initializer)
		{
			statement_vec statements;
			statements.push_back(std::move(initializer));
			statements.push_back(std::move(body));
			body = make_stmt<block_stmt>(std::move(statements));
		}

		return body;
	}

	statement_ptr if_statement()
	{
		consume(token_type::LEFT_PAREN, "Expect '(' after 'if'.");

		auto condition = expr();
		consume(token_type::RIGHT_PAREN, "Expection ')' after if condition.");

		statement_ptr then_branch = statement();
		statement_ptr else_branch;

		if (match<token_type::ELSE>())
			else_branch = statement();

		return make_stmt<if_stmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
	}

	statement_ptr print_statement()
	{
		auto value = expr();
		consume(token_type::SEMICOLON, "Expect ';' after value.");

		return make_stmt<print_stmt>(std::move(value));
	}

	statement_ptr while_statement()
	{
		consume(token_type::LEFT_PAREN, "Expect '(' after 'while'.");
		auto condition{expr()};
		consume(token_type::RIGHT_PAREN, "Expect ')' after condition.");

		auto body{statement()};
		return make_stmt<while_stmt>(std::move(condition), std::move(body));
	}

	statement_ptr expression_statement()
	{
		auto value = expr();
		consume(token_type::SEMICOLON, "Expect ';' after expression.");

		return make_stmt<expression_stmt>(std::move(value));
	}

	block_stmt::statements_t block()
	{
		block_stmt::statements_t statements;

		while (!check(token_type::RIGHT_BRACE) && !is_at_end())
			statements.push_back(declaration());

		consume(token_type::RIGHT_BRACE, "Expect '}' after block.");
		return statements;
	}
};

} // namespace lox

