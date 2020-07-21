#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/inst_header.h"
#include "validator.h"

using namespace z3;

void test1() {

  cout << "test 1: branch free programs" << endl;
  int length = 7;

  inst p1[] = {
    inst(IMMED, 5, 4), // immed[a5, 4]
    inst(IMMED, 16, 5), // immed[b0, 5]
    inst(IMMED, 6, 2), // immed[a6, 1]
    inst(ALU, 0, 5, ALU_PLUS, 16), // alu[a0, a5, +, b0] (a0 = 9)
    inst(ALU, 0, 0, ALU_MINUS, 6), // alu[a0, a0, -, a6] (a0 = 7)
    inst(NOP),
    inst(NOP),
  };

  inst p2[] {
    inst(IMMED, 16, 0x0000010a),
    inst(IMMED, 17, 0x00000305),
    inst(ALU, 1, 16, ALU_OR, 17), // a1 = 0x0000030f
    inst(IMMED, 18, 0xabcd0008),
    inst(ALU, 2, 1, ALU_XOR, 18), // a2 = 0xabcd0007
    inst(IMMED, 0, 0),
    inst(ALU, 0, 0, ALU_PLUS_8, 2), // a0 = 7
  };

  inst p3[] {
    inst(IMMED, 0, 13),
    inst(IMMED, 0, 12),
    inst(IMMED, 0, 11),
    inst(IMMED, 0, 10),
    inst(IMMED, 0, 9),
    inst(IMMED, 0, 8),
    inst(IMMED, 0, 7),
  };

  inst p4[] {
    inst(IMMED, 0, 7),
    inst(NOP),
    inst(NOP),
    inst(NOP),
    inst(NOP),
    inst(NOP),
    inst(IMMED, 0, 123456),
  };

  validator vld(p1, length);
  print_test_res(vld.is_equal_to(p1, length, p2, length), "p1 == p2");
  print_test_res(vld.is_equal_to(p1, length, p3, length), "p1 == p3");
  print_test_res(!vld.is_equal_to(p1, length, p4, length), "p1 != p4");
}


void test2() {
  cout << "test 2: counterexample generation" << endl;
  int length = 2;
  
  // output = input * 2
  inst p1[] {
    inst(ALU, 0, 0, ALU_B, 16),
    inst(ALU, 0, 16, ALU_PLUS, 0),
  };

  // output = input + 5
  inst p2[] {
    inst(IMMED, 5, 16),
    inst(ALU, 0, 16, ALU_PLUS, 0),
  };

  validator vld(p1, length);
  inout counterex;
  inout_t input, output;
  input.init(); output.init();
  counterex.set_in_out(input, output);
  print_test_res(!vld.is_equal_to(p1, length, p2, length), "p1 != p2");
  counterex = vld._last_counterex;
  print_test_res(counterex.input.reg != 5 && counterex.output.reg != 10, "counterexample generation");
  cout << "counterexample: input=" << counterex.input.reg << " output=" << counterex.output.reg << endl;
}

void test3() {
  cout << "test 3" << endl;

  inst p1[] {
    inst(IMMED, 0, 101),
    inst(IMMED, 16, 10),
    inst(ALU, 0, 0, ALU_MINUS, 16),
    inst(NOP),
    inst(NOP),
    inst(NOP),
  };

  inst p2[] {
    inst(IMMED, 0, 101),
    inst(IMMED, 16, 10),
    inst(IMMED, 17, 1),
    inst(ALU, 16, 0, ALU_INV_B, 16),
    inst(ALU, 0, 0, ALU_PLUS, 16),
    inst(ALU, 0, 0, ALU_PLUS, 17),
  };

  int length = sizeof(p1) / sizeof(inst);
  validator vld(p1, length);
  bool result = vld.is_equal_to(p1, length, p2, length)
  print_test_res(result, "p1 == p2");
  if (!result) {
    inout counterex = vld._last_counterex;
    cout << "counterexample: input=" << counterex.input.reg << " output=" << counterex.output.reg << endl;
  }
}

int main(int argc, char *argv[]) {
  cout << "=== Program equivalence tests using validator class ===" << endl;
  test1(); 
  test2();
  test3();
  return 0;
}
