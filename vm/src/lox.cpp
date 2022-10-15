#include "lox/common.hpp"
#include "lox/chunk.hpp"

using namespace lox;

int main(int argc, const char* argv[])
{
	ignore(argc, argv);

	Chunk chunk;

	chunk.write(OpCode::OP_RETURN);
	chunk.disassemble("test chunk");
	
	return 0;
}

