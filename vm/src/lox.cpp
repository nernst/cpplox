#include "lox/common.hpp"
#include "lox/chunk.hpp"
#include "lox/debug.hpp"
#include "lox/vm.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <fmt/format.h>

using namespace lox;

void repl(VM& vm)
{
	std::string line;

	while (true)
	{
		fmt::print("> ");

		std::getline(std::cin, line);
		std::cout << std::endl;
		if (!std::cin)
			break;

		if (line.empty())
			continue;

		vm.interpret(line);
	}
}

std::string read_file(std::string const& name)
{
	std::ifstream infile{name};
	std::stringstream buffer;
	buffer << infile.rdbuf();
	return buffer.str();

}

void run_file(VM& vm, std::string const& name)
{
	auto source{read_file(name)};
	auto result = vm.interpret(source);
	switch(result)
	{
		case VM::Result::COMPILE_ERROR: exit(65);
		case VM::Result::RUNTIME_ERROR: exit(70);
		default: break;
	}
}

int main(int argc, const char* argv[])
{
	ignore(argc, argv);

	VM vm;

	if (argc == 1)
		repl(vm);
	else if (argc == 2)
		run_file(vm, argv[1]);
	else
	{
		fmt::print(stderr, "Usage: lox [path]\n");
		exit(64);
	}
	
	return 0;
}

