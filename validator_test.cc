#include <iostream>
#include "validator.h"
#include "inst.h"

using namespace z3;

#define v(x) string_to_expr(x)

void print_test_res(bool res, string test_name) {
  if (res) {
    std::cout << "check " + test_name + " SUCCESS\n";
  } else {
    std::cout << "check " + test_name + " NOT SUCCESS\n";
  }
}

void test1() {
  std::cout << "\ntest 1: no branch program equivalence check starts...\n";
  // instructions1 == instructions2 == instructions3 != instructions4
  inst instructions1[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                           inst(ADDXY, 0, 1),     /* add r0, r1 */
                           inst(MOVXC, 2, 15),    /* mov r2, 15 */
                           inst(MAXC, 0, 15),     /* max r0, 15 */
                           inst(MAXX, 0, 1),      /* max r0, r1 */
                           inst(RETX, 0),
                          };

  inst instructions2[7] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                           inst(MOVXC, 2, 10),    /* mov r2, 10 */
                           inst(ADDXY, 0, 1),     /* add r0, r1 */
                           inst(MOVXC, 2, 15),    /* mov r2, 15 */
                           inst(MAXC, 0, 15),     /* max r0, 15 */
                           inst(MAXX, 0, 1),      /* max r0, r1 */
                           inst(RETX, 0),
                          };

  inst instructions3[5] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                           inst(ADDXY, 0, 1),     /* add r0, r1 */
                           inst(MOVXC, 2, 15),    /* mov r2, 15 */
                           inst(MAXC, 0, 15),     /* max r0, 15 */
                           inst(MAXX, 0, 2),      /* max r0, r2 */
                          };                      // default: ret 0

  inst instructions4[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                           inst(ADDXY, 0, 1),     /* add r0, r1 */
                           inst(MOVXC, 2, 15),    /* mov r2, 15 */
                           inst(MAXC, 0, -1),     /* max r0, 15 */
                           inst(MAXX, 0, 3),      /* max r0, r3 */
                           inst(RETX, 0),
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
  inst instructions1[3] = {inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
                           inst(RETX, 0),         // ret r0
                           inst(RETX, 2),         // ret r2;
                          };
  inst instructions2[3] = {inst(JMPLT, 0, 2, 1),  // if r0 >= r2
                           inst(RETX, 2),         // ret r2
                           inst(RETX, 0),         // ret r0
                          };
  vld.set_orig(instructions1, 3);
  print_test_res(vld.is_equal_to(instructions2, 3), "instructions1 == instructions2");

  // instructions3 == instructions4 != instructions5
  inst instructions3[3] = {inst(JMPGT, 0, 2, 1),  // return max(r0, r2)
                           inst(RETX, 2),
                           inst(RETX, 0),
                          };
  inst instructions4[2] = {inst(MAXX, 0, 2),      // return r0=max(r0, r2)
                           inst(RETX, 0),
                          };
  inst instructions5[3] = {inst(JMPGT, 2, 0, 1),  // return min(r0, r2)
                           inst(RETX, 2),
                           inst(RETX, 0),
                          };
  vld.set_orig(instructions3, 3);
  print_test_res(vld.is_equal_to(instructions4, 2), "instructions3 == instructions4");
  print_test_res(!vld.is_equal_to(instructions5, 3), "instructions3 != instructions5");

  // f(x) = max(x, r1, r2, 10)
  // p11 == p12
  inst p11[5] = {inst(MAXX, 0, 1),
                 inst(MAXX, 0, 2),
                 inst(MOVXC, 1, 10),
                 inst(MAXX, 0, 1),
                 inst(RETX, 0),
                };
  inst p12[11] = {inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                  inst(MOVXC, 0, 0),
                  inst(ADDXY, 0, 1),
                  inst(JMPGT, 0, 2, 2), // skip r0 <- r2, if r0 > r2
                  inst(MOVXC, 0, 0),
                  inst(ADDXY, 0, 2),
                  inst(MOVXC, 1, 10),   // r1 <- 10
                  inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
                  inst(MOVXC, 0, 0),
                  inst(ADDXY, 0, 1),
                  inst(RETX, 0),        // ret r0
                 };
  vld.set_orig(p11, 5);
  print_test_res(vld.is_equal_to(p12, 11), "f(x)_p1 == f(x)_p2");
}

// fx == program_fx test
void test3() {
  std::cout << "\ntest 3 starts...\n";
  expr x = string_to_expr("x");
  expr y = string_to_expr("y");
  expr fx = implies(x > 10, y == x) && implies(x <= 10, y == 10);
  inst p_fx[4] = {inst(MOVXC, 1, 10),
                  inst(JMPLT, 0, 1, 1),
                  inst(RETX, 0),
                  inst(RETX, 1),
                 };
  validator vld(fx, x, y);
  print_test_res(vld.is_equal_to(p_fx, 4), "Program_f(x) == (f(x)=max(x, 10))");
}

