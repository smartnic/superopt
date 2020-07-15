#include <iostream>
#include <cstdint>
#include "../../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

#define v(x) to_expr(x, 32)

bool is_valid(z3::expr smt, bool counterex_print = 0) {
  // use bv tactic to accelerate
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(!smt);
  switch (s.check()) {
    case z3::unsat: return true;
    case z3::sat: {
      z3::model m = s.get_model();
      if (counterex_print) cout << m << endl;
      return false;
    }
    case z3::unknown:
      cout << "ERROR: smt is_valid unknown" << endl;
      return false;
  }
}

void test1() {
  cout << "Test 1: ALU operations" << endl;
  int32_t a = 5, b = 11, c = 0x0010203, z = ~0;

  z = compute_mov(a);
  // cout << z << endl;
  print_test_res(z == a, "compute_mov");
  z3::expr pred = predicate_mov(v(a), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_mov match compute_mov");

  z = compute_subtract(b, a);
  print_test_res(z == b-a, "compute_subtract");
  pred = predicate_subtract(v(b), v(a), v(z));
  print_test_res(is_valid(pred), "predicate_subtract match compute_subtract");

  z = compute_add16(a, c);
  // cout << z << endl;
  print_test_res(z == a + L16(c), "compute_add16");
  pred = predicate_add16(v(a), v(c), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_add16 match compute_mov");

  z = compute_add8(a, c);
  // cout << z << endl;
  print_test_res(z == a + L8(c), "compute_add8");
  pred = predicate_add8(v(a), v(c), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_add8 match compute_mov");

  z = compute_inv(c);
  // cout << z << endl;
  print_test_res(z == ~c, "compute_inv");
  pred = predicate_inv(v(c), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_inv match compute_inv");

  z = compute_and(a, b);
  // cout << z << endl;
  print_test_res(z == (a & b), "compute_and");
  pred = predicate_and(v(a), v(b), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_and match compute_and");

  z = compute_inv_and(a, b);
  // cout << z << endl;
  print_test_res(z == (~a & b), "compute_inv_and");
  pred = predicate_inv_and(v(a), v(b), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_inv_and match compute_inv_and");

  z = compute_or(a, b);
  // cout << z << endl;
  print_test_res(z == (a | b), "compute_or");
  pred = predicate_or(v(a), v(b), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_or match compute_or");

  z = compute_xor(a, b);
  // cout << z << endl;
  print_test_res(z == (a ^ b), "compute_xor");
  pred = predicate_xor(v(a), v(b), v(z));
  // cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_xor match compute_xor");
}


void test2() {
  cout << "Test 2: experimenting with z3 addition overflow" << endl;
  uint32_t x = UINT32_MAX, y = 1;
  uint32_t z = x + y;
  z3::expr pred = predicate_add(v(x), v(y), v(z));
  cout << pred << endl;
  print_test_res(is_valid(pred), "bit vector sum overflows");
}

int main() {
 cout << "=== Verification tests for Netronome ISA ===" << endl;
 // These test make sure that the metaprogramatically generated code for interpreting and verifying each instruction agree with each other
 test1();
 test2();
 return 0;
}
