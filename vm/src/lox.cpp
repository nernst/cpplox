#include "lox/common.hpp"
#include "lox/chunk.hpp"
#include "lox/debug.hpp"
#include "lox/vm.hpp"

#include <fmt/format.h>

using namespace lox;

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

