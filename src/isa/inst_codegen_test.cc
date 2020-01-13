#include <iostream>
#include "../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

z3::context ctx;

void test1() {
  cout << "Test 1" << endl;
  int32_t a = 4, b = 5, c = 10;
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

void test2() {
  cout << "Test 2" << endl;
  int64_t expected;
  int64_t a = 0x0123456789abcdef, b = 0;

  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0x2301456789abcdef;
  print_test_res(compute_le16(a, b) == expected, "compute_le16");
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0x6745230189abcdef;
  print_test_res(compute_le32(a, b) == expected, "compute_le32");
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(compute_le64(a, b) == expected, "compute_le64");

  if (is_little_endian()) expected = 0x0123456789abefcd;
  else expected = 0x0123456789abcdef;
  print_test_res(compute_be16(a, b) == expected, "compute_be16");
  if (is_little_endian()) expected = 0x01234567efcdab89;
  else expected = 0x0123456789abcdef;
  print_test_res(compute_be32(a, b) == expected, "compute_be32");
  if (is_little_endian()) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(compute_be64(a, b) == expected, "compute_be64");
}

int main() {
  test1();
  test2();

  return 0;
}
