#pragma once

#include <iostream>
#include "ast_printer.hpp"
#include "expr.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "scanner.hpp"
#include "source_file.hpp"

namespace lox {

	class Lox
	{
	public:

		explicit Lox(
			std::istream* stdin = nullptr, // unowned
			std::ostream* stdout = nullptr, // unowned
			std::ostream* stderr = nullptr // unowned
		)
		: stdin_{stdin == nullptr ? &std::cin : stdin}
		, stdout_{stdout == nullptr ? &std::cout : stdout}
		, stderr_{stderr == nullptr ? &std::cerr : stderr}
		, interpreter_{stdin_, stdout_, stderr_}
		{
			assert(stdin_);
			assert(stdout_);
			assert(stderr_);
		}

		~Lox(){}

		Lox(Lox const&) = delete;
		Lox(Lox&&) = default;

		Lox& operator=(Lox const&) = delete;
		Lox& operator=(Lox&&) = default;

		std::istream& stdin() const { return *stdin_; }
		std::ostream& stdout() const { return *stdout_; }
		std::ostream& stderr() const { return *stderr_; }

		bool had_error() const { return had_error_; }
		bool had_parse_error() const { return had_parse_error_; }
		bool had_runtime_error() const { return had_runtime_error_; }

		void run(source& input)
		{
			had_error_ = had_parse_error_ = had_runtime_error_ = false;

			auto [had_error, tokens] = scan_source(input);
			if (had_error)
			{
				stderr() << "error tokenizing input." << std::endl;
				return;
			}

			lox::parser parser{std::move(tokens)};
			auto [had_parse_error, statements] = parser.parse();
			had_error_ = had_error;
			had_parse_error_ = had_parse_error_;

			if (had_parse_error)
				return;

			resolver res{stderr(), interpreter_};
			res.resolve(statements);

			if (res.had_error())
				return;

			try
			{
				interpreter_.interpret(statements);
			}
			catch(runtime_error const& ex)
			{
				had_runtime_error_ = true;
				stderr() << ex.what() << std::endl;
			}
		}
	
	private:
		std::istream* stdin_;
		std::ostream* stdout_;
		std::ostream* stderr_;
		interpreter interpreter_;
		bool had_error_ = false;
		bool had_parse_error_ = false;
		bool had_runtime_error_ = false;
	};

} // namespace lox

