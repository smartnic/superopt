#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

static void run_test(string name, string description, inst *instructions, size_t len, int64_t answer) {
	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = answer;
	
	cout << name + ": " + description << endl;
	interpret(output, instructions, len, ps, input);
	print_test_res(true, "interpret " + name + " completion");
	print_test_res(output == expected, "interpret " + name + " correctness");
	cout << ps << endl;
}


inst instructions1[] = {
	inst(NOP),
	inst(IMMED, 0, 7),
};

inst instructions2[] = {
	inst(IMMED, 5, 4), // immed[a5, 4]
	inst(IMMED, 16, 5), // immed[b0, 5]
	inst(IMMED, 6, 1), // immed[a5, 1]
	inst(ALU, 0, 5, ALU_PLUS, 16), // alu[a0, a5, +, b0] (a0 = 9)
	inst(ALU, 0, 0, ALU_MINUS, 6), // alu[a0, a0, -, a5] (a0 = 8)
};

inst instructions3[] = {
	inst(IMMED, 2, (4 | 1<<16)), // immed[a2, 0x00010004]
	inst(IMMED, 1, 1), // immed[a1, 1]
	inst(ALU, 0, 1, ALU_PLUS_16, 2), // alu[a0, a1, +16, a2]
};

inst instructions4[] = {
	inst(IMMED, 0, 1), // immed[a0, 1]
	inst(ALU, 0, 0, ALU_INV_B, 0), // alu[a0, a0, ~B, a0]
};

inst instructions5[] = {
	inst(IMMED, 16, 5),
	inst(IMMED, 17, 11),
	inst(ALU, 0, 16, ALU_XOR, 17)
};

int main(int argc, char *argv[]) {
  cout << "=== Interpretation tests for Netronome ISA ===" << endl;
  run_test("Test 1", "nop, immed", instructions1, sizeof(instructions1)/sizeof(inst), 7);
  run_test("Test 2", "immed, alu add", instructions2, sizeof(instructions2)/sizeof(inst), 8);
  run_test("Test 3", "immed, alu +16", instructions3, sizeof(instructions3)/sizeof(inst), 5);
  run_test("Test 4", "alu ~B", instructions4, sizeof(instructions4)/sizeof(inst), -2);
  run_test("Test 5", "alu", instructions5, sizeof(instructions5)/sizeof(inst), 5 ^ 11);
  return 0;
}
