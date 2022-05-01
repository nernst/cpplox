#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <fmt/format.h>

#define NO_FROM_CHARS

#ifdef NO_FROM_CHARS
#include <boost/lexical_cast.hpp>
#else
#include <charconv>
#include <system_error>
#endif

#include "source_file.hpp"
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
			source const& s,
			std::size_t offset, 
			std::string_view where,
			std::string_view message
			)
		{
			ignore_unused(offset);

			auto [line_no, line_off, line] = s.get_line(offset);

			std::cerr << "in " << s.name() << " (" << line_no << ':' << line_off << "):\n";
			std::cerr << fmt::format("{:>5} |", line_no) << line << '\n';
			std::cerr << "      |";
			for (size_t count = line_off; count; --count)
				std::cerr << ' ';
			std::cerr << "^\n";
			std::cerr << "[line " << line_no << "] Error" << where << ": " << message << "\n\n";
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

		using keyword_map = std::unordered_map<std::string_view, token_type>;

		static keyword_map const& keywords()
		{
			static const keyword_map keywords{
				{"and", token_type::AND},
				{"class", token_type::CLASS},
				{"else", token_type::ELSE},
				{"false", token_type::FALSE},
				{"for", token_type::FOR},
				{"fun", token_type::FUN},
				{"if", token_type::IF},
				{"nil", token_type::NIL},
				{"or", token_type::OR},
				{"print", token_type::PRINT},
				{"return", token_type::RETURN},
				{"super", token_type::SUPER},
				{"this", token_type::THIS},
				{"true", token_type::TRUE},
				{"var", token_type::VAR},
				{"while", token_type::WHILE},
			};

			return keywords;
		}

		explicit scanner(
			source& source,
			TokenSink&& token_sink = TokenSink(),
			ErrorHandler&& error_handler = ErrorHandler()
		)
		: source_{source}
		, token_sink_{std::forward<TokenSink>(token_sink)}
		, error_handler_{std::forward<ErrorHandler>(error_handler)}
		, current_{0}
		, start_{0}
		, end_{source.size()}
		, line_{1}
		, had_error_{false}
		{ }

		void scan();

		bool is_at_end() const { return current_ >= end_; }

		bool had_error() const { return had_error_; }

	private:
		source& source_;
		TokenSink token_sink_;
		ErrorHandler error_handler_;
		std::size_t current_;
		std::size_t start_;
		std::size_t end_;
		std::size_t line_;
		bool had_error_;

		void add_token(token_type type)
		{
			token_sink_(
				token{type, std::string_view{source_.cbegin() + start_, current_ - start_}, location{source_, start_}}
			);
		}

		void add_token(token_type type, object&& literal)
		{
			token_sink_(
				token{type, std::string_view{source_.cbegin() + start_, current_ - start_}, std::move(literal), location{source_, start_}}
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

		std::string_view current_lexeme() const {
			return std::string_view{
				source_.cbegin() + start_,
				current_ - start_
			};
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
			source_.add_line(current_);
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

			add_token(token_type::STRING, object{sv});
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
			add_token(token_type::NUMBER, object{val});
		}

		void identifier()
		{
			while (is_alpha_numeric(peek()))
				advance();

			auto&& kws = keywords();

			auto id = current_lexeme();
			auto i = kws.find(id);
			auto type = i == kws.end() ? token_type::IDENTIFIER : i->second;

			add_token(type);
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
						number();
					else if (is_alpha(c))
						identifier();
					else
					{
						auto msg = fmt::format("Unexpected character: {}", c);
						error(msg);
					}
			}
		}

		void error(std::string_view message)
		{
			had_error_ = true;
			error_handler_(
				source_,
				current_,
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
		source_.add_line(start_);
		start_ = end_;
		add_token(token_type::END_OF_FILE);
	}


	std::tuple<bool, std::vector<token>> scan_source(source& input)
	{
		std::vector<token> tokens;

		auto handler = [&](token&& tok) { tokens.push_back(std::move(tok)); };
		scanner<decltype(handler)> s{input, std::move(handler)};
		s.scan();
		return std::make_tuple(s.had_error(), tokens); 
	}


} // namespace lox

