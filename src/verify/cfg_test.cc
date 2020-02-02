#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "cfg.h"

using namespace std;

void test1() {
  cout << "Test1: cfg check" << endl;
  inst instructions1[6] = {inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
                           inst(ADDXY, 0, 1),     // add r0, r1
                           inst(MOVXC, 2, 15),    // mov r2, 15
                           inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
                           inst(RETX, 2),         // ret r2
                           inst(RETX, 0),         // else ret r0
                          };
  string expected;
  graph g1(instructions1, 6);
  expected = "nodes:0,0 1,1 2,3 4,4 5,5 edges: 0:;1,2, 1:0,;2, 2:1,0,;3,4, 3:2,; 4:2,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");

  inst instructions2[6] = {inst(MOVXC, 1, 4),     // mov r0, 4
                           inst(JMPGT, 0, 2, 3),  // if r0 <= r2:
                           inst(MOVXC, 2, 15),    // mov r2, 15
                           inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
                           inst(RETX, 2),         // ret r2
                           inst(RETX, 0),         // else ret r0
                          };
  graph g2(instructions2, 6);
  expected = "nodes:0,1 2,3 4,4 5,5 edges: 0:;1,3, 1:0,;2,3, 2:1,; 3:1,0,;";
  print_test_res(g2.graph_to_str() == expected, "program 2");

  inst instructions3[7] = {inst(MOVXC, 1, 4),     // mov r1, 4
                           inst(JMPGT, 0, 2, 3),  // if r0 <= r3:
                           inst(RETX, 0),         // else ret r0
                           inst(MOVXC, 2, 15),    // mov r2, 15
                           inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
                           inst(RETX, 2),         // ret r2
                           inst(RETX, 0),         // else ret r0
                          };
  graph g3(instructions3, 7);
  expected = "nodes:0,1 2,2 5,5 edges: 0:;1,2, 1:0,; 2:0,;";
  print_test_res(g3.graph_to_str() == expected, "program 3");

  inst instructions4[4] = {inst(JMPGT, 0, 2, 1),     // 0 JMP to inst 2
                           inst(RETX, 2),            // 1 END
                           inst(JMPGT, 0, 2, -2),    // 2 JMP to inst 1
                           inst(RETX, 2),            // 3 END
                          };
  graph g4(instructions4, 4);
  expected = "nodes:0,0 1,1 2,2 3,3 edges: 0:;1,2, 1:0,2,; 2:0,;3,1, 3:2,;";
  print_test_res(g4.graph_to_str() == expected, "program 4");
}

void test2() {
  cout << "Test2: illegal input cfg check" << endl;
  string expected;
  string actual;
  // test illegal input with loop
  expected = "illegal input: loop from node 1[2:3] to node 0[0:1]";
  actual = "";
  inst instructions1[6] = {inst(MOVXC, 1, 4),     // 0 mov r0, 4
                           inst(JMPGT, 0, 2, 3),  // 1 if r0 <= r2:
                           inst(MOVXC, 2, 15),    // 2 mov r2, 15
                           inst(JMPGT, 0, 2, -4), // 3 loop from here to instruction 0
                           inst(RETX, 2),         // 4 ret r2
                           inst(RETX, 0),         // 5 else ret r0
                          };
  try {
    graph g1(instructions1, 6);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 1");

  // test illegal input with loop
  expected = "illegal input: loop from node 2[2:2] to node 3[3:3]";
  actual = "";
  inst instructions2[5] = {inst(JMPGT, 0, 2, 2),     // 0 JMP to inst 3
                           inst(RETX, 2),            // 1 END
                           inst(NOP),                // 2
                           inst(JMPGT, 0, 2, -2),    // 3 JMP to inst 2, cause the loop from inst 2 to inst 3
                           inst(RETX, 0),            // 4 END
                          };
  try {
    graph g2(instructions2, 5);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 2");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 3 goes to an invalid instruction 4";
  actual = "";
  inst instructions3[4] = {inst(JMPGT, 0, 2, 2),     // 0 JMP to inst 3
                           inst(RETX, 2),            // 1 END
                           inst(RETX, 0),            // 2 END
                           inst(JMPGT, 0, 2, -2),    // 3 JMP to inst 2. illegal: no jump will go to 4
                          };
  try {
    graph g3(instructions3, 4);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 3");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 0 goes to an invalid instruction 2";
  actual = "";
  inst instructions4[2] = {inst(JMPGT, 0, 2, 1),     // 0 JMP to inst 2 -> illegal
                           inst(RETX, 2),            // 1 END
                          };
  try {
    graph g4(instructions4, 2);
  } catch (const string err_msg) {
    actual = err_msg;
  }

  print_test_res(actual == expected, "program 4");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 0 goes to an invalid instruction -1";
  actual = "";
  inst instructions5[2] = {inst(JMPGT, 0, 2, -2),     // 0 JMP to inst -1 -> illegal
                           inst(RETX, 2),             // 1 END
                          };
  try {
    graph g5(instructions5, 2);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 5");

  // loop caused by unconditional jmp
  expected = "illegal input: loop from node 1[2:2] to node 0[0:0]";
  actual = "";
  inst instructions6[3] = {inst(JMP, 1),
                           inst(RETX, 0),
                           inst(JMP, -3),
                          };
  try {
    graph g6(instructions6, 3);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 6");

  // test (jmp -1) loop
  expected = "illegal input: loop from node 0[0:0] to node 0[0:0]";
  actual = "";
  inst instructions7[1] = {inst(JMP, -1)};
  try {
    graph g7(instructions7, 1);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 7");
}

void test3() {
  cout << "Test 3: special cases check" << endl;
  string expected;
  // instruction JMP logic when jmp distance is 0
  inst instructions1[7] = {inst(MOVXC, 2, 4),
                           inst(ADDXY, 0, 2),
                           inst(MOVXC, 3, 15),
                           inst(JMPLE, 2, 0, 0), // expect: this JMPLE will not cause other blocks
                           inst(),
                           inst(MAXX, 0, 3),
                           inst(),
                          };
  graph g1(instructions1, 7);
  expected = "nodes:0,3 4,6 edges: 0:;1,1, 1:0,0,;";
  print_test_res(g1.graph_to_str() == expected, "case 1");

  // block ending up with NOP will connect to block starting from the instruction following NOP
  inst instructions2[4] = {inst(JMPEQ, 1, 3, 1),
                           inst(),
                           inst(MOVXC, 2, 4),
                           inst(),
                          };
  graph g2(instructions2, 4);
  expected = "nodes:0,0 1,1 2,3 edges: 0:;1,2, 1:0,;2, 2:1,0,;";
  print_test_res(g2.graph_to_str() == expected, "case 2");
}

/* test unconditional jmp */
void test4() {
  cout << "Test 4: unconditional jmp check" << endl;
  string expected;
  inst instructions1[3] = {inst(JMP, 1),
                           inst(ADDXY, 0, 0),
                           inst(RETX, 0),
                          };
  graph g1(instructions1, 3);
  expected = "nodes:0,0 2,2 edges: 0:;1, 1:0,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");


  inst instructions2[3] = {inst(JMP, 1),
                           inst(RETX, 0),
                           inst(JMP, -2),
                          };
  graph g2(instructions2, 3);
  expected = "nodes:0,0 2,2 1,1 edges: 0:;1, 1:0,;2, 2:1,;";
  print_test_res(g2.graph_to_str() == expected, "program 2");
}

int main () {
  test1();
  test2();
  test3();
  test4();

  return 0;
}
