#pragma once

#include "ast_printer.hpp"
#include "expr.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "scanner.hpp"
#include "source_file.hpp"

namespace lox {

	class Lox
	{
	public:

		Lox() {}
		Lox(Lox const&) = delete;
		Lox(Lox&&) = default;

		Lox& operator=(Lox const&) = delete;
		Lox& operator=(Lox&&) = default;

		void run(source& input)
		{
			had_error_ = had_parse_error_ = false;

			auto [had_error, tokens] = scan_source(input);
			if (had_error)
			{
				std::cerr << "error tokenizing input." << std::endl;
				return;
			}

			lox::parser parser{std::move(tokens)};
			auto [had_parse_error, statements] = parser.parse();

			if (!had_parse_error)
			{
				interpreter_.interpret(statements);
			}
			had_error_ = had_error;
			had_parse_error_ = had_parse_error_;
		}
	
	private:
		interpreter interpreter_;
		bool had_error_;
		bool had_parse_error_;
	};

} // namespace lox

