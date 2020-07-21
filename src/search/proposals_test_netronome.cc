#include <iostream>
#include "../../src/utils.h"
#include "proposals.h"

inst instructions[] {
    inst(IMMED, 5, 4), // immed[a5, 4]
    inst(IMMED, 16, 5), // immed[b0, 5]
    inst(IMMED, 6, 2), // immed[a6, 2]
    inst(ALU, 0, 5, ALU_PLUS, 16), // alu[a0, a5, +, b0] (a0 = 9)
    inst(ALU, 0, 0, ALU_MINUS, 6), // alu[a0, a0, -, a6] (a0 = 7)
    inst(NOP),
    inst(NOP),
};

int N = sizeof(instructions) / sizeof(inst);

void test1() {
  cout << "Test 1: mod_random_inst_operand()" << endl;

  prog p1(instructions);
  p1.print();
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_inst_operand(*p[i - 1]);
    p[i]->print();
  }
  for (int i = 1; i < 6; i++) {
    delete p[i];
  }
}

void test2() {
  cout << "Test 2: mod_random_inst()" << endl;

  prog p1(instructions);
  p1.print();
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_inst(*p[i - 1]);
    cout << "Transformed program after " << i << " proposals:" << endl;
    p[i]->print();
  }

  // Test that if an instruction is changed to another that has fewer operands,
  // tne now-extra operands are set to zero
  bool assert_res = true;
  for (int i = 1; i < 6; i++) {
    for (int j = 0; j < N; j++) {
      inst ins = p[i]->inst_list[j];
      for (int k = ins.get_num_operands(); k < MAX_OP_LEN; k++) {
        bool res = (ins.get_operand(k) == 0);
        if (! res) {
          assert_res = false;
          cout << "in program " << i << ", line " << j << endl;
          cout << "unused " << k << "th operand in ";
          ins.print();
          cout << "is not 0, but " << ins.get_operand(k) << endl;
        }
      }
    }
  }
  print_test_res(assert_res, "set unused operands as 0");

  for (int i = 1; i < 6; i++) {
    delete p[i];
  }
}

void test3() {
  cout << "Test 3: mod_random_k_cont_insts()" << endl;

  prog p1(instructions);
  p1.print();
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_k_cont_insts(*p[i - 1], i);
    cout << "Transformed program after " << i << " proposals:" << endl;
    cout << "(" << i << " continuous instrcution(s) is(are) changed." << ")" << endl;
    p[i]->print();
  }
  for (int i = 1; i < 6; i++) {
    delete p[i];
  }
}

int main(int argc, char const *argv[])
{
  cout << "=== Program proposal tests ===" << endl;
  test1();
  test2();
  test3();
  return 0;
}