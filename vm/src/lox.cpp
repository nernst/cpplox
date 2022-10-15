#include "lox/common.hpp"
#include "lox/chunk.hpp"
#include "lox/debug.hpp"

using namespace lox;

int main(int argc, const char* argv[])
{
	ignore(argc, argv);

	Chunk chunk;

	int constant = chunk.add_constant(3.14159);
	chunk.write(OpCode::OP_CONSTANT, 123);
	chunk.write(static_cast<byte>(constant), 123);

	chunk.write(OpCode::OP_RETURN, 124);
	disassemble(chunk, "test chunk");
	
	return 0;
}

