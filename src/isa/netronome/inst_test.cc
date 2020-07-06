#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

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

void test1() {
	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = 7;
	
	cout << "test 1: nop + immed" << endl;
	interpret(output, instructions1, 2, ps, input);
	print_test_res(true, "interpret program 1 completion");
	print_test_res(output == expected, "interpret program 1 correctness");
}

void test2() {
	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = 8;
	
	cout << "test 2: immed + alu add, subtract" << endl;
	interpret(output, instructions2, sizeof(instructions2)/sizeof(inst), ps, input);
	// cout << ps << endl;
	print_test_res(true, "interpret program 2 completion");
	print_test_res(output == expected, "interpret program 2 correctness");
}

void test3() {
	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = 5;
	
	cout << "test 3: immed, alu +16" << endl;
	interpret(output, instructions3, sizeof(instructions3)/sizeof(inst), ps, input);
	cout << ps << endl;
	print_test_res(true, "interpret program 3 completion");
	print_test_res(output == expected, "interpret program 3 correctness");
}

int main(int argc, char *argv[]) {
  cout << "=== Interpretation tests for Netronome ISA ===" << endl;
  test1();
  test2();
  test3();
  return 0;
}
