#include <iostream>
#include "proposals.h"
#include "inst.h"

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
  print_program(instructions, N);
  prog p1(instructions);
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_inst_operand(*p[i-1]);
    print_program(p[i]->inst_list, N);
  }
  for (int i = 1; i < 6; i++) {
    prog::clear_prog(p[i]);
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
  print_program(instructions, N);
  prog p1(instructions);
  prog* p[6];
  p[0] = &p1;
  for (int i = 1; i < 6; i++) {
    p[i] = mod_random_inst(*p[i-1]);
    cout << "Transformed program after " << i << " proposals:" << endl;
    print_program(p[i]->inst_list, N);
  }
  for (int i = 1; i < 6; i++) {
    prog::clear_prog(p[i]);
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
}
