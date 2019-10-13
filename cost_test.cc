#include <iostream>
#include "cost.h"

using namespace std;

inst instructions[7] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r0 */
                        inst(), /* nop */
};

inst instructions2[7] = {inst(MOVXC, 2, 4),     /* mov r2, 4 */
                         inst(ADDXY, 0, 2),     /* add r0, r2 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(RETX, 0),         /* ret r0 */
                         inst(), /* nop */
                         inst(), /* nop */
                         inst(), /* nop */
};

inst instructions3[7] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                         inst(ADDXY, 0, 2),  /* add r0, r2 */
                         inst(MOVXC, 3, 15),  /* mov r3, 15  */
                         inst(JMPEQ, 0, 3, 1),  /* if r0 != r3: */
                         inst(RETX, 3),      /* ret r3 */
                         inst(RETX, 0),      /* else ret r0 */
                         inst(), /* nop */
};

inout ex_set[2];

void test1() {
  #define NUM_INTS 6
  unsigned int ints_list[NUM_INTS] = {0, 1, 5, 7, 63, 114};
  for (int i = 0; i < NUM_INTS; i++) {
    cout << i << ": " << endl;
    pop_count_asm(ints_list[i]);
  }
}

void test2() {
  int err_cost = error_cost(ex_set, 2, instructions, instructions);
  cout << "Error cost: " << err_cost << endl;
}

void test3() {
  int err_cost = error_cost(ex_set, 2, instructions, instructions2);
  cout << "Error cost: " << err_cost << endl;
}

void test4() {
  int err_cost = error_cost(ex_set, 2, instructions, instructions3);
  cout << "Error cost: " << err_cost << endl;
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  test2();
  test3();
  test4();
  return 0;
}
