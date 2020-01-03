#include <iostream>
#include "../../src/utils.h"
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
  graph g1(instructions1, 6);
  cout << "graph1 is" << g1 << endl;

  toy_isa_inst instructions2[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // mov r0, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  graph g2(instructions2, 6);
  cout << "graph2 is" << g2 << endl;

  toy_isa_inst instructions3[7] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // mov r1, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // if r0 <= r3:
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // else ret r0
                                  };
  graph g3(instructions3, 7);
  cout << "graph3 is" << g3 << endl;

  toy_isa_inst instructions4[4] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 2 JMP to toy_isa_inst 1
                                   toy_isa_inst(toy_isa::RETX, 2),            // 3 END
                                  };
  graph g4(instructions4, 4);
  cout << "graph4 is" << g4 << endl;
}

void test2() {
  // test illegal input with loop
  cout << "graph1 is" << endl;
  toy_isa_inst instructions1[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     // 0 mov r0, 4
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, 3),  // 1 if r0 <= r2:
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    // 2 mov r2, 15
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -4), // 3 loop from here to instruction 0
                                   toy_isa_inst(toy_isa::RETX, 2),         // 4 ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // 5 else ret r0
                                  };
  try {
    graph g1(instructions1, 6);
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
  try {
    graph g2(instructions2, 5);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph3 is " << endl;
  toy_isa_inst instructions3[5] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 2),     // 0 JMP to toy_isa_inst 3
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                   toy_isa_inst(toy_isa::RETX, 0),            // 2 END
                                   toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),    // 3 JMP to toy_isa_inst 2. illegal: no jump will go to 4
                                  };
  try {
    graph g3(instructions3, 4);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph4 is " << endl;
  toy_isa_inst instructions4[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),     // 0 JMP to toy_isa_inst 2 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),            // 1 END
                                  };
  try {
    graph g4(instructions4, 2);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  // test illegal input: goes to an invalid instruction
  cout << "graph5 is " << endl;
  toy_isa_inst instructions5[2] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, -2),     // 0 JMP to toy_isa_inst -1 -> illegal
                                   toy_isa_inst(toy_isa::RETX, 2),             // 1 END
                                  };
  try {
    graph g5(instructions5, 2);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }

  cout << "graph6 is " << endl;
  // loop caused by unconditional jmp
  toy_isa_inst instructions6[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -3),
                                  };
  try {
    graph g6(instructions6, 3);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }
  cout << "graph7 is " << endl;
  // test (jmp -1) loop
  toy_isa_inst instructions7[1] = {toy_isa_inst(toy_isa::JMP, -1)};
  try {
    graph g7(instructions7, 1);
  } catch (const string err_msg) {
    cerr << err_msg << endl;
  }
}

void test3() {
  toy_isa_inst instructions10[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),
                                    toy_isa_inst(toy_isa::ADDXY, 0, 2),
                                    toy_isa_inst(toy_isa::MOVXC, 3, 15),
                                    toy_isa_inst(toy_isa::JMPLE, 2, 0, 0), // expect: this JMPLE will not cause other blocks
                                    toy_isa_inst(),
                                    toy_isa_inst(toy_isa::MAXX, 0, 3),
                                    toy_isa_inst(),
                                   };
  graph g10(instructions10, 7);
  int expected_num_blocks = 2;
  print_test_res(expected_num_blocks == int(g10.nodes.size()),
                 "instruction JMP logic when jmp distance is 0");

  toy_isa_inst instructions11[4] = {toy_isa_inst(toy_isa::JMPEQ, 1, 3, 1),
                                    toy_isa_inst(),
                                    toy_isa_inst(toy_isa::MOVXC, 2, 4),
                                    toy_isa_inst(),
                                   };
  graph g11(instructions11, 4);
  bool assert_res = (int(g11.nodes.size() == 3)) &&
                    (int(g11.nodes_in[2].size()) == 2) &&
                    ((g11.nodes_in[2][0] == 0 && g11.nodes_in[2][1] == 1) ||
                     (g11.nodes_in[2][0] == 1 && g11.nodes_in[2][1] == 0));
  print_test_res(assert_res, "block ending up with NOP will connect to " \
                 "block starting from the instruction following NOP");
}

/* test unconditional jmp */
void test4() {
  cout << "Test unconditional jmp" << endl;
  toy_isa_inst instructions1[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::ADDXY, 0, 0),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  graph g1(instructions1, 3);
  cout << "graph1 is" << g1 << endl;

  toy_isa_inst instructions2[3] = {toy_isa_inst(toy_isa::JMP, 1),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                   toy_isa_inst(toy_isa::JMP, -2),
                                  };
  graph g2(instructions2, 3);
  cout << "graph2 is" << g2 << endl;
}

int main () {
  test1();
  test2();
  test3();
  test4();

  return 0;
}
