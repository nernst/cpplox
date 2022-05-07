#pragma once

#include <iostream>
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

		explicit Lox(
			std::istream* input = nullptr, // unowned
			std::ostream* output = nullptr, // unowned
			std::ostream* error = nullptr // unowned
		)
		: input_{input ? input : &std::cin}
		, output_{output ? output : &std::cout}
		, error_{error ? error : &std::cerr}
		{
			assert(input_);
			assert(output_);
			assert(error_);
		}

		~Lox(){}

		Lox(Lox const&) = delete;
		Lox(Lox&&) = default;

		Lox& operator=(Lox const&) = delete;
		Lox& operator=(Lox&&) = default;

		std::istream& input() const { assert(input_); return *input_; }
		std::ostream& output() const { assert(output_); return *output_; }
		std::ostream& error() const { assert(error_); return *error_; }

		bool had_error() const { return had_error_; }
		bool had_parse_error() const { return had_parse_error_; }
		bool had_runtime_error() const { return had_runtime_error_; }

		void run(source& input)
		{
			had_error_ = had_parse_error_ = had_runtime_error_ = false;

			auto [had_error, tokens] = scan_source(input);
			if (had_error)
			{
				error() << "error tokenizing input." << std::endl;
				return;
			}

			lox::parser parser{std::move(tokens)};
			auto [had_parse_error, statements] = parser.parse();
			had_error_ = had_error;
			had_parse_error_ = had_parse_error_;

			if (!had_parse_error)
			{
				try
				{
					interpreter_.interpret(statements);
				}
				catch(runtime_error const& ex)
				{
					had_runtime_error_ = true;
					*error_ << ex.what() << std::endl;
				}
			}
		}
	
	private:
		interpreter interpreter_;
		bool had_error_ = false;
		bool had_parse_error_ = false;
		bool had_runtime_error_ = false;
		std::istream* input_;
		std::ostream* output_;
		std::ostream* error_;
	};

} // namespace lox

