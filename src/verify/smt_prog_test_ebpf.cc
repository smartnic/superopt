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
  print_test_res((s0 == ps.post_stack_write_table[0][0]), "post stack_write_table 1");
  s1 = s0;
  s1.add(v("r_0_1_10_0") + to_expr(-1), v("r_0_1_1_1").extract(7, 0));
  print_test_res((s1 == ps.post_stack_write_table[1][0]), "post stack_write_table 2");
  s21 = s1;
  s22 = s0;
  print_test_res((s21 == ps.post_stack_write_table[2][0]), "post stack_write_table 3");
  print_test_res((s22 == ps.post_stack_write_table[2][1]), "post stack_write_table 4");
}

int main() {
  test1();

  return 0;
}
