#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "smt_prog.h"

using namespace z3;

#define v(x) string_to_expr(x)

void test1() {
  // branch test for st/ld
  inst p1[6] = {inst(STXB, 10, -1, 1),
                inst(JEQXC, 1, 0x12, 2),
                inst(MOV64XC, 1, 0x12),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  smt_prog ps;
  unsigned int prog_id = 0;
  expr pl = ps.gen_smt(prog_id, p1, 6);
  // graph info
  // nodes:
  //  0[0:1] 1[2:3] 2[4:5]
  // edges:
  //  0 in:4294967295  out:1 2  // 4294967295 means -1
  //  1 in:0  out:2
  //  2 in:1 0  out:
  smt_stack s0, s1, s21, s22;
  s0.add(v("r_0_0_10_0") + to_expr(-1), v("r_0_0_1_0").extract(7, 0));
  bool res = (s0 == ps.post_stack_write_table[0][0]);
  s1 = s0;
  s1.add(v("r_0_1_10_0") + to_expr(-1), v("r_0_1_1_1").extract(7, 0));
  res = res && (s1 == ps.post_stack_write_table[1][0]);
  s21 = s1;
  s22 = s0;
  res = res && (s21 == ps.post_stack_write_table[2][0]) && (s22 == ps.post_stack_write_table[2][1]);
  print_test_res(res, "post stack_write_table 1");

  // test jmp 0
  inst p2[5] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 0, 1, 0),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  pl = ps.gen_smt(prog_id, p2, 5);
  // nodes:
  //  0[0:1] 1[2:4]
  // edges:
  //  0 in:4294967295  out:1 1 // 4294967295 means -1
  //  1 in:0 0  out:
  smt_stack s;
  s.add(v("r_0_0_10_0") + to_expr(-1), v("r_0_0_1_0").extract(7, 0));
  res = (s == ps.post_stack_write_table[0][0]);
  s.add(v("r_0_1_10_0") + to_expr(-1), v("r_0_1_1_0").extract(7, 0));
  res = res &&
        (s == ps.post_stack_write_table[1][0]) &&
        (s == ps.post_stack_write_table[1][1]);
  print_test_res(res, "post stack_write_table 2");

  // test jmp 0 with other jmps
  inst p3[8] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 1, 2, 2), // jmp case 1, r1 == r2
                inst(STXB, 10, -1, 2),
                inst(JEQXY, 1, 3, 2), // jmp case 2, r1 == r3
                inst(STXB, 10, -1, 3),
                inst(JEQXY, 1, 4, 0), // jmp case 3, r1 == r4
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  pl = ps.gen_smt(prog_id, p3, 8);
  // nodes:
  //  0[0:1] 1[2:3] 2[4:5] 3[6:7]
  // edges:
  //  0 in:4294967295  out:1 2
  //  1 in:0  out:2 3
  //  2 in:1 0  out:3 3
  //  3 in:2 2 1  out:
  // test the post_stack_write_table of basic block 3
  vector<expr> w_addr = {v("r_0_0_10_0") + to_expr(-1), // write in basic block 0
                         v("r_0_1_10_0") + to_expr(-1), // write in basic block 1
                         v("r_0_2_10_0") + to_expr(-1), // write in basic block 2
                        };
  vector<expr> w_val = {v("r_0_0_1_0").extract(7, 0),
                        v("r_0_1_2_0").extract(7, 0),
                        v("r_0_2_3_0").extract(7, 0),
                       };
  // for basic block 3, paths:
  // case1: from 2: 0 -> 1 -> 2 -> 3, 0 -> 2 -> 3,
  // case2: from 2: 0 -> 1 -> 2 -> 3, 0 -> 2 -> 3,
  // case3: from 1: 0 -> 1 -> 3
  // case1 and 2 are the same, can be tested by the same smt_stack
  s.clear();
  // 0 -> 1 -> 2 -> 3
  s.add(w_addr[0], w_val[0]); s.add(w_addr[1], w_val[1]); s.add(w_addr[2], w_val[2]);
  res = (s == ps.post_stack_write_table[3][0]) && (s == ps.post_stack_write_table[3][2]);
  // 0 -> 2 -> 3
  s.clear(); 
  s.add(w_addr[0], w_val[0]); s.add(w_addr[2], w_val[2]);
  res = res && (s == ps.post_stack_write_table[3][1]) && (s == ps.post_stack_write_table[3][3]);
  // 0 -> 1 -> 3
  s.clear();
  s.add(w_addr[0], w_val[0]); s.add(w_addr[1], w_val[1]);
  res = res && (s == ps.post_stack_write_table[3][4]);
  print_test_res(res, "post stack_write_table 3");  
}

int main() {
  test1();

  return 0;
}
