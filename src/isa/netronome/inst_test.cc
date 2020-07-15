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
	inst(IMMED, 6, 1), // immed[a6, 1]
	inst(ALU, 0, 5, ALU_PLUS, 16), // alu[a0, a5, +, b0] (a0 = 9)
	inst(ALU, 0, 0, ALU_MINUS, 6), // alu[a0, a0, -, a6] (a0 = 8)
};

inst instructions2b[] = {
	inst(IMMED, 16, 5), // immed[b0, 5]
	inst(IMMED, 6, 1), // immed[a5, 1]
	inst(ALU, 0, 16, ALU_MINUS, 6), // alu[a0, b0, -, a5] (a0 = 4)	
	// inst(ALU, 0, 6, ALU_B_MINUS_A, 16),
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

static void test6() {
	inst instructions[] {
		inst(IMMED, 1, 0xffffffff),
		inst(IMMED, 2, 2),
		inst(ALU, 0, 1, ALU_PLUS, 2),
		// inst(ALU, 16, 16, ALU_B, 16)
	};
	int len = sizeof(instructions) / sizeof(inst);

	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = 1;
	
	cout << "Test 6: testing carry flag" << endl;
	interpret(output, instructions, len, ps, input);
	cout << ps << endl;
	print_test_res(output == expected, "correct lower 32 bits");
	print_test_res(ps._unsigned_carry == 1, "correct carry");
}

static void test6b() {
	inst instructions[] {
		inst(IMMED, 1, 0xffffffff), // a1 = 0xffffffff
		inst(IMMED, 2, 0xffffffff), // a2 = 0xffffffff
		inst(IMMED, 3, 0x3), // a3 = 3
		inst(ALU, 0, 1, ALU_PLUS, 2), // a0 = 0xffffffff + 0xffffffff = 0xfffffffe, carry = 1
		inst(ALU, 0, 0, ALU_PLUS_CARRY, 3), // a0 = 0xfffffffe + 3 + 1 = 2, carry = 1
		// inst(ALU, 0, 0, ALU_PLUS_CARRY, 4), // a0 = 2 + 0 + 1
	};
	int len = sizeof(instructions) / sizeof(inst);

	prog_state ps;
	inout_t input, output, expected;
	input.init(); output.init(); expected.init();
	expected.reg = 3;
	// expected.reg = 0x0661f99e;
	
	cout << "Test 6b: ALU_PLUS_CARRY" << endl;
	interpret(output, instructions, len, ps, input);
	cout << ps << endl;
	print_test_res(output == expected, "correct lower 32 bits");
	print_test_res(ps._unsigned_carry == 0, "correct carry");
}

int main(int argc, char *argv[]) {
  cout << "=== Interpretation tests for Netronome ISA ===" << endl;
  // run_test("Test 1", "nop, immed", instructions1, sizeof(instructions1)/sizeof(inst), 7);
  // run_test("Test 2", "immed, alu add", instructions2, sizeof(instructions2)/sizeof(inst), 8);
  run_test("Test 2b", "alu subtract", instructions2b, sizeof(instructions2b)/sizeof(inst), 4);
  // run_test("Test 3", "immed, alu +16", instructions3, sizeof(instructions3)/sizeof(inst), 5);
  // run_test("Test 4", "alu ~B", instructions4, sizeof(instructions4)/sizeof(inst), -2);
  // run_test("Test 5", "alu", instructions5, sizeof(instructions5)/sizeof(inst), 5 ^ 11);
  test6();
  test6b();
  return 0;
}
