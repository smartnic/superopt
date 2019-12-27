#include <iostream>
#include "../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

z3::context ctx;

void test1() {
  cout << "Test 1" << endl;
  int a = 4, b = 5, c = 10;
  z3::expr x = ctx.int_val(a);
  z3::expr y = ctx.int_val(b);
  z3::expr z = ctx.int_val(c);

  // check add
  print_test_res(compute_add(a, b, c) == (a + b), "compute_add");
  z3::expr expected = (z == x + y);
  print_test_res(predicate_add(x, y, z) == expected, "predicate_add");

  // check mov
  print_test_res(compute_mov(a, b) == a, "compute_mov");
  expected = (y == x);
  print_test_res(predicate_mov(x, y) == expected, "predicate_mov");

  // check max
  print_test_res(compute_max(a, b, c) == max(a, b), "compute_max");
  expected = ((x > a) && (z == x)) || ((x <= a) && (z == a));
  print_test_res(predicate_max(x, y, z) == expected, "predicate_max 1");
  expected = ((x > y) && (z == x)) || ((x <= y) && (z == y));
  print_test_res(predicate_max(x, a, z) == expected, "predicate_max 2");
}

int main() {
  test1();
  return 0;
}
