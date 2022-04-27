#pragma once

#include <string_view>
#include <type_traits>
#include <fmt/format.h>
#include "literal_holder.hpp"
#include "source_file.hpp"
#include "token_type.hpp"

namespace lox {

	class token
	{
	public:
		using literal_t = literal_holder;
		
		token() = default;
	
		token(token_type type, std::string_view	lexeme, literal_t literal, location const& loc)
		: type_{type}
		, lexeme_{lexeme}
		, literal_{literal}
		, location_{loc}
		{ }

		token(token_type type, std::string_view lexeme, location const& loc)
		: token(type, lexeme, literal_t(), loc)
		{ }

		token(token const&) = default;
		token(token&&) = default;

		token& operator=(token const&) = default;
		token& operator=(token&&) = default;

		token_type type() const { return type_; }
		std::string_view const& lexeme() const { return lexeme_; }
		literal_t const& literal() const { return literal_; }
		location const& source_location() const { return location_; }
		std::size_t line() const { return source_location().line(); }

	private:
		token_type type_;
		std::string_view lexeme_;
		literal_t literal_;
		location location_;

	};

	std::string str(token tok)
	{
		return fmt::format(
			"[type={}, lexeme={}, line={}, literal={}]",
			str(tok.type()),
			tok.lexeme(),
			tok.line(),
			std::visit([](auto&& arg) -> std::string {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, double>) {
						return std::to_string(arg);
					} else if constexpr (std::is_same_v<T, bool>) {
						return arg ? "true" : "false";
					} else if constexpr (std::is_same_v<T, nullptr_t>) {
						return "nil";
					} else if constexpr (std::is_same_v<T, std::string_view>) {
						return std::string(arg);
					}
				},
				tok.literal()
			)
		);
	}

} // namespace lox

