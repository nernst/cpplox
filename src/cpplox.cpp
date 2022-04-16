#include <iostream>
#include <string>
#include <string_view>
#include <sysexits.h>
#include "lox/utility.hpp"
#include "lox/source_file.hpp"
#include "lox/scanner.hpp"
#include "lox/expr.hpp"


using namespace lox;

void run(std::string_view source_path, std::string_view source)
{
	scanner<> s{source_path, source};
	s.scan();

	std::cout << "had error? " << s.had_error() << std::endl;
}

void run_prompt()
{
	std::cout << "lox REPL. Enter EOF to stop." << std::endl;

	for (std::string line; std::cout << "> " << std::flush, std::getline(std::cin, line); )
	{
		run("<stdin>", line);
	}
	std::cout << "stopped." << std::endl;
}

void run_file(std::string_view path)
{
	source_file file{path};
	run(path, file.view());
}


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


int main(int argc, const char** argv)
{
#if 0
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

