#include <iostream>
#include <string>
#include <string_view>
#include <sysexits.h>

#include "lox/lox.hpp"


using namespace lox;

void run_prompt(Lox& lox)
{
	std::cout << "lox REPL. Enter EOF to stop." << std::endl;

	for (std::string line; std::cout << "> " << std::flush, std::getline(std::cin, line); )
	{
		string_source ss{"<stdin>", line};
		lox.run(ss);
	}
	std::cout << "stopped." << std::endl;
}

void run_file(Lox& lox, std::string_view path)
{
	file_source file{path};
	lox.run(file);
}


int main(int argc, const char** argv)
{
	Lox lox;
	switch(argc)
	{
		case 1:
			run_prompt(lox);
			break;

		case 2:
			run_file(lox, argv[1]);
			break;

		default:
			std::cerr << "Usage: " << argv[0] << " [script]" << std::endl;
			return EX_USAGE;
	}


	return 0;
}

