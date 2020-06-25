#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

inst instructions1[] = {
	inst(NOP),
};

void test1() {
	prog_state ps;
	inout_t input, output;
	input.init(); output.init();
	
	cout << "test 1: just a nop :)" << endl;
	interpret(output, instructions1, 1, ps, input);
	print_test_res(true, "interpret program 1 completion");
	print_test_res(output == input, "interpret program 1 correctness");

}

int main(int argc, char *argv[]) {
  cout << "=== Interpretation tests for Netronome ISA ===" << endl;
  test1();
  return 0;
}
