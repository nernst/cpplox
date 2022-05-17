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

		using enum token_type;

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

	inline void log_error(std::ostream& err_stream, token const& tok, std::string_view message)
	{
		auto const& loc = tok.source_location();
		auto [line_no, line_off, line] = loc.get_line();

		err_stream << "in " << loc.where().name() << " (" << line_no << ':' << line_off << "): " << message << '\n';
		err_stream << fmt::format("{:>5} |", line_no) << line << '\n';
		err_stream << "      |";
		for (size_t count = line_off; count; --count)
			err_stream << ' ';
		err_stream << "^\n";
	}

} // namespace lox

