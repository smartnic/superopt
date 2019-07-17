#include <iostream>
#include "proposals.h"

int test1(int input) {
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
  inst* prog = instructions;
  inst* new_prog;
  for (int i = 0; i < 5; i++) {
    new_prog = mod_random_inst_operand(prog, N);
    print_program(new_prog, N);
    prog = new_prog;
  }
  return 0;
}

int test2(int input) {
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
  inst* prog = instructions;
  inst* new_prog;
  for (int i = 0; i < 5; i++) {
    new_prog = mod_random_inst(prog, N);
    cout << "Transformed program after " << i << " proposals:" << endl;
    print_program(new_prog, N);
    prog = new_prog;
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
