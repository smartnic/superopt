#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/inst_header.h"
#include "smt_prog.h"

using namespace z3;

static z3::expr v(string var) {
  return string_to_expr(var);
}
static z3::expr v(uint32_t x) {
  return to_expr(x, 32);
}

void test1() {
  std::cout << "test1: jump free block of instructions" << endl;
  
  inst p[] = {
    inst(IMMED, 5, 4), // immed[a5, 4]
    inst(IMMED, 16, 5), // immed[b0, 5]
    inst(IMMED, 6, 2), // immed[a6, 1]
    inst(ALU, 0, 5, ALU_PLUS, 16), // alu[a0, a5, +, b0] (a0 = 9)
    inst(ALU, 0, 0, ALU_MINUS, 6), // alu[a0, a0, -, a6] (a0 = 7)
  };
  
  smt_prog ps;
  unsigned int prog_id = 0;
  // statement for entire program
  z3::expr p_smt = ps.gen_smt(prog_id, p, sizeof(p)/sizeof(inst));
  // statement for zeroth basic block
  // z3::expr bl_smt = ps.bl[0];

  cout << "Program expression:" << endl;
  cout << p_smt << endl;
  print_test_res(ps.bl.size() == 1, "Correct number of basic blocks");
  
  z3::expr test_smt = v("output" + to_string(prog_id)) == v(7);
  print_test_res(is_smt_valid(implies(p_smt, test_smt)), "Verifies program gives correct output");

  test_smt = v("output" + to_string(prog_id)) == v(9);
  print_test_res(!is_smt_valid(implies(p_smt, test_smt)), "Does not verify program gives wrong output");
}

int main() {
  cout << "=== Program and block level verification tests ===" << endl;
  test1();
  return 0;
}
