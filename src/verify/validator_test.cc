#include <iostream>
#include "../../src/utils.h"
#include "validator.h"

using namespace z3;

void test1() {
  std::cout << "test 1: no branch program equivalence check starts...\n";
  // instructions1 == instructions2 == instructions3 != instructions4
  toy_isa_inst instructions1[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     /* mov r1, 4  */
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     /* add r0, r1 */
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    /* mov r2, 15 */
                                   toy_isa_inst(toy_isa::MAXC, 0, 15),     /* max r0, 15 */
                                   toy_isa_inst(toy_isa::MAXX, 0, 1),      /* max r0, r1 */
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };

  toy_isa_inst instructions2[7] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     /* mov r1, 4  */
                                   toy_isa_inst(toy_isa::MOVXC, 2, 10),    /* mov r2, 10 */
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     /* add r0, r1 */
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    /* mov r2, 15 */
                                   toy_isa_inst(toy_isa::MAXC, 0, 15),     /* max r0, 15 */
                                   toy_isa_inst(toy_isa::MAXX, 0, 1),      /* max r0, r1 */
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };

  toy_isa_inst instructions3[5] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     /* mov r1, 4  */
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     /* add r0, r1 */
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    /* mov r2, 15 */
                                   toy_isa_inst(toy_isa::MAXC, 0, 15),     /* max r0, 15 */
                                   toy_isa_inst(toy_isa::MAXX, 0, 2),      /* max r0, r2 */
                                  };                      // default: ret 0

  toy_isa_inst instructions4[6] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),     /* mov r1, 4  */
                                   toy_isa_inst(toy_isa::ADDXY, 0, 1),     /* add r0, r1 */
                                   toy_isa_inst(toy_isa::MOVXC, 2, 15),    /* mov r2, 15 */
                                   toy_isa_inst(toy_isa::MAXC, 0, -1),     /* max r0, 15 */
                                   toy_isa_inst(toy_isa::MAXX, 0, 3),      /* max r0, r3 */
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  validator vld(instructions1, 6);
  print_test_res(vld.is_equal_to(instructions2, 7), "instructions1 == instructions2");
  print_test_res(vld.is_equal_to(instructions3, 5), "instructions1 == instructions3");
  print_test_res(!vld.is_equal_to(instructions4, 6), "instructions1 != instructions4");
}

void test2() {
  validator vld;
  std::cout << "\ntest 2: branch program equivalence check starts...\n";
  // instructions1 == instructions2
  toy_isa_inst instructions1[3] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // if r0 <= r2:
                                   toy_isa_inst(toy_isa::RETX, 0),         // ret r0
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2;
                                  };
  toy_isa_inst instructions2[3] = {toy_isa_inst(toy_isa::JMPLT, 0, 2, 1),  // if r0 >= r2
                                   toy_isa_inst(toy_isa::RETX, 2),         // ret r2
                                   toy_isa_inst(toy_isa::RETX, 0),         // ret r0
                                  };
  vld.set_orig(instructions1, 3);
  print_test_res(vld.is_equal_to(instructions2, 3), "instructions1 == instructions2");

  // instructions3 == instructions4 != instructions5
  toy_isa_inst instructions3[3] = {toy_isa_inst(toy_isa::JMPGT, 0, 2, 1),  // return max(r0, r2)
                                   toy_isa_inst(toy_isa::RETX, 2),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  toy_isa_inst instructions4[2] = {toy_isa_inst(toy_isa::MAXX, 0, 2),      // return r0=max(r0, r2)
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  toy_isa_inst instructions5[3] = {toy_isa_inst(toy_isa::JMPGT, 2, 0, 1),  // return min(r0, r2)
                                   toy_isa_inst(toy_isa::RETX, 2),
                                   toy_isa_inst(toy_isa::RETX, 0),
                                  };
  vld.set_orig(instructions3, 3);
  print_test_res(vld.is_equal_to(instructions4, 2), "instructions3 == instructions4");
  print_test_res(!vld.is_equal_to(instructions5, 3), "instructions3 != instructions5");

  // f(x) = max(x, r1, r2, 10)
  // p11 == p12
  toy_isa_inst p11[5] = {toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::MAXX, 0, 2),
                         toy_isa_inst(toy_isa::MOVXC, 1, 10),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::RETX, 0),
                        };
  toy_isa_inst p12[11] = {toy_isa_inst(toy_isa::JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                          toy_isa_inst(toy_isa::MOVXC, 0, 0),
                          toy_isa_inst(toy_isa::ADDXY, 0, 1),
                          toy_isa_inst(toy_isa::JMPGT, 0, 2, 2), // skip r0 <- r2, if r0 > r2
                          toy_isa_inst(toy_isa::MOVXC, 0, 0),
                          toy_isa_inst(toy_isa::ADDXY, 0, 2),
                          toy_isa_inst(toy_isa::MOVXC, 1, 10),   // r1 <- 10
                          toy_isa_inst(toy_isa::JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                          toy_isa_inst(toy_isa::MOVXC, 0, 0),
                          toy_isa_inst(toy_isa::ADDXY, 0, 1),
                          toy_isa_inst(toy_isa::RETX, 0),        // ret r0
                         };
  vld.set_orig(p11, 5);
  print_test_res(vld.is_equal_to(p12, 11), "f(x)_p1 == f(x)_p2");

  // check unconditonal jmp
  // p13 != p11, p14 == p15 == p11
  toy_isa_inst p13[6] = {toy_isa_inst(toy_isa::JMP, 3),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::MAXX, 0, 2),
                         toy_isa_inst(toy_isa::MOVXC, 1, 10),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::RETX, 0),
                        };
  toy_isa_inst p14[6] = {toy_isa_inst(toy_isa::JMP, 0),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::MAXX, 0, 2),
                         toy_isa_inst(toy_isa::MOVXC, 1, 10),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::RETX, 0),
                        };
  toy_isa_inst p15[7] = {toy_isa_inst(toy_isa::JMP, 3),
                         toy_isa_inst(toy_isa::MOVXC, 1, 10),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::RETX, 0),
                         toy_isa_inst(toy_isa::MAXX, 0, 1),
                         toy_isa_inst(toy_isa::MAXX, 0, 2),
                         toy_isa_inst(toy_isa::JMP, -6),
                        };
  print_test_res(!vld.is_equal_to(p13, 6), "unconditonal jmp 1");
  print_test_res(vld.is_equal_to(p14, 6), "unconditonal jmp 2");
  print_test_res(vld.is_equal_to(p15, 7), "unconditonal jmp 3");
}

