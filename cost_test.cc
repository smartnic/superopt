#include <iostream>
#include "cost.h"

using namespace std;

inst instructions[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 1, 2),  /* add r1, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 1),      /* else ret r1 */
};

inst instructions2[4] = {inst(MOVXC, 2, 4),     /* mov r2, 4 */
                         inst(ADDXY, 1, 2),     /* add r1, r2 */
                         inst(MAXC, 1, 15),     /* max r1, 15 */
                         inst(RETX, 1),         /* ret r1 */
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
  int err_cost = error_cost(ex_set, 2, instructions, 6, instructions, 6);
  cout << "Error cost: " << err_cost << endl;
}

void test3() {
  int err_cost = error_cost(ex_set, 2, instructions, 6, instructions2, 4);
  cout << "Error cost: " << err_cost << endl;
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  test2();
  test3();
  return 0;
}
