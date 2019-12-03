#include <iostream>
#include "validator.h"
#include "inst.h"
#include "utils.h"

using namespace z3;

#define v(x) string_to_expr(x)

// basic block test
void test1() {
  std::cout << "test 1: basic block check starts...\n";
  inst p[5] = {inst(MOVXC, 1, 10),   // 0
               inst(JMPLT, 0, 1, 1), // 1
               inst(RETX, 1),        // 2
               inst(MAXC, 0, 15),    // 3
               inst(RETX, 0),        // 4
              };
  smt_prog ps;
  unsigned int prog_id = 0;
  expr pl = ps.gen_smt(prog_id, p, 5);
  // test block 2[3:4]
  std::cout << "test 1.1: check basic block 2[3:4]\n";
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

  std::cout << "\ntest1.2: check basic block 2[2:3]\n";
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

  std::cout << "\ntest1.3: check program-end basic block 0[0:0] without RET instructions\n";
  inst p2[1] = {inst(ADDXY, 0, 0),
               };
  prog_id = 2;
  ps.gen_smt(prog_id, p2, 1);
  // fmt: r_[prog_id]_[block_id]_[reg_id]_[version_id]
  expr post0 = implies(v("true"), v("output" + to_string(prog_id)) == v("r_2_0_0_1"));
  print_test_res(is_smt_valid(post0 == ps.post[0][0]), "post condition");
}

void test2() {
  std::cout << "\ntest2.1: check single instruction logic\n";
  // check instrcution MAXX logic
  // case1: inst(MAXX, 0, 0); case2: inst(MAXX, 0, 1)
  inst p[1] = {inst(MAXX, 0, 0)};
  smt_prog ps;
  unsigned int prog_id = 0;
  ps.gen_smt(prog_id, p, 1);
  expr bl_expected = v("r_0_0_0_1") == v("r_0_0_0_0");
  bool assert_res = is_smt_valid(bl_expected == ps.bl[0]);
  inst p1[1] = {inst(MAXX, 0, 1)};
  ps.gen_smt(prog_id, p1, 1);
  bl_expected = (v("r_0_0_0_0") >= v("r_0_0_1_0") && (v("r_0_0_0_1") == v("r_0_0_0_0"))) ||
                (v("r_0_0_0_0") < v("r_0_0_1_0") && (v("r_0_0_0_1") == v("r_0_0_1_0")));
  assert_res = assert_res && is_smt_valid(bl_expected == ps.bl[0]);
  print_test_res(assert_res, "instruction MAXX logic");

  // check instruction JMP logic when jmp distance is 0
  inst p2[2] = {inst(JMPEQ, 2, 0, 0),
                inst(ADDXY, 0, 1),
               };
  expr pl = ps.gen_smt(prog_id, p2, 2);
  expr pl_expected = (v("r_0_1_0_0") == v("r_0_0_0_0")) &&
                     (v("r_0_1_1_0") == v("r_0_0_1_0")) &&
                     (v("r_0_1_2_0") == v("r_0_0_2_0")) &&
                     (v("r_0_1_3_0") == v("r_0_0_3_0")) &&
                     (v("r_0_1_0_1") == v("r_0_1_0_0") + v("r_0_1_1_0")) &&
                     (v("output0") == v("r_0_1_0_1"));
  print_test_res(is_smt_valid(pl_expected == pl), "instruction JMP logic when jmp distance is 0");
}

int main() {
  test1();
  test2();
  return 0;
}
