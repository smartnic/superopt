#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "cfg.h"

using namespace std;

void test1() {
  cout << "Test1: cfg check" << endl;
  toy_isa_inst instructions1[6] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     // add r0, r1
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  vector<inst*> instptr_list(6, 0);
  instructions1->convert_to_pointers(instptr_list, instructions1);
  string expected;
  graph g1(instptr_list);
  expected = "nodes:0,0 1,1 2,3 4,4 5,5 edges: 0:;1,2, 1:0,;2, 2:1,0,;3,4, 3:2,; 4:2,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");

  toy_isa_inst instructions2[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // mov r0, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  instptr_list.resize(6, 0);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  graph g2(instptr_list);
  expected = "nodes:0,1 2,3 4,4 5,5 edges: 0:;1,3, 1:0,;2,3, 2:1,; 3:1,0,;";
  print_test_res(g2.graph_to_str() == expected, "program 2");

  toy_isa_inst instructions3[7] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // mov r1, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // if r0 <= r3:
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  instptr_list.resize(7, 0);
  instructions3->convert_to_pointers(instptr_list, instructions3);
  graph g3(instptr_list);
  expected = "nodes:0,1 2,2 5,5 edges: 0:;1,2, 1:0,; 2:0,;";
  print_test_res(g3.graph_to_str() == expected, "program 3");

  toy_isa_inst instructions4[4] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 2 JMP to toy_isa_inst 1
                                   toy_isa_inst(toy_isa::RETX, 2),            // 3 END
                                  };
  instptr_list.resize(4, 0);
  instructions4->convert_to_pointers(instptr_list, instructions4);
  graph g4(instptr_list);
  expected = "nodes:0,0 1,1 2,2 3,3 edges: 0:;1,2, 1:0,2,; 2:0,;3,1, 3:2,;";
  print_test_res(g4.graph_to_str() == expected, "program 4");
}

void test2() {
  cout << "Test2: illegal input cfg check" << endl;
  vector<inst*> instptr_list(6);
  string expected;
  string actual;
  // test illegal input with loop
  expected = "illegal input: loop from node 1[2:3] to node 0[0:1]";
  actual = "";
  toy_isa_inst instructions1[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // 0 mov r0, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // 1 if r0 <= r2:
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // 2 mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -4), // 3 loop from here to instruction 0
                                   toy_isa_inst(toy_isa::RETX, 2),         // 4 ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // 5 else ret r0
                                  };
  instructions1->convert_to_pointers(instptr_list, instructions1);
  try {
    graph g1(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 1");

  // test illegal input with loop
  expected = "illegal input: loop from node 2[2:2] to node 3[3:3]";
  actual = "";
  toy_isa_inst instructions2[5] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 2),     // 0 JMP to toy_isa_inst 3
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::NOP),                // 2
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 3 JMP to toy_isa_inst 2, cause the loop from toy_isa_inst 2 to toy_isa_inst 3
                                   toy_isa_inst(toy_isa::RETX, 0),            // 4 END
                                  };
  instptr_list.resize(5);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  try {
    graph g2(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 2");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 3 goes to an invalid instruction 4";
  actual = "";
  toy_isa_inst instructions3[4] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 2),     // 0 JMP to toy_isa_inst 3
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::RETX, 0),            // 2 END
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 3 JMP to toy_isa_inst 2. illegal: no jump will go to 4
                                  };
  instptr_list.resize(4);
  instructions3->convert_to_pointers(instptr_list, instructions3);
  try {
    graph g3(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 3");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 0 goes to an invalid instruction 2";
  actual = "";
  toy_isa_inst instructions4[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                  };
  instptr_list.resize(2);
  instructions4->convert_to_pointers(instptr_list, instructions4);
  try {
    graph g4(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }

  print_test_res(actual == expected, "program 4");

  // test illegal input: goes to an invalid instruction
  expected = "illegal input: instruction 0 goes to an invalid instruction -1";
  actual = "";
  toy_isa_inst instructions5[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),     // 0 JMP to toy_isa_inst -1 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),             // 1 END
                                  };
  instptr_list.resize(2);
  instructions5->convert_to_pointers(instptr_list, instructions5);
  try {
    graph g5(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 5");

  // loop caused by unconditional jmp
  expected = "illegal input: loop from node 1[2:2] to node 0[0:0]";
  actual = "";
  toy_isa_inst instructions6[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -3),
                                  };
  instptr_list.resize(3);
  instructions6->convert_to_pointers(instptr_list, instructions6);
  try {
    graph g6(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 6");

  // test (jmp -1) loop
  expected = "illegal input: loop from node 0[0:0] to node 0[0:0]";
  actual = "";
  toy_isa_inst instructions7[1] = {toy_isa_inst(toy_isa::JMP, -1)};
  instptr_list.resize(1);
  instructions7->convert_to_pointers(instptr_list, instructions7);
  try {
    graph g7(instptr_list);
  } catch (const string err_msg) {
    actual = err_msg;
  }
  print_test_res(actual == expected, "program 7");
}

void test3() {
  cout << "Test 3: special cases check" << endl;
  vector<inst*> instptr_list(7);
  string expected;
  // instruction JMP logic when jmp distance is 0
  toy_isa_inst instructions1[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),
                                   toy_isa_inst(toy_isa::ADDXY, 0, 2),
                                   toy_isa_inst(toy_isa::MOVXC, 3, 15),
                                   toy_isa_inst(toy_isa::JMPLE, 2, 0, 0), // expect: this JMPLE will not cause other blocks
                                   toy_isa_inst(),
                                   toy_isa_inst(toy_isa::MAXX, 0, 3),
                                   toy_isa_inst(),
                                  };
  instructions1->convert_to_pointers(instptr_list, instructions1);
  graph g1(instptr_list);
  expected = "nodes:0,3 4,6 edges: 0:;1,1, 1:0,0,;";
  print_test_res(g1.graph_to_str() == expected, "case 1");

  // block ending up with NOP will connect to block starting from the instruction following NOP
  toy_isa_inst instructions2[4] = {toy_isa_inst(toy_isa::JMPEQ, 1, 3, 1),
                                   toy_isa_inst(),
                                   toy_isa_inst(toy_isa::MOVXC, 2, 4),
                                   toy_isa_inst(),
                                  };
  instptr_list.resize(4);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  graph g2(instptr_list);
  expected = "nodes:0,0 1,1 2,3 edges: 0:;1,2, 1:0,;2, 2:1,0,;";
  print_test_res(g2.graph_to_str() == expected, "case 2");
}

/* test unconditional jmp */
void test4() {
  cout << "Test 4: unconditional jmp check" << endl;
  vector<inst*> instptr_list(3);
  string expected;
  toy_isa_inst instructions1[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::ADDXY, 0, 0),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  instructions1->convert_to_pointers(instptr_list, instructions1);
  graph g1(instptr_list);
  expected = "nodes:0,0 2,2 edges: 0:;1, 1:0,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");


  toy_isa_inst instructions2[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -2),
                                  };
  instptr_list.resize(3);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  graph g2(instptr_list);
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
