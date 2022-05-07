#pragma once

#include <string_view>
#include <type_traits>
#include <fmt/format.h>
#include "object.hpp"
#include "source_file.hpp"
#include "token_type.hpp"

namespace lox {

	class token
	{
	public:
		token() = default;
	
		token(token_type type, std::string_view	lexeme, object&& literal, location const& loc)
		: type_{type}
		, lexeme_{lexeme}
		, literal_{std::move(literal)}
		, location_{loc}
		{ }

		token(token_type type, std::string_view lexeme, location const& loc)
		: token(type, lexeme, object{}, loc)
		{ }

		token(token const&) = default;
		token(token&&) = default;

		token& operator=(token const&) = default;
		token& operator=(token&&) = default;

		token_type type() const { return type_; }
		std::string_view const& lexeme() const { return lexeme_; }
		object const& literal() const { return literal_; }
		location const& source_location() const { return location_; }
		std::size_t line() const { return source_location().line(); }

	private:
		token_type type_;
		std::string_view lexeme_;
		object literal_;
		location location_;

	};

	inline std::string str(token const& tok)
	{
		return fmt::format(
			"[type={}, lexeme={}, line={}, literal={}]",
			str(tok.type()),
			tok.lexeme(),
			tok.line(),
			tok.literal().str()
		);
	}

} // namespace lox

