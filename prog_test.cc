#include "prog.h"

using namespace std;

inst instructions[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 1, 2),  /* add r1, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 1),      /* else ret r1 */
};

inst instructions2[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                         inst(ADDXY, 1, 2),  /* add r1, r2 */
                         inst(MOVXC, 3, 15),  /* mov r3, 15  */
                         inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                         inst(RETX, 3),      /* ret r3 */
                         inst(RETX, 1),      /* else ret r1 */
};

inst instructions3[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                         inst(ADDXY, 1, 2),  /* add r1, r2 */
                         inst(MOVXC, 3, 15),  /* mov r3, 15  */
                         inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                         inst(RETX, 3),      /* ret r3 */
                         inst(RETX, 2),      /* else ret r2 */
};

inst instructions4[2] = {inst(NOP),
                         inst(RETC, 1),
};

int test1() {
  prog p(instructions, 6);
  prog q(p);
  p.print();
  q.print();
  return 0;
}

int test2() {
  cout << "Equality testing:" << endl;
  prog p1(instructions, 6);
  prog p2(instructions, 6);
  prog p3(instructions2, 6);
  prog p4(instructions3, 6);
  prog p5(instructions4, 2);
  cout << (p1 == p1) << endl;
  cout << (p1 == p2) << endl;
  cout << (p1 == p3) << endl;
  cout << (p1 == p4) << endl;
  cout << (p1 == p5) << endl;
  cout << (p2 == p3) << endl;
  cout << "Hash values:" << endl;
  cout << progHash()(p1) << " "
       << progHash()(p2) << " "
       << progHash()(p3) << " "
       << progHash()(p4) << " "
       << progHash()(p5) << endl;
  return 0;
}

int main() {
  test1();
  test2();
  return 0;
}
