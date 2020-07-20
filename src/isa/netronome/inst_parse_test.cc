#include <iostream>
#include "inst_parse.h"

int main(int argc, char const *argv[])
{
	cout << "=== Parsing tests for netronome isa ===" << endl;
	cout << opcode_names[NOP] << endl;
	cout << opcode_names[ALU] << endl;

	int length;
	inst* program = read_program("test_programs/lab4.list", length);
	
	cout << "Got length: " << length << endl;
	delete[] program;

	return 0;
}