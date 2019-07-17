#include <iostream>
#include "inst.h"

int main(int argc, char *argv[]) {
#define N 7
  /* Add the notion of program input */
  int input = 10;
  if (argc > 1) {
    input = atoi(argv[1]);
  }
  inst instructions[N] = {inst(MOVXC, 1, input), /* mov r1, input */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 1),      /* else ret r1 */
  };
  prog_state ps;

  inst instructions2[5] = {inst(MOVXC, 1, input), /* mov r1, input */
                           inst(MOVXC, 2, 4),     /* mov r2, 4 */
                           inst(ADDXY, 1, 2),     /* add r1, r2 */
                           inst(MAXC, 1, 15),     /* max r1, 15 */
                           inst(RETX, 1),         /* ret r1 */
  };

  inst instructions3[3] = {inst(MOVXC, 1, input), /* mov r1, input */
                           inst(NOP), /* test no-op */
                           inst(RETX, 1), /* ret r1 */
  };

  cout << "Result of full interpretation: " << endl;
  cout << interpret(instructions, N, ps) << endl;
  cout << "Program 2" << endl;
  cout << interpret(instructions2, 5, ps) << endl;
  cout << "Program 3" << endl;
  cout << interpret(instructions3, 3, ps) << endl;
  return 0;
}
