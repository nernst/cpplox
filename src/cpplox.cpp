#include <iostream>
#include <string>
#include <string_view>
#include <sysexits.h>
#include "lox/utility.hpp"
#include "lox/source_file.hpp"
#include "lox/scanner.hpp"


using namespace lox;

void run(std::string_view source_path, std::string_view source)
{
	scanner<> s{source_path, source};

	s.scan([](token&& token){
		std::cout << "[type=" << token.type() << ", lexeme=" << token.lexeme() << ", line=" << token.line() << "]\n";
	});

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


int main(int argc, const char** argv)
{
	lox::ignore_unused(argc, argv);

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


	return 0;
}

