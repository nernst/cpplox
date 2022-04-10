#pragma once

#include <string_view>
#include <variant>
#include "token_type.hpp"

namespace lox {

	class token
	{
	public:
		using literal_t = std::variant<std::string_view, double>;
	
		token(token_type type, std::string_view	lexeme, literal_t literal, std::size_t line)
		: type_(type)
		, lexeme_(lexeme)
		, literal_(literal)
		, line_(line)
		{ }

		token(token_type type, std::string_view lexeme, std::size_t line)
		: token(type, lexeme, literal_t(), line)
		{ }

		token(token const&) = default;
		token(token&&) = default;

		token& operator=(token const&) = default;
		token& operator=(token&&) = default;

		token_type type() const { return type_; }
		std::string_view const& lexeme() const { return lexeme_; }
		literal_t const& literal() const { return literal_; }
		std::size_t line() const { return line_; }

	private:
		token_type type_;
		std::string_view lexeme_;
		literal_t literal_;
		std::size_t line_;

	};

} // namespace lox

