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
		if (std::getline(std::cin, line))
		{
			std::cout << std::endl;
			break;
		}

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

	Chunk chunk;

	byte constant = chunk.add_constant(1.2);
	chunk.write(OpCode::OP_CONSTANT, 123);
	chunk.write(constant, 123);
	constant = chunk.add_constant(3.4);
	chunk.write(OpCode::OP_CONSTANT, 123);
	chunk.write(constant, 123);
	chunk.write(OpCode::OP_ADD, 123);

	constant = chunk.add_constant(5.6);
	chunk.write(OpCode::OP_CONSTANT, 123);
	chunk.write(constant, 123);

	chunk.write(OpCode::OP_DIVIDE, 123);
	chunk.write(OpCode::OP_NEGATE, 123);
	chunk.write(OpCode::OP_RETURN, 124);
	disassemble(chunk, "test chunk");
	fmt::print("\n");

	VM vm(std::move(chunk));
	vm.interpret();

	
	return 0;
}

