#pragma once

#include <iostream>
#include <string_view>

namespace lox {

	enum class token_type
	{
		// single-character tokens
		LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
		COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

		// one or two character tokens
		BANG, BANG_EQUAL,
		EQUAL, EQUAL_EQUAL,
		GREATER, GREATER_EQUAL,
		LESS, LESS_EQUAL,

		// literals
		IDENTIFIER,
		STRING,
		NUMBER,

		// keywords
		AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
		PRINT, RETURN, SUPER, TRUE, VAR, WHILE, 

		END_OF_FILE
	};

	inline std::string_view str(token_type type)
	{
#define TOKEN_CASE(ident) case token_type::ident: return std::string_view(#ident)

		switch(type)
		{
			TOKEN_CASE(LEFT_PAREN);
			TOKEN_CASE(RIGHT_PAREN);
			TOKEN_CASE(LEFT_BRACE);
			TOKEN_CASE(RIGHT_BRACE);
			TOKEN_CASE(COMMA);
			TOKEN_CASE(DOT);
			TOKEN_CASE(MINUS);
			TOKEN_CASE(PLUS);
			TOKEN_CASE(SEMICOLON);
			TOKEN_CASE(SLASH);
			TOKEN_CASE(STAR);
			TOKEN_CASE(BANG);
			TOKEN_CASE(BANG_EQUAL);
			TOKEN_CASE(EQUAL);
			TOKEN_CASE(EQUAL_EQUAL);
			TOKEN_CASE(GREATER);
			TOKEN_CASE(GREATER_EQUAL);
			TOKEN_CASE(LESS);
			TOKEN_CASE(LESS_EQUAL);
			TOKEN_CASE(IDENTIFIER);
			TOKEN_CASE(STRING);
			TOKEN_CASE(NUMBER);
			TOKEN_CASE(AND);
			TOKEN_CASE(CLASS);
			TOKEN_CASE(ELSE);
			TOKEN_CASE(FALSE);
			TOKEN_CASE(FUN);
			TOKEN_CASE(FOR);
			TOKEN_CASE(IF);
			TOKEN_CASE(NIL);
			TOKEN_CASE(OR);
			TOKEN_CASE(PRINT);
			TOKEN_CASE(RETURN);
			TOKEN_CASE(SUPER);
			TOKEN_CASE(TRUE);
			TOKEN_CASE(VAR);
			TOKEN_CASE(WHILE);
			TOKEN_CASE(END_OF_FILE);

		default:
			return "<INVALID TOKEN>";
		}
#undef TOKEN_CASE
	}

	std::ostream& operator<<(std::ostream& os, token_type type)
	{
		return os << str(type);
	}

} // namespace lox

