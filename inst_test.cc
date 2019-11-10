#include <iostream>
#include "inst.h"

int main(int argc, char *argv[]) {
  /* Add the notion of program input */
  int input = 10;
  if (argc > 1) {
    input = atoi(argv[1]);
  }
  /* r0 contains the input */
  inst instructions[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 0, 2),  /* add r0, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 0),      /* else ret r0 */
                         };
  prog_state ps;

  inst instructions2[4] = {inst(MOVXC, 2, 4),     /* mov r2, 4 */
                           inst(ADDXY, 0, 2),     /* add r0, r2 */
                           inst(MAXC, 0, 15),     /* max r1, 15 */
                           inst(RETX, 0),         /* ret r0 */
                          };

  inst instructions3[2] = {inst(NOP), /* test no-op */
                           inst(RETX, 0), /* ret r0 */
                          };

  // f(x) = max(x, r1, 10) = max(x, 10)
  inst orig2[7] = {inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                   inst(MOVXC, 0, 0),
                   inst(ADDXY, 0, 1),
                   inst(MOVXC, 1, 10),   // r1 <- 10
                   inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                   inst(MOVXC, 0, 0),
                   inst(ADDXY, 0, 1),
                  };
  cout << interpret(orig2, 7, ps, 11) << endl;
  cout << "Result of full interpretation: " << endl;
  cout << interpret(instructions, 6, ps, input) << endl;
  cout << "Program 2" << endl;
  cout << interpret(instructions2, 4, ps, input) << endl;
  cout << "Program 3" << endl;
  cout << interpret(instructions3, 2, ps, input) << endl;

  inst x = inst(MOVXC, 2, 4);
  inst y = inst(MOVXC, 2, 4);
  inst z = inst(MOVXC, 2, 3);
  inst w = inst(RETX, 3);

  cout << (x == y) << endl;
  cout << (inst(RETX, 3) == inst(RETC, 3)) << endl;
  cout << (inst(RETX, 3) == inst(RETX, 2)) << endl;
  cout << (inst(RETX, 3) == inst(RETX, 3)) << endl;
  cout << "Hashes of mov instructions: " << instHash()(x) << " "
       << instHash()(y) << endl;
  cout << "Hashes of different instructions: " << instHash()(x) << " "
       << instHash()(z) << " " << instHash()(w) << endl;
  return 0;
}
