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

	explicit parser(token_vec&& tokens)
	: tokens_{std::move(tokens)}
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

		std::cerr << "in " << loc.where().name() << " (" << line_no << ':' << line_off << "): " << message << '\n';
		std::cerr << fmt::format("{:>5} |", line_no) << line << '\n';
		std::cerr << "      |";
		for (size_t count = line_off; count; --count)
			std::cerr << ' ';
		std::cerr << "^\n";

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
			return equality();
		}
		catch(parse_error const&)
		{
			return expression_ptr{};
		}
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
		if (match<token_type::FALSE, token_type::TRUE, token_type::NIL, token_type::NUMBER, token_type::STRING>())
			return make_expr<literal>(previous().literal());

		if (match<token_type::IDENTIFIER>())
			return make_expr<variable>(previous().lexeme());
		
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
		if (match<token_type::PRINT>())
			return print_statement();

		return expression_statement();
	}

	statement_ptr print_statement()
	{
		auto value = expr();
		consume(token_type::SEMICOLON, "Expect ';' after value.");

		return make_stmt<print_stmt>(std::move(value));
	}

	statement_ptr expression_statement()
	{
		auto value = expr();
		consume(token_type::SEMICOLON, "Expect ';' after expression.");

		return make_stmt<expression_stmt>(std::move(value));
	}
};

} // namespace lox

