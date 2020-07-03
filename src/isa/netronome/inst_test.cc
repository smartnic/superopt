#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

inst instructions1[] = {
	inst(NOP),
	inst(IMMED, 0, 7),
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

int main(int argc, char *argv[]) {
  cout << "=== Interpretation tests for Netronome ISA ===" << endl;
  test1();
  return 0;
}
