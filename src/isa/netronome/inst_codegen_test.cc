#include <iostream>
#include "../../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

#define v(x) to_expr(x, 64)

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
	cout << "Test 1" << endl;
	int64_t a = 4, b = 5, c = 10, d = 0x00010101;

	c = compute_mov(a);
	cout << c << endl;
	print_test_res(c == a, "compute_mov");
	z3::expr pred = predicate_mov(v(a), v(c));
	cout << pred << endl;
	print_test_res(is_valid(pred), "predicate_mov match compute_mov");

	c = compute_add(a, b);
	cout << c << endl;
	print_test_res(c == (a+b), "compute_add");
	pred = predicate_add(v(a), v(b), v(c));
	cout << pred << endl;
	print_test_res(is_valid(pred), "predicate_add match compute_add");

  c = compute_add16(a, d, c);
  cout << c << endl;
  print_test_res(c == (a + L16(d)), "compute_add16");
  pred = predicate_add16(v(a), v(d), v(c));
  cout << pred << endl;
  print_test_res(is_valid(pred), "predicate_add16 match compute_add16");
}

int main() {
 cout << "=== Verification tests for Netronome ISA ===" << endl;
 // These test make sure that the metaprogramatically generated code for interpreting and verifying each instruction agree with each other
 test1();
 return 0;
}