// fx == program_fx test
void test3() {
  std::cout << "\ntest 3 starts...\n";
  expr x = string_to_expr("x");
  expr y = string_to_expr("y");
  expr fx = implies(x > 10, y == x) && implies(x <= 10, y == 10);
  toy_isa_inst p_fx[4] = {toy_isa_inst(toy_isa::MOVXC, 1, 10),
                          toy_isa_inst(toy_isa::JMPLT, 0, 1, 1),
                          toy_isa_inst(toy_isa::RETX, 0),
                          toy_isa_inst(toy_isa::RETX, 1),
                         };
  validator vld(fx, x, y);
  print_test_res(vld.is_equal_to(p_fx, 4), "Program_f(x) == (f(x)=max(x, 10))");
}

void test4() {
  std::cout << "\ntest4: check counterexample generation\n";
  // orig: output = max(input, 11);
  // synth: output = max(input, 10);
  // counterexample: input <= 10, output = 11
  toy_isa_inst orig[3] = {toy_isa_inst(toy_isa::MOVXC, 2, 11),    /* mov r2, 11 */
                          toy_isa_inst(toy_isa::MAXX, 0, 2),      /* max r0, r2 */
                          toy_isa_inst(toy_isa::RETX, 0),
                         };
  toy_isa_inst synth[3] = {toy_isa_inst(toy_isa::MOVXC, 2, 10),    /* mov r2, 10 */
                           toy_isa_inst(toy_isa::MAXX, 0, 2),      /* max r0, r2 */
                           toy_isa_inst(toy_isa::RETX, 0),
                          };
  validator vld(orig, 3);
  inout counterex;
  counterex.set_in_out(0, 0);
  if (!vld.is_equal_to(synth, 3)) {
    counterex = vld._last_counterex;
  }
  print_test_res((counterex.input <= 10) && (counterex.output == 11), "counterexample generation");
}

void test5() {
  std::cout << "\ntest5: check get_orig_output\n";
  // orig: output = max(input, 11);
  toy_isa_inst orig[3] = {toy_isa_inst(toy_isa::MOVXC, 2, 11),    /* mov r2, 11 */
                          toy_isa_inst(toy_isa::MAXX, 0, 2),      /* max r0, r2 */
                          toy_isa_inst(toy_isa::RETX, 0),
                         };
  validator vld(orig, 3);
  vector<int> ex_set = {1, 10, 11, 12, 20};
  vector<int> expected = {11, 11, 11, 12, 20};
  for (size_t i = 0; i < ex_set.size(); i++) {
    int output = vld.get_orig_output(ex_set[i], orig->get_num_regs());
    print_test_res(output == expected[i], to_string(i));
  }
}

int main(int argc, char *argv[]) {
  test1(); // no branch
  test2(); // with branch
  test3();
  test4();
  test5();
  return 0;
}
