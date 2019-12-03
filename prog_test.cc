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
  print_test_res(p1.prog_rel_bit_vec(p2) == rel_bv_prog("1111111"),
                 "prog_rel_bit_vec(prog) 1");
  print_test_res(p1.prog_rel_bit_vec(p3) == rel_bv_prog("1111101"),
                 "prog_rel_bit_vec(prog) 2");
  print_test_res(p1.prog_rel_bit_vec(p4) == rel_bv_prog("0000001"),
                 "prog_rel_bit_vec(prog) 3");

  vector<prog> progs;
  progs.push_back(p4);
  print_test_res(p1.prog_rel_bit_vec(progs) == rel_bv_prog("0000001"),
                 "prog_rel_bit_vec(progs) 1");
  progs.push_back(p3);
  print_test_res(p1.prog_rel_bit_vec(progs) == rel_bv_prog("1111101"),
                 "prog_rel_bit_vec(progs) 2");
  progs.push_back(p2);
  print_test_res(p1.prog_rel_bit_vec(progs) == rel_bv_prog("1111111"),
                 "prog_rel_bit_vec(progs) 3");
}

void test5() {
  cout << endl << "Test 5 " << endl;
  prog p1(instructions);
  string expected_bv_str = string("00010000100010000000") +
                           string("00001000000001000000") +
                           string("00010000110111100000") +
                           string("00110000000001100001") +
                           string("00011000110000000000") +
                           string("00011000000000000000") +
                           string("00000000000000000000");
  print_test_res(p1.prog_abs_bit_vec().to_string() == expected_bv_str, "prog_abs_bit_vec");
}

void test6() {
  cout << endl << "Test 6 " << endl;
  // case 1: no reg used
  inst insts1[2] = {inst(NOP),
                    inst(),
                   };
  // case 2: do not use reg 0
  inst insts2[7] = {inst(MOVXC, 2, 15),
                    inst(ADDXY, 2, 1),
                    inst(ADDXY, 1, 2),
                    inst(JMPGT, 1, 2, 1),
                    inst(RETX, 1),
                    inst(ADDXY, 1, 2),
                    inst(RETC, 11),
                   };
  // case 3: use reg 0, and other regs have been used before reg 0
  inst insts3[7] = {inst(MOVXC, 2, 15),
                    inst(ADDXY, 2, 1),
                    inst(ADDXY, 0, 2),
                    inst(JMPGT, 1, 2, 1),
                    inst(RETX, 1),
                    inst(ADDXY, 1, 2),
                    inst(RETC, 11),
                   };
  // case 4: already canonical
  // the canonical of insts2
  inst insts4[7] = {inst(MOVXC, 0, 15),
                    inst(ADDXY, 0, 1),
                    inst(ADDXY, 1, 0),
                    inst(JMPGT, 1, 0, 1),
                    inst(RETX, 1),
                    inst(ADDXY, 1, 0),
                    inst(RETC, 11),
                   };
  // the canonical of insts3
  inst insts5[7] = {inst(MOVXC, 1, 15),
                    inst(ADDXY, 1, 2),
                    inst(ADDXY, 0, 1),
                    inst(JMPGT, 2, 1, 1),
                    inst(RETX, 2),
                    inst(ADDXY, 2, 1),
                    inst(RETC, 11),
                   };
  // test case 1
  prog p1(insts1);
  prog p2(insts1);
  p1.canonicalize();
  print_test_res(p1 == p2, "canonicalize 1");
  // test case 2
  prog p3(insts2);
  prog p4(insts4);
  p3.canonicalize();
  print_test_res(p3 == p4, "canonicalize 2");
  // test case 3
  prog p5(insts3);
  prog p6(insts5);
  p5.canonicalize();
  print_test_res(p5 == p6, "canonicalize 3");
  // test case 4
  p4.canonicalize();
  p6.canonicalize();
  print_test_res((p4 == p3) && (p6 == p5), "canonicalize 4");
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  return 0;
}
