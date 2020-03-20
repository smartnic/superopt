#include "smt_var.h"
#include "../../src/utils.h"

using namespace std;

void test1() {
  smt_wt s1;
  s1.add(string_to_expr("a"), string_to_expr("b"));
  smt_wt s2 = s1;
  z3::expr f = (s1.addr[0] == s2.addr[0]) && (s1.val[0] == s2.val[0]);
  print_test_res(f.simplify() == string_to_expr("true"), "smt_wt=");

  // test smt_wt ==
  smt_wt s3 = s2;
  smt_wt s4 = s2;
  print_test_res(s1 == s2, "smt_wt == 1");
  s1.add(string_to_expr("a1"), string_to_expr("b1"));
  print_test_res(!(s1 == s2), "smt_wt == 2");
  s2.add(string_to_expr("a1"), string_to_expr("b1"));
  print_test_res(s1 == s2, "smt_wt == 3");
  s3.add(string_to_expr("a2"), string_to_expr("b1"));
  print_test_res(!(s1 == s3), "smt_wt == 4");
  s4.add(string_to_expr("a1"), string_to_expr("b2"));
  print_test_res(!(s1 == s4), "smt_wt == 5");
}

int main() {
  test1();

  return 0;
}
