#include "prog.h"
#include "utils.h"

using namespace std;

inst instructions[7] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r1 */
                        inst(), /* nop */
};

inst instructions2[7] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                         inst(ADDXY, 0, 2),  /* add r0, r2 */
                         inst(MOVXC, 3, 15),  /* mov r3, 15  */
                         inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                         inst(RETX, 3),      /* ret r3 */
                         inst(RETX, 0),      /* else ret r0 */
                         inst(), /* nop */
};

inst instructions3[7] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                         inst(ADDXY, 0, 2),  /* add r0, r2 */
                         inst(MOVXC, 3, 15),  /* mov r3, 15  */
                         inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                         inst(RETX, 3),      /* ret r3 */
                         inst(RETX, 2),      /* else ret r2 */
                         inst(), /* nop */
};

inst instructions4[7] = {inst(NOP),
                         inst(RETC, 1),
                         inst(), /* nop */
                         inst(), /* nop */
                         inst(), /* nop */
                         inst(), /* nop */
                         inst(), /* nop */
};

int test1() {
  prog p(instructions);
  prog q(p);
  p.print();
  q.print();
  return 0;
}

int test2() {
  cout << "Equality testing:" << endl;
  prog p1(instructions);
  prog p2(instructions);
  prog p3(instructions2);
  prog p4(instructions3);
  prog p5(instructions4);
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

int test3() {
  cout << endl << "Test 3 " << endl;
  prog p(instructions);
  prog* q = prog::make_prog(p);
  p.print();
  q->print();
  prog::clear_prog(q);
  return 0;
}

void test4() {
  cout << endl << "Test 4 " << endl;
  prog p1(instructions);
  prog p2(instructions2);
  prog p3(instructions3);
  prog p4(instructions4);
  print_test_res(p1.prog_bit_vec(p2) == bv_prog("1111111"), "prog bit vector 1");
  print_test_res(p1.prog_bit_vec(p3) == bv_prog("1111101"), "prog bit vector 2");
  print_test_res(p1.prog_bit_vec(p4) == bv_prog("0000001"), "prog bit vector 3");

  vector<prog> progs;
  progs.push_back(p4);
  print_test_res(p1.prog_best_bit_vec(progs) == bv_prog("0000001"),
                 "prog_best_bit_vec 1");
  progs.push_back(p3);
  print_test_res(p1.prog_best_bit_vec(progs) == bv_prog("1111101"),
                 "prog_best_bit_vec 2");
  progs.push_back(p2);
  print_test_res(p1.prog_best_bit_vec(progs) == bv_prog("1111111"),
                 "prog_best_bit_vec 3");
}

int main() {
  test1();
  test2();
  test3();
  test4();
  return 0;
}
