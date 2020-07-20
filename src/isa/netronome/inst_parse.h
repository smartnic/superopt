#include <vector>
#include <unordered_map>
#include "inst.h"

// string representations of register names
const vector<string> register_names {
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12", "a13", "a14", "a15",
	"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "b10", "b11", "b12", "b13", "b14", "b15",
};

// string representations of enum OPCODES
const vector<string> opcode_names {
	"nop",
	"immed",
	"alu",
};

// string representations of enum ALU_OPS
const vector<string> alu_op_names {
	"+",
	"+16",
	"+8",
	"+carry",
	// "-carry",
	"-",
	"b-a",
	"b",
	"~b",
	"and",
	"~and",
	"and~",
	"or",
	"xor",
};

// Parses the file `filename` for a program. Returns the program as an inst[] with length `length`.
// Caller should delete[] the program when done.
inst *read_program(const string filename, int& length);