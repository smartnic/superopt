#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "inst_parse.h"

inst *read_program(const string filename, int& length) {
	
	// Open input file
	ifstream input_file;
	input_file.open(filename);

	if (!input_file.is_open()) {
		cout << "Unable to open file " << filename << endl;
		length = 0;
		return NULL;
	}

	// Create maps of string -> int constants
	unordered_map<string, int> register_names_map;
	for (int i = 0; i < register_names.size(); i++) {
		register_names_map[register_names[i]] = i;
	}

	unordered_map<string, int> opcode_names_map;
	for (int i = 0; i < opcode_names.size(); i++) {
		opcode_names_map[opcode_names[i]] = i;
	}

	unordered_map<string, int> alu_op_names_map;
	for (int i = 0; i < alu_op_names.size(); i++) {
		alu_op_names_map[alu_op_names[i]] = i;
	}

	// Read file and count number of lines
	// (for now, assume number of lines == number of instructions)
	vector<string> lines;
	string line;
	int length_found = 0;
	while (getline(input_file, line)) {
		lines.push_back(line);
		length_found++;
	}

	inst* program = new inst[1];
	program[0] = inst(IMMED, 0, 999);
	length = length_found;
	return program;
}