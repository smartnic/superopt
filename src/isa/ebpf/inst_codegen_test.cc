#include <iostream>
#include <sstream>
#include "../../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

bool is_valid(z3::expr smt) {
  z3::solver s(smt_c);
  s.add(!smt);
  switch (s.check()) {
    case z3::unsat: return true;
    case z3::sat: {
      z3::model m = s.get_model();
      cout << m << endl;
      return false;
    }
    case z3::unknown:
      cout << "ERROR: smt is_valid unknown" << endl;
      return false;
  }
}

#define v(x) to_expr(x, 64)

void test1() {
  cout << "Test 1: ALU check" << endl;
  int64_t a = 4, b = 5, c = 10;
  int64_t x = -1, y = -1, z = 0;

  // check add
  c = compute_add(a, b);
  print_test_res(c == (a + b), "compute_add");
  print_test_res(is_valid(predicate_add(v(a), v(b), v(c))), "predicate_add match compute_add");

  // check add32
  z = compute_add32(x, y);
  print_test_res(z == L32(x + y), "compute_add32");
  print_test_res(is_valid(predicate_add32(v(x), v(y), v(z))), "predicate_add32 match compute_add32");

  // check mov
  c = compute_mov(a);
  print_test_res(c == a, "compute_mov");
  print_test_res(is_valid(predicate_mov(v(a), v(c))), "predicate_mov match compute_mov");

  z = compute_mov32(x);
  print_test_res(z == L32(x), "compute_mov32");
  print_test_res(is_valid(predicate_mov32(v(x), v(z))), "predicate_mov32 match compute_mov32");
}

void test2() {
  cout << "Test 2: LE and BE check" << endl;
  int64_t expected;
  int64_t a = 0x0123456789abcdef, b = 0;
  bool is_le = is_little_endian();

  b = compute_le16(a);
  if (is_le) expected = 0xcdef;
  else expected = 0xefcd;
  print_test_res(b == expected, "compute_le16");
  print_test_res(is_valid(predicate_le16(v(a), v(b))), "predicate_le16 match compute_le16");

  b = compute_le32(a);
  if (is_le) expected = 0x89abcdef;
  else expected = 0xefcdab89;
  print_test_res(compute_le32(a, b) == expected, "compute_le32");
  print_test_res(is_valid(predicate_le32(v(a), v(b))), "predicate_le32 match compute_le32");

  b = compute_le64(a);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(compute_le64(a, b) == expected, "compute_le64");
  print_test_res(is_valid(predicate_le64(v(a), v(b))), "predicate_le64 match compute_le64");

  b = compute_be16(a);
  if (is_le) expected = 0xefcd;
  else expected = 0xcdef;
  print_test_res(compute_be16(a, b) == expected, "compute_be16");
  print_test_res(is_valid(predicate_be16(v(a), v(b))), "predicate_be16 match compute_be16");

  b = compute_be32(a);
  if (is_le) expected = 0xefcdab89;
  else expected = 0x89abcdef;
  print_test_res(compute_be32(a, b) == expected, "compute_be32");
  print_test_res(is_valid(predicate_be32(v(a), v(b))), "predicate_be32 match compute_be32");

  b = compute_be64(a);
  if (is_le) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(compute_be64(a, b) == expected, "compute_be64");
  print_test_res(is_valid(predicate_be64(v(a), v(b))), "predicate_be64 match compute_be64");
}

void test3() {
  cout << "Test 3: SHIFT check" << endl;
  int64_t expected64, expected32;
  int64_t a = 0xfffffffffffffffb, b = 1, c = 0; // a = -5

  expected64 = 0xfffffffffffffff6; // expected64 = -10
  expected32 = 0xfffffff6; // expected32 = -10
  c = compute_lsh(a, b);
  print_test_res(c == expected64, "compute_lsh");
  print_test_res(is_valid(predicate_lsh(v(a), v(b), v(c))), "predicate_lsh match compute_lsh");
  c = compute_lsh32(a, b);
  print_test_res(c == expected32, "compute_lsh32");
  print_test_res(is_valid(predicate_lsh32(v(a), v(b), v(c))), "predicate_lsh32 match compute_lsh32");

  expected64 = 0x7ffffffffffffffd;
  expected32 = 0x7ffffffd;
  c = compute_rsh(a, b);
  print_test_res(c == expected64, "compute_rsh");
  print_test_res(is_valid(predicate_rsh(v(a), v(b), v(c))), "predicate_rsh match compute_rsh");
  c = compute_rsh32(a, b);
  print_test_res(c == expected32, "compute_rsh32");
  print_test_res(is_valid(predicate_rsh32(v(a), v(b), v(c))), "predicate_rsh32 match compute_rsh32");

  expected64 = 0xfffffffffffffffd; // expected64 = -3
  expected32 = 0xfffffffd; // expected32 = -3
  c = compute_arsh(a, b);
  print_test_res(c == expected64, "compute_arsh");
  print_test_res(is_valid(predicate_arsh(v(a), v(b), v(c))), "predicate_arsh match compute_arsh");
  c = compute_arsh32(a, b);
  print_test_res(c == expected32, "compute_arsh32");
  print_test_res(is_valid(predicate_arsh32(v(a), v(b), v(c))), "predicate_arsh32 match compute_arsh32");
}

string array_to_str(uint8_t* a, int len) {
  ostringstream oss;
  for (int i = 0; i < len; i++) {
    oss << hex << (uint)a[i];
  }
  return oss.str();
}

bool check_array_equal(uint8_t* a, uint8_t* b, int len) {
  for (int i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      cout << array_to_str(a, len) << " != " << array_to_str(b, len) << endl;
      return false;
    }
  }
  return true;
}

void test4() {
  cout << "Test 3: Memory check" << endl;
  uint8_t a[4];
  uint8_t expected[4] = {0xef, 0xcd, 0xab, 0x89};
  int64_t x = 0x0123456789abcdef;
  compute_st32(x, (uint64_t)a, 0);
  // store lower 32-bit of x into a, check whether a == expected
  print_test_res(check_array_equal(a, expected, 4), "compute st32");
  // load four bytes from expected, check whether the value == L32(x)
  print_test_res(compute_ld32((uint64_t)expected, 0) == L32(x), "compute ld32");

  z3::expr mem = to_constant_expr("mem");
  z3::expr mem_expected = store(mem, v((uint64_t)a), v(x).extract(7, 0));
  mem_expected = store(mem_expected, v((uint64_t)a + 1), v(x).extract(15, 8));
  mem_expected = store(mem_expected, v((uint64_t)a + 2), v(x).extract(23, 16));
  mem_expected = store(mem_expected, v((uint64_t)a + 3), v(x).extract(31, 24));
  z3::expr smt = predicate_st32(v(x), v((uint64_t)a), v(0), mem, mem_expected);
  print_test_res(is_valid(smt), "predicate_st32");
  smt = predicate_ld32(v((uint64_t)a), v(0), mem_expected, v(L32(x)));
  print_test_res(is_valid(smt), "predicate_ld32");
}

void test5() {
  cout << "Test 5: Stack check" << endl;
  vector<z3::expr> offs = {v(0), v(1)};
  vector<uint8_t> vals = {0x12, 0x34};
  z3::expr addr = v((uint64_t)0xff12000000001234);
  smt_stack s;
  // (write addr+off, 8, in, s)
  // out == (read addr+off, 8, s)
  predicate_st8(to_expr(vals[0], 8), addr, offs[0], s);
  z3::expr smt = (s.addr[0] == (addr + offs[0])) && (s.val[0] == to_expr(vals[0], 8));
  print_test_res(is_valid(smt), "predicate_st8 1");
  smt = predicate_ld8(addr, offs[0], s, to_expr(vals[0], 64));
  print_test_res(is_valid(smt), "predicate_ld8 1");

  predicate_st8(to_expr(vals[1], 8), addr, offs[1], s);
  smt = (s.addr[1] == (addr + offs[1])) && (s.val[1] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 2");
  smt = predicate_ld8(addr, offs[1], s, to_expr(vals[1], 64));
  print_test_res(is_valid(smt), "predicate_ld8 2");

  // test rewrite
  predicate_st8(to_expr(vals[1], 8), addr, offs[0], s);
  smt = (s.addr[2] == (addr + offs[0])) && (s.val[2] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 3");
  smt = predicate_ld8(addr, offs[0], s, to_expr(vals[1], 64));
  print_test_res(is_valid(smt), "predicate_ld8 3");
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();

  return 0;
}