// basic block test
void test4() {
  std::cout << "\ntest 4: basic block check starts...\n";
  inst p[5] = {inst(MOVXC, 1, 10),   // 0
               inst(JMPLT, 0, 1, 1), // 1
               inst(RETX, 1),        // 2
               inst(MAXC, 0, 15),    // 3
               inst(RETX, 0),        // 4
              };
  prog_smt ps;
  unsigned int prog_id = 0;
  expr pl = ps.gen_smt(prog_id, p, 5);
  // test block 2[3:4]
  std::cout << "test 4.1: check basic block 2[3:4]\n";
  // fmt: r_[prog_id]_[block_id]_[reg_id]_[version_id]
  expr prePC2 = (v("r_0_0_0_0") < v("r_0_0_1_1"));
  expr preIV2 = (v("r_0_0_0_0") == v("r_0_2_0_0") && \
                 v("r_0_0_1_1") == v("r_0_2_1_0") && \
                 v("r_0_0_2_0") == v("r_0_2_2_0") && \
                 v("r_0_0_3_0") == v("r_0_2_3_0")
                );
  expr bl2 = (implies(v("r_0_2_0_0") > 15, v("r_0_2_0_1") == v("r_0_2_0_0")) && \
              implies(v("r_0_2_0_0") <= 15, v("r_0_2_0_1") == 15)
             );
  expr post2 = implies(prePC2, v("output" + to_string(prog_id)) == v("r_0_2_0_1"));
  print_test_res(is_smt_valid(prePC2 == ps.path_con[2][0]), "pre path condition");
  print_test_res(is_smt_valid(preIV2 == ps.reg_iv[2][0]), "pre register initial values");
  print_test_res(is_smt_valid(bl2 == ps.bl[2]), "basic block logic");
  print_test_res(is_smt_valid(post2 == ps.post[2][0]), "post condition");

  std::cout << "\ntest4.2: check basic block 2[2:3]\n";
  inst p1[7] = {inst(JMPLT, 0, 1, 3),   // 0 [0:0]
                inst(MOVXC, 0, 1),      // 1 [1:1]
                inst(ADDXY, 0, 0),      // 2 [2:3]
                inst(RETX, 0),          // 3
                inst(ADDXY, 0, 0),      // 4 [4:5]
                inst(JMPLT, 0, 1, -4),  // 5
                inst(RETX, 0),          // 6 [6:6]
               };
  prog_id = 1;
  ps.gen_smt(prog_id, p1, 7);
  // blocks: 0[0:0] 1[1:1] 2[2:3] 3[4:5] 4[6:6]
  // case0: 0 -> 1 -> 2; case1: 0 -> 3 -> 2
  // fmt: r_[prog_id]_[block_id]_[reg_id]_[version_id]
  expr pre_pc2_0 = !(v("r_1_0_0_0") < v("r_1_0_1_0"));
  expr pre_pc2_1 = ((v("r_1_0_0_0") < v("r_1_0_1_0")) && \
                    (v("r_1_3_0_1") < v("r_1_3_1_0"))
                   );
  expr pre_iv2_0 = (v("r_1_1_0_1") == v("r_1_2_0_0") && \
                    v("r_1_1_1_0") == v("r_1_2_1_0") && \
                    v("r_1_1_2_0") == v("r_1_2_2_0") && \
                    v("r_1_1_3_0") == v("r_1_2_3_0")
                   );
  expr pre_iv2_1 = (v("r_1_3_0_1") == v("r_1_2_0_0") && \
                    v("r_1_3_1_0") == v("r_1_2_1_0") && \
                    v("r_1_3_2_0") == v("r_1_2_2_0") && \
                    v("r_1_3_3_0") == v("r_1_2_3_0")
                   );
  bl2 = (v("r_1_2_0_1") == v("r_1_2_0_0") + v("r_1_2_0_0"));
  post2 = implies(pre_pc2_0 || pre_pc2_1, v("output" + to_string(prog_id)) == v("r_1_2_0_1"));
  print_test_res(is_smt_valid(pre_pc2_0 == ps.path_con[2][0]), "pre path condition 0");
  print_test_res(is_smt_valid(pre_pc2_1 == ps.path_con[2][1]), "pre path condition 1");
  print_test_res(is_smt_valid(pre_iv2_0 == ps.reg_iv[2][0]), "pre register initial values 0");
  print_test_res(is_smt_valid(pre_iv2_1 == ps.reg_iv[2][1]), "pre register initial values 1");
  print_test_res(is_smt_valid(bl2 == ps.bl[2]), "basic block logic");
  print_test_res(is_smt_valid(post2 == ps.post[2][0]), "post condition");

  std::cout << "\ntest4.3: check program-end basic block 0[0:0] without RET instructions\n";
  inst p2[1] = {inst(ADDXY, 0, 0),
               };
  prog_id = 2;
  ps.gen_smt(prog_id, p2, 1);
  // fmt: r_[prog_id]_[block_id]_[reg_id]_[version_id]
  expr post0 = implies(v("true"), v("output" + to_string(prog_id)) == v("r_2_0_0_1"));
  print_test_res(is_smt_valid(post0 == ps.post[0][0]), "post condition");
}

void test5() {
  std::cout << "\ntest5: check counterexample generation\n";
  // orig: output = max(input, 11);
  // synth: output = max(input, 10);
  // counterexample: input <= 10, output = 11
  inst orig[3] = {inst(MOVXC, 2, 11),    /* mov r2, 11 */
                  inst(MAXX, 0, 2),      /* max r0, r2 */
                  inst(RETX, 0),
                 };
  inst synth[3] = {inst(MOVXC, 2, 10),    /* mov r2, 10 */
                   inst(MAXX, 0, 2),      /* max r0, r2 */
                   inst(RETX, 0),
                  };
  validator vld(orig, 3);
  inout counterex;
  counterex.set_in_out(0, 0);
  if (!vld.is_equal_to(synth, 3)) {
    counterex = vld.counterex;
  }
  print_test_res((counterex.input <= 10) && (counterex.output == 11), "counterexample generation");
}

int main(int argc, char *argv[]) {
  test1(); // no branch
  test2(); // with branch
  test3();
  test4();
  test5();
  return 0;
}
