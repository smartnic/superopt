#include <bitset>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "prog.h"

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
  cout << "Test 1" << endl;
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
  prog* q = new prog(p);
  p.print();
  q->print();
  delete q;

  return 0;
}

void test4() {
  cout << endl << "Test 4 " << endl;
  prog p1(instructions);
  prog p2(instructions2);
  prog p3(instructions3);
  prog p4(instructions4);
  const int N = MAX_PROG_LEN;
  print_test_res(bitset<N>(p1.to_rel_bv(p2)).to_string() == "1111111",
                 "program to_rel_bv(prog) 1");
  print_test_res(bitset<N>(p1.to_rel_bv(p3)).to_string() == "1111101",
                 "program to_rel_bv(prog) 2");
  print_test_res(bitset<N>(p1.to_rel_bv(p4)).to_string() == "0000001",
                 "program to_rel_bv(prog) 3");

  vector<prog> progs;
  progs.push_back(p4);
  print_test_res(bitset<N>(p1.to_rel_bv(progs)).to_string() == "0000001",
                 "program to_rel_bv(progs) 1");
  progs.push_back(p3);
  print_test_res(bitset<N>(p1.to_rel_bv(progs)).to_string() == "1111101",
                 "program to_rel_bv(progs) 2");
  progs.push_back(p2);
  print_test_res(bitset<N>(p1.to_rel_bv(progs)).to_string() == "1111111",
                 "program to_rel_bv(progs) 3");
}

void test5() {
  cout << endl << "Test 5 " << endl;
  prog p1(instructions);
  string expected_bv_str = string("00010000100010000000") +
                           string("00001000000001000000") +
                           string("00010000110111100000") +
                           string("00111000000001100001") +
                           string("00011000110000000000") +
                           string("00011000000000000000") +
                           string("00000000000000000000");
  vector<int> bv;
  p1.to_abs_bv(bv);
  string str = "";
  const int N = OP_NUM_BITS;
  for (size_t i = 0; i < bv.size(); i++) {
    str += bitset<N>(bv[i]).to_string();
  }
  print_test_res(str == expected_bv_str, "program to_abs_bv");
}

void test6() {
  cout << endl << "Test 6 " << endl;
  // case 1: no reg used
  inst insts1[7] = {inst(NOP),
                    inst(),
                    inst(),
                    inst(),
                    inst(),
                    inst(),
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
  // case 4: need implicit RETX 0 instruction, no reg0 usage, other regs are used
  // case 4.1: no RETs instruction: reg0 cannot be used
  inst insts41[7] = {inst(MOVXC, 2, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
  // case 4.2: RETs instruction with JMP: reg0 cannot be used
  inst insts42[7] = {inst(MOVXC, 2, 15),
                     inst(JMPGT, 1, 2, 1),
                     inst(RETC, 11),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
  // case 5: already canonical
  // the canonical of insts2
  inst insts5[7] = {inst(MOVXC, 0, 15),
                    inst(ADDXY, 0, 1),
                    inst(ADDXY, 1, 0),
                    inst(JMPGT, 1, 0, 1),
                    inst(RETX, 1),
                    inst(ADDXY, 1, 0),
                    inst(RETC, 11),
                   };
  // the canonical of insts3
  inst insts6[7] = {inst(MOVXC, 1, 15),
                    inst(ADDXY, 1, 2),
                    inst(ADDXY, 0, 1),
                    inst(JMPGT, 2, 1, 1),
                    inst(RETX, 2),
                    inst(ADDXY, 2, 1),
                    inst(RETC, 11),
                   };
  // the canonical of insts40
  inst insts71[7] = {inst(MOVXC, 1, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
  // the canonical of insts41
  inst insts72[7] = {inst(MOVXC, 1, 15),
                     inst(JMPGT, 2, 1, 1),
                     inst(RETC, 11),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
  // test case 1
  prog p1(insts1);
  prog p2(insts1);
  p1.canonicalize();
  print_test_res(p1 == p2, "canonicalize 1");
  return;
  // test case 2
  prog p3(insts2);
  prog p4(insts5);
  p3.canonicalize();
  print_test_res(p3 == p4, "canonicalize 2");
  // test case 3
  prog p5(insts3);
  prog p6(insts6);
  p5.canonicalize();
  print_test_res(p5 == p6, "canonicalize 3");
  // test case 4
  prog p7(insts41);
  prog p8(insts71);
  p7.canonicalize();
  print_test_res(p7 == p8, "canonicalize 4.1");
  prog p9(insts42);
  prog p10(insts72);
  p9.canonicalize();
  print_test_res(p9 == p10, "canonicalize 4.2");
  // test case 5
  p4.canonicalize();
  p6.canonicalize();
  p8.canonicalize();
  p10.canonicalize();
  print_test_res((p4 == p3) && (p6 == p5) && (p8 == p7) && (p10 == p9),
                 "canonicalize 5");
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
