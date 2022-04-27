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
	const char* what() const noexcept override { return message_.data(); }

private:
	token_type type_;
	std::string_view message_;
};


class parser
{
public:

	using expression_ptr = expressions::expression_ptr;

	explicit parser(token_vec&& tokens)
	: tokens_{std::move(tokens)}
	, current_{0}
	, end_{tokens_.size()}
	{ }

	parser(parser const&) = delete;
	parser(parser&&) = default;

	parser& operator=(parser const&) = delete;
	parser& operator=(parser&&) = default;


	expression_ptr parse()
	{
		try
		{
			return expr();
		}
		catch(parse_error const& ex)
		{
			std::cerr << "parse error: " << ex.what() << "\n";
			return expression_ptr{};
		}
	}


private:
	token_vec tokens_;
	std::size_t current_;
	std::size_t end_;

	using expression = expressions::expression;
	using binary_t = expressions::binary;
	using unary_t = expressions::unary;
	using literal_t = expressions::literal;
	using grouping_t = expressions::grouping;

	template<class T, class... Args>
	static expression_ptr make(Args&&... args)
	{ return expression::make<T>(std::forward<Args>(args)...); }


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

		return tokens_[current_];
	}

	bool is_at_end() const { return peek().type() == token_type::END_OF_FILE; }
	token const& peek() const { return tokens_[current_]; }

	token const& previous() const
	{
		assert(current_ != 0);
		return tokens_[current_ - 1];
	}

	parse_error on_error(token const& tok, std::string_view message)
	{
		auto const& loc = tok.source_location();
		auto [line_no, line_off, line] = loc.get_line();

		std::cerr << "in " << loc.where().name() << " (" << line_no << ':' << line_off << "):\n";
		std::cerr << fmt::format("{:>5} |", line_no) << line << '\n';
		std::cerr << "      |";
		for (size_t count = line_off; count; --count)
			std::cerr << ' ';
		std::cerr << "^\n";
		std::cerr << "[line " << line_no << "] Parse Error: " << message << "\n";

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
		return equality();
	}

	expression_ptr equality()
	{
		auto expr = comparison();

		while (match<token_type::BANG_EQUAL, token_type::EQUAL_EQUAL>())
		{
			token op = previous();
			auto right = comparison();
			expr = make<binary_t>(std::move(expr), std::move(op), std::move(right));
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
			expr = make<binary_t>(std::move(expr), std::move(op), std::move(right));
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
			expr = make<binary_t>(std::move(expr), std::move(op), std::move(right));
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
			expr = make<binary_t>(std::move(expr), std::move(op), std::move(right));
		}

		return expr;
	}

	expression_ptr unary()
	{
		if (match<token_type::BANG, token_type::MINUS>())
		{
			token op = previous();
			auto right = unary();
			return make<unary_t>(std::move(op), std::move(right));
		}

		return primary();
	}

	expression_ptr primary()
	{
		if (match<token_type::FALSE, token_type::TRUE, token_type::NIL, token_type::NUMBER, token_type::STRING>())
			return make<literal_t>(previous().literal());
		
		if (match<token_type::LEFT_PAREN>())
		{
			auto e{expr()};
			consume(token_type::RIGHT_PAREN, "Expect '}' after expression.");

			return make<grouping_t>(std::move(e));
		}

		throw on_error(peek(), "Expect expression.");
		return nullptr;
	}
};

} // namespace lox

