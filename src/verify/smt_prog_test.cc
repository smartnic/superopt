#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/toy-isa/inst.h"
#include "smt_prog.h"

using namespace z3;

#define v(x) string_to_expr(x)

// basic block test
void test1() {
  std::cout << "test 1: basic block check starts...\n";
  vector<inst*> instptr_list(5);
  toy_isa_inst p[5] = {toy_isa_inst(toy_isa::MOVXC, 1, 10),   // 0
                       toy_isa_inst(toy_isa::JMPLT, 0, 1, 1), // 1
                       toy_isa_inst(toy_isa::RETX, 1),        // 2
                       toy_isa_inst(toy_isa::MAXC, 0, 15),    // 3
                       toy_isa_inst(toy_isa::RETX, 0),        // 4
                      };
  smt_prog ps;
  unsigned int prog_id = 0;
  p->convert_to_pointers(instptr_list, p);
  expr pl = ps.gen_smt(prog_id, instptr_list);
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
  toy_isa_inst p1[7] = {toy_isa_inst(toy_isa::JMPLT, 0, 1, 3),   // 0 [0:0]
                        toy_isa_inst(toy_isa::MOVXC, 0, 1),      // 1 [1:1]
                        toy_isa_inst(toy_isa::ADDXY, 0, 0),      // 2 [2:3]
                        toy_isa_inst(toy_isa::RETX, 0),          // 3
                        toy_isa_inst(toy_isa::ADDXY, 0, 0),      // 4 [4:5]
                        toy_isa_inst(toy_isa::JMPLT, 0, 1, -4),  // 5
                        toy_isa_inst(toy_isa::RETX, 0),          // 6 [6:6]
                       };
  prog_id = 1;
  instptr_list.resize(7);
  p1->convert_to_pointers(instptr_list, p1);
  ps.gen_smt(prog_id, instptr_list);
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
  toy_isa_inst p2[1] = {toy_isa_inst(toy_isa::ADDXY, 0, 0),
                       };
  prog_id = 2;
  instptr_list.resize(1);
  p2->convert_to_pointers(instptr_list, p2);
  ps.gen_smt(prog_id, instptr_list);
  // fmt: r_[prog_id]_[block_id]_[reg_id]_[version_id]
  expr post0 = implies(v("true"), v("output" + to_string(prog_id)) == v("r_2_0_0_1"));
  print_test_res(is_smt_valid(post0 == ps.post[0][0]), "post condition");
}

void test2() {
  std::cout << "\ntest2.1: check single instruction logic\n";
  vector<inst*> instptr_list(1);
  // check instrcution MAXX logic
  // case1: toy_isa_inst(toy_isa::MAXX, 0, 0); case2: toy_isa_inst(toy_isa::MAXX, 0, 1)
  toy_isa_inst p[1] = {toy_isa_inst(toy_isa::MAXX, 0, 0)};
  smt_prog ps;
  p->convert_to_pointers(instptr_list, p);
  unsigned int prog_id = 0;
  ps.gen_smt(prog_id, instptr_list);
  expr bl_expected = v("r_0_0_0_1") == v("r_0_0_0_0");
  bool assert_res = is_smt_valid(bl_expected == ps.bl[0]);

  toy_isa_inst p1[1] = {toy_isa_inst(toy_isa::MAXX, 0, 1)};
  instptr_list.resize(1);
  p1->convert_to_pointers(instptr_list, p1);
  ps.gen_smt(prog_id, instptr_list);
  bl_expected = (v("r_0_0_0_0") >= v("r_0_0_1_0") && (v("r_0_0_0_1") == v("r_0_0_0_0"))) ||
                (v("r_0_0_0_0") < v("r_0_0_1_0") && (v("r_0_0_0_1") == v("r_0_0_1_0")));
  assert_res = assert_res && is_smt_valid(bl_expected == ps.bl[0]);
  print_test_res(assert_res, "instruction MAXX logic");

  // check instruction JMP logic when jmp distance is 0
  toy_isa_inst p2[2] = {toy_isa_inst(toy_isa::JMPEQ, 2, 0, 0),
                        toy_isa_inst(toy_isa::ADDXY, 0, 1),
                       };
  instptr_list.resize(2);
  p2->convert_to_pointers(instptr_list, p2);
  expr pl = ps.gen_smt(prog_id, instptr_list);
  expr pl_expected = (v("r_0_1_0_0") == v("r_0_0_0_0")) &&
                     (v("r_0_1_1_0") == v("r_0_0_1_0")) &&
                     (v("r_0_1_2_0") == v("r_0_0_2_0")) &&
                     (v("r_0_1_3_0") == v("r_0_0_3_0")) &&
                     (v("r_0_1_0_1") == v("r_0_1_0_0") + v("r_0_1_1_0")) &&
                     (v("output0") == v("r_0_1_0_1"));
  print_test_res(is_smt_valid(pl_expected == pl), "instruction JMP logic when jmp distance is 0");
}

void test3() {
  std::cout << "\ntest3: check unconditional jmp program\n";
  toy_isa_inst p1[4] = {toy_isa_inst(toy_isa::JMP, 1),
                        toy_isa_inst(toy_isa::ADDXY, 0, 0),
                        toy_isa_inst(toy_isa::ADDXY, 0, 0),
                        toy_isa_inst(toy_isa::RETX, 0),
                       };
  int prog_id = 1;
  smt_prog ps;
  vector<inst*> instptr_list(4); 
  p1->convert_to_pointers(instptr_list, p1);
  ps.gen_smt(prog_id, instptr_list);
  expr pre_iv1_1 = (v("r_1_1_0_0") == v("r_1_0_0_0") && \
                    v("r_1_1_1_0") == v("r_1_0_1_0") && \
                    v("r_1_1_2_0") == v("r_1_0_2_0") && \
                    v("r_1_1_3_0") == v("r_1_0_3_0")
                   );
  expr bl1_1 = (v("r_1_1_0_1") == v("r_1_1_0_0") + v("r_1_1_0_0"));
  expr post1 = v("output" + to_string(prog_id)) == v("r_1_1_0_1");
  expr pl1 = pre_iv1_1 && bl1_1 && post1;
  print_test_res(is_smt_valid(pl1 == ps.pl), "unconditional jmp");
}

int main() {
  test1();
  test2();
  test3();
  return 0;
}
