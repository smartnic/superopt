#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "cfg.h"

using namespace std;

void test1() {
  toy_isa_inst instructions1[6] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     // add r0, r1
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  vector<inst*> instptr_list(6, 0);
  instructions1->convert_to_pointers(instptr_list, instructions1);
  graph g1(instptr_list);
  cout << "graph1 is" << g1 << endl;

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
  cout << "graph2 is" << g2 << endl;

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
  cout << "graph3 is" << g3 << endl;

  toy_isa_inst instructions4[4] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 2 JMP to toy_isa_inst 1
                                   toy_isa_inst(toy_isa::RETX, 2),            // 3 END
                                  };
  instptr_list.resize(4, 0);
  instructions4->convert_to_pointers(instptr_list, instructions4);
  graph g4(instptr_list);
  cout << "graph4 is" << g4 << endl;
}

void test2() {
  vector<inst*> instptr_list(6);
  // test illegal input with loop
  cout << "graph1 is" << endl;
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
    cerr << err_msg << endl;
  }

  // test illegal input with loop
  cout << "graph2 is " << endl;
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
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph3 is " << endl;
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
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph4 is " << endl;
  toy_isa_inst instructions4[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                  };
  instptr_list.resize(2);
  instructions4->convert_to_pointers(instptr_list, instructions4);
  try {
    graph g4(instptr_list);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph5 is " << endl;
  toy_isa_inst instructions5[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),     // 0 JMP to toy_isa_inst -1 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),             // 1 END
                                  };
  instptr_list.resize(2);
  instructions5->convert_to_pointers(instptr_list, instructions5);
  try {
    graph g5(instptr_list);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  cout << "graph6 is " << endl;
  // loop caused by unconditional jmp
  toy_isa_inst instructions6[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -3),
                                  };
  instptr_list.resize(3);
  instructions6->convert_to_pointers(instptr_list, instructions6);
  try {
    graph g6(instptr_list);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  cout << "graph7 is " << endl;
  // test (jmp -1) loop
  toy_isa_inst instructions7[1] = {toy_isa_inst(toy_isa::JMP, -1)};
  instptr_list.resize(1);
  instructions7->convert_to_pointers(instptr_list, instructions7);
  try {
    graph g7(instptr_list);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }
}

void test3() {
  vector<inst*> instptr_list(7);
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
  int expected_num_blocks = 2;
  print_test_res(expected_num_blocks == int(g1.nodes.size()),
                 "instruction JMP logic when jmp distance is 0");

  toy_isa_inst instructions2[4] = {toy_isa_inst(toy_isa::JMPEQ, 1, 3, 1),
                                   toy_isa_inst(),
                                   toy_isa_inst(toy_isa::MOVXC, 2, 4),
                                   toy_isa_inst(),
                                  };
  instptr_list.resize(4);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  graph g2(instptr_list);
  bool assert_res = (int(g2.nodes.size() == 3)) &&
                    (int(g2.nodes_in[2].size()) == 2) &&
                    ((g2.nodes_in[2][0] == 0 && g2.nodes_in[2][1] == 1) ||
                     (g2.nodes_in[2][0] == 1 && g2.nodes_in[2][1] == 0));
  print_test_res(assert_res, "block ending up with NOP will connect to " \
                 "block starting from the instruction following NOP");
}

/* test unconditional jmp */
void test4() {
  cout << "Test unconditional jmp" << endl;
  vector<inst*> instptr_list(3);
  toy_isa_inst instructions1[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::ADDXY, 0, 0),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  instructions1->convert_to_pointers(instptr_list, instructions1);
  graph g1(instptr_list);
  cout << "graph1 is" << g1 << endl;

  toy_isa_inst instructions2[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -2),
                                  };
  instptr_list.resize(3);
  instructions2->convert_to_pointers(instptr_list, instructions2);
  graph g2(instptr_list);
  cout << "graph2 is" << g2 << endl;
}

int main () {
  test1();
  test2();
  test3();
  test4();

  return 0;
}
