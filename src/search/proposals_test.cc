#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "proposals.h"

int test1(int input) {
  cout << "Test 1" << endl;
#define N 7
  inst instructions[N] = {inst(MOVXC, 1, input), /* mov r1, input */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 1),      /* else ret r1 */
                         };
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
  return 0;
}

int test2(int input) {
  cout << "Test 2" << endl;
#define N 7
  inst instructions[N] = {inst(MOVXC, 1, input), /* mov r1, input */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 1),      /* else ret r1 */
                         };
  prog p1(instructions);
  p1.print();
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_inst(*p[i - 1]);
    cout << "Transformed program after " << i << " proposals:" << endl;
    p[i]->print();
  }
  bool assert_res = true;
  for (int i = 1; i < 6; i++) {
    for (int j = 0; j < N; j++) {
      inst ins = p[i]->inst_list[j];
      for (int k = ins.get_num_operands(); k < MAX_OP_LEN; k++) {
        bool res = (ins.get_operand(k) == 0);
        if (! res) {
          assert_res = false;
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
  return 0;
}

int test3(int input) {
  cout << "Test 3" << endl;
#define N 7
  inst instructions[N] = {inst(MOVXC, 1, input), /* mov r1, input */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 1),      /* else ret r1 */
                         };
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
  return 0;
}

int main(int argc, char *argv[]) {
  int input = 10;
  if (argc > 1) {
    input = atoi(argv[1]);
  }
  test1(input);
  test2(input);
  test3(input);
}
