#include <iostream>
#include <string>
#include <string_view>
#include <sysexits.h>

#include "lox/ast_printer.hpp"
#include "lox/expr.hpp"
#include "lox/interpreter.hpp"
#include "lox/parser.hpp"
#include "lox/scanner.hpp"
#include "lox/source_file.hpp"
#include "lox/utility.hpp"


using namespace lox;

// #define TEST_EXPRESSION

void run(source& input)
{
	auto [had_error, tokens] = scan_source(input);
	if (had_error)
	{
		std::cerr << "error tokenizing input." << std::endl;
		return;
	}

	lox::parser parser{std::move(tokens)};
	auto expr{parser.parse()};

	if (expr != nullptr)
#if 0
		std::cout << ast_printer::print(*expr) << std::endl;
#else
		std::cout << str(interpreter::evaluate(*expr)) << std::endl;	
#endif
	else
		std::cerr << "error parsing input." << std::endl;
}

void run_prompt()
{
	std::cout << "lox REPL. Enter EOF to stop." << std::endl;

	for (std::string line; std::cout << "> " << std::flush, std::getline(std::cin, line); )
	{
		string_source ss{"<stdin>", line};
		run(ss);
	}
	std::cout << "stopped." << std::endl;
}

void run_file(std::string_view path)
{
	file_source file{path};
	run(file);
}


#ifdef TEXT_EXPRESSION
void test_expression()
{
	using namespace lox::expressions;

	auto expr = expression::make<binary>(
		expression::make<unary>(
			token{token_type::MINUS, "-"},
			expression::make<literal>(123)
		),
		token{token_type::STAR, "*"},
		expression::make<grouping>(expression::make<literal>(45.67))
	);

	ast_printer printer;
	expr->accept(printer);
	std::cout << printer.result() << std::endl;	
}
#endif


int main(int argc, const char** argv)
{
#ifdef TEST_EXPRESSION
	lox::ignore_unused(argc, argv);
	test_expression();
#else

	switch(argc)
	{
		case 1:
			run_prompt();
			break;

		case 2:
			run_file(argv[1]);
			break;

		default:
			std::cerr << "Usage: " << argv[0] << " [script]" << std::endl;
			return EX_USAGE;
	}
#endif


	return 0;
}

