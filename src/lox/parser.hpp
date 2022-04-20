#pragma once

#include <vector>
#include <exception>
#include "expr.hpp"
#include "token.hpp"

namespace lox {

using token_vec = std::vector<token>;

class parse_error : public std::exception
{
public:

	explicit parse_error(token_type type, std::string_view message)
	: type_{type}
	, message_{message}
	{ }

	parse_error(parse_error const&) = default;
	parse_error(parse_error&&) = default;

	parse_error& operator=(parse_error const&) = default;
	parse_error& operator=(parse_error&&) = default;

	token_type type() const { return type_; }
	const char* what() const override { return message_.data(); }

private:
	token_type type_;
	std::string_view message_;
};

class parser
{
public:

	using expressions::expression_ptr;

	explicit(token_vec&& tokens)
	: tokens_{std::move(tokens)}
	, current_{0}
	, end_{tokens_.size()}
	{ }


private:
	token_vec tokens_;
	std::size_t current_;
	std::size_t end_;

	bool match(token_type... types)
	{
		token_type to_match[sizeof(types...)] = {types...};

		for (auto type : to_match)
		{
			if (check(type))
			{
				advance();
				return true
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

		return tokens_[current_];
	}

	bool is_at_end() const { return peek().type() == token_type::END_OF_FILE; }
	token const& peek() const { return tokens_[current_]; }

	token const& previous() const
	{
		assert(current_ != 0);
		return tokens[current_ - 1];
	}

	parse_error on_error(tyken_type type, std::string_view message)
	{
		// log error
		return parse_error{type, message};
	}


	token const& consume(token_type type, std::string_view message)
	{
		if (check(type))
			return advance();

		on_error(type, message);
	}

	
	expression_ptr expr()
	{
		return equality();
	}

	expression_ptr equality()
	{
		auto expr = comparison();

		while (match(token_type::BANG_EQUAL, token_type::EQUAL_EQUAL))
		{
			token const& op = previous();
			auto right = comparison();
			expr = expresion::make<binary>(expr, op, right);
		}

		return expr;
	}

	expression_ptr comparison()
	{
		auto expr = term();

		while (match(token_type::GREATER, token_type::GREATER_EQUAL, token_type::LESS, token_type::LESS_EQUAL))
		{
			token const& op = previous();
			auto right = term();
			expr = expression::make<binary>(expr, op, right);
		}

		return expr;
	}

	expression_ptr term()
	{
		auto expr = factor();

		while (match(token_type::MINUS, token_type::PLUS))
		{
			token const& op = previous();
			auto right = factor();
			expr = expr::make<binary>(expr, op, right);
		}

		return expr;
	}

	expression_ptr factor()
	{
		auto expr = unary();

		while (match(token_type::SLASH, token_type::STAR))
		{
			token const& op = previous();
			auto right = unary();
			expr = expr::make<binary>(expr, op, right);
		}

		return expr;
	}

	expression_ptr unary()
	{
		if (match(token_type::BANG, token_type::MINUS))
		{
			token const& op = previous();
			auto right = unary();
			return expr::make<unary>(op, right);
		}

		return primary();
	}

	expression_ptr primary()
	{
		if (match(token_type::FALSE, token_typer::TRUE, token_type::NIL, token::type::NUMBER, token_type::STRING))
			return expression::make<literal>(previous().literal());
		
		if (match(token_type::LEFT_PAREN)
		{
			auto expr = expression();
			consume(token_type::RIGHT_PAREN, "Expect ')' after expression.");
		}

		assert(false); // TODO: error handling!
	}
};

} // namespace lox

