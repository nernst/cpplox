#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>
#include <fmt/format.h>

#define NO_FROM_CHARS

#ifdef NO_FROM_CHARS
#include <boost/lexical_cast.hpp>
#else
#include <charconv>
#include <system_error>
#endif

#include "token.hpp"
#include "utility.hpp"

namespace lox {

	double double_from_chars(std::string_view sv)
	{
		double val;
#ifdef NO_FROM_CHARS
		val = boost::lexical_cast<double>(sv);
#else
		auto [ptr, ec] {std::from_chars(std::begin(sv), std::end(sv), val) };
		assert(ec == std::errc()); // TODO: Probably should never happen, but sanity check?
#endif
		return val;
	}

	struct scanner_error_handler
	{
		void operator()(
			std::string_view const& source_path, 
			std::size_t offset, 
			std::size_t line_number,
			std::string_view line,
			std::size_t line_offset,
			std::string_view where,
			std::string_view message
			)
		{
			ignore_unused(offset);

			std::cerr << "in " << source_path << " (" << line_number << ':' << line_offset << "):\n";
			std::cerr << fmt::format("{:>5} |", line_number) << line << '\n';
			std::cerr << "      |";
			for (size_t count = line_offset; count; --count)
				std::cerr << ' ';
			std::cerr << "^\n";
			std::cerr << "[line " << line_number << "] Error" << where << ": " << message << "\n\n";
		}
	};

	struct token_handler
	{
		void operator()(token&& tok)
		{
			std::cout << str(tok) << '\n';
		}
	};


	template<
		typename TokenSink = token_handler,
		typename ErrorHandler = scanner_error_handler
	>
	class scanner
	{
	public:

		explicit scanner(
			std::string_view source_path,
			std::string_view source,
			TokenSink&& token_sink = TokenSink(),
			ErrorHandler&& error_handler = ErrorHandler()
		)
		: source_path_{source_path}
		, source_{source}
		, token_sink_{std::forward<TokenSink>(token_sink)}
		, error_handler_{std::forward<ErrorHandler>(error_handler)}
		, current_{0}
		, start_{0}
		, end_{source.size()}
		, line_{1}
		, had_error_{false}
		{
			lines_.push_back(0);
		}

		void scan();

		std::size_t line_number(std::size_t offset) const {
			assert(offset <= end_);

			auto i = std::lower_bound(std::cbegin(lines_), std::cend(lines_), offset);
			return std::distance(std::cbegin(lines_), i);
		}

		bool is_at_end() const { return current_ >= end_; }

		bool had_error() const { return had_error_; }

	private:
		std::string_view source_path_;
		std::string_view source_;
		TokenSink token_sink_;
		ErrorHandler error_handler_;
		std::vector<std::size_t> lines_;
		std::size_t current_;
		std::size_t start_;
		std::size_t end_;
		std::size_t line_;
		bool had_error_;

		std::string_view current_line()
		{
			assert(!lines_.empty());
			std::size_t start{lines_.back()};
			std::size_t end{start};
			while (end < end_ && source_[end] != '\n')
				++end;

			return std::string_view{source_.data() + start, end - start};
		}

		void add_token(token_type type)
		{
			token_sink_(
				token{type, std::string_view{source_.data() + start_, current_ - start_}, line_}
			);
		}

		void add_token(token_type type, token::literal_t literal)
		{
			token_sink_(
				token{type, std::string_view{source_.data() + start_, current_ - start_}, literal, line_}
			);
		}

		char advance()
		{
			assert(!is_at_end());
			return source_[current_++];
		}

		char peek()
		{
			return is_at_end() ? '\0' : source_[current_];
		}

		char peek_next()
		{
			if (current_ + 1 > end_)
				return '\0';
			return source_[current_ + 1];
		}

		bool match(char expected)
		{
			if (is_at_end() || source_[current_] != expected)
				return false;

			++current_;
			return true;
		}

		bool is_digit(char c)
		{
			return '0' <= c && c <= '9';
		}

		bool is_alpha(char c)
		{
			return ('a' <= c && c <= 'z')
				|| ('A' <= c && c <= 'Z' )
				|| c == '_';
		}

		bool is_alpha_numeric(char c)
		{
			return is_digit(c) || is_alpha(c);
		}


		void new_line()
		{
			++line_;
			lines_.push_back(current_);
		}

		void string()
		{
			while (peek() != '"' && !is_at_end())
			{
				if (peek() == '\n')
					new_line();

				advance();
			}

			if (is_at_end())
			{
				error("Unterminated string.");
				return;
			}

			advance(); // closing quote

			// Value without enclosing quotes
			auto sv = source_.substr(
				start_ + 1,
				current_ - (start_ + 1) - 1
			);

			add_token(token_type::STRING, sv);
		}

		void number()
		{
			while (is_digit(peek()) && !is_at_end())
				advance();

			if (peek() == '.' && is_digit(peek_next()))
			{
				advance(); // consume '.'
				while (is_digit(peek()) && !is_at_end())
					advance();
			}

			double val{double_from_chars(
				source_.substr(
					start_,
					current_ - start_
			))};
			add_token(token_type::NUMBER, val);
		}

		void identifier()
		{
			while (is_alpha_numeric(peek()))
				advance();

			add_token(token_type::IDENTIFIER);
		}

		void scan_token()
		{
			char c{advance()};
			switch(c)
			{
				case ' ': case '\t': case '\r':
					// ignore whitespace
					break;
				
				case '\n':
					new_line();
					break;

				case '(': add_token(token_type::LEFT_PAREN); break;
				case ')': add_token(token_type::RIGHT_PAREN); break;
				case '{': add_token(token_type::LEFT_BRACE); break;
				case '}': add_token(token_type::RIGHT_BRACE); break;
				case ',': add_token(token_type::COMMA); break;
				case '.': add_token(token_type::DOT); break;
				case '-': add_token(token_type::MINUS); break;
				case '+': add_token(token_type::PLUS); break;
				case ';': add_token(token_type::SEMICOLON); break;
				case '*': add_token(token_type::STAR); break;

				case '!': add_token(match('=') ? token_type::BANG_EQUAL : token_type::BANG); break;
				case '=': add_token(match('=') ? token_type::EQUAL_EQUAL : token_type::EQUAL); break;
				case '<': add_token(match('=') ? token_type::LESS_EQUAL : token_type::LESS); break;
				case '>': add_token(match('=') ? token_type::GREATER_EQUAL : token_type::GREATER); break;

				case '/':
					if (match('/'))
					{
						// comment, ignore to end of line
						while (peek() != '\n' && !is_at_end())
							advance();
					}
					else
						add_token(token_type::SLASH);

					break;

				case '"':
					string();
					break;

				default:
					if (is_digit(c))
					{
						number();
					} else if (is_alpha(c))
					{
						identifier();
					}
					else
					{
						auto msg = fmt::format("Unexpected character: {}", c);
						error(msg);
					}
			}
		}

		void error(std::string_view message)
		{
			assert(!lines_.empty());

			had_error_ = true;
			error_handler_(
				source_path_,
				current_,
				line_number(current_),
				current_line(),
				current_ - 1 - lines_.back(),
				"",
				message
			);
		}

	};

	template<typename TokenSink, typename ErrorHandler>
	inline void scanner<TokenSink, ErrorHandler>::scan()
	{
		while (!is_at_end())
		{
			start_ = current_;
			scan_token();
		}

		// force start_ to end_ so end of file lexeme is blank.
		start_ = end_;
		add_token(token_type::END_OF_FILE);
	}


} // namespace lox

