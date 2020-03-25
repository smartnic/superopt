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
  cout << "Test 4: Memory check" << endl;
  int64_t x = 0x0123456789abcdef;

  uint8_t a1[8] = {0};
  uint8_t expected1[8] = {0xef, 0, 0, 0, 0, 0, 0, 0};
  compute_st8(x, (uint64_t)a1, 0);
  print_test_res(check_array_equal(a1, expected1, 8), "compute st8");
  print_test_res(compute_ld8((uint64_t)expected1, 0) == (x & 0xff), "compute ld8");

  uint8_t a2[8] = {0};
  uint8_t expected2[8] = {0xef, 0xcd, 0, 0, 0, 0, 0, 0};
  compute_st16(x, (uint64_t)a2, 0);
  print_test_res(check_array_equal(a2, expected2, 8), "compute st16");
  print_test_res(compute_ld16((uint64_t)expected2, 0) == (x & 0xffff), "compute ld16");

  uint8_t a3[8] = {0};
  uint8_t expected3[8] = {0xef, 0xcd, 0xab, 0x89, 0, 0, 0, 0};
  compute_st32(x, (uint64_t)a3, 0);
  print_test_res(check_array_equal(a3, expected3, 8), "compute st32");
  print_test_res(compute_ld32((uint64_t)expected3, 0) == (x & 0xffffffff), "compute ld32");

  uint8_t a4[8] = {0};
  uint8_t expected4[8] = {0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01};
  compute_st64(x, (uint64_t)a4, 0);
  print_test_res(check_array_equal(a4, expected4, 8), "compute st64");
  print_test_res(compute_ld64((uint64_t)expected4, 0) == x, "compute ld64");
}

void test5() {
  cout << "Test 5: Stack check" << endl;
  vector<z3::expr> offs = {v(0), v(1), v(2), v(3), v(4), v(5), v(6), v(7)};
  vector<uint8_t> vals = {0x12, 0x34};
  z3::expr addr = v((uint64_t)0xff12000000001234);
  smt_mem m;
  smt_wt *s = &m._stack._wt;
  // (write addr+off, 8, in, s)
  // out == (read addr+off, 8, s)
  predicate_st8(v(vals[0]), addr, offs[0], m);
  z3::expr smt = (s->addr[0] == (addr + offs[0])) && (s->val[0] == to_expr(vals[0], 8));
  print_test_res(is_valid(smt), "predicate_st8 1");
  smt = predicate_ld8(addr, offs[0], m, v(vals[0]));
  print_test_res(is_valid(smt), "predicate_ld8 1");

  predicate_st8(v(vals[1]), addr, offs[1], m);
  smt = (s->addr[1] == (addr + offs[1])) && (s->val[1] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 2");
  smt = predicate_ld8(addr, offs[1], m, v(vals[1]));
  print_test_res(is_valid(smt), "predicate_ld8 2");

  // test rewrite
  predicate_st8(v(vals[1]), addr, offs[0], m);
  smt = (s->addr[2] == (addr + offs[0])) && (s->val[2] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 3");
  smt = predicate_ld8(addr, offs[0], m, v(vals[1]));
  print_test_res(is_valid(smt), "predicate_ld8 3");
  s->clear();

  // test st16 && ld16
  uint64_t x = 0x0123456789abcdef;
  predicate_st16(v(x), addr, offs[0], m);
  smt = predicate_ld8(addr, offs[0], m, v(x & 0xff)) &&
        predicate_ld8(addr, offs[1], m, v((x >> 8) & 0xff));
  print_test_res(is_valid(smt), "predicate_st16/ld8");
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff));
  print_test_res(is_valid(smt), "predicate_st16/ld16");
  s->clear();
  predicate_st8(v(x), addr, offs[0], m);
  predicate_st8(v(x >> 8), addr, offs[1], m);
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff));
  print_test_res(is_valid(smt), "predicate_st8/ld16");
  s->clear();

  // test st32 && ld32
  predicate_st32(v(x), addr, offs[0], m);
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff)) &&
        predicate_ld16(addr, offs[2], m, v((x >> 16) & 0xffff));
  print_test_res(is_valid(smt), "predicate_st32/ld16");
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff));
  print_test_res(is_valid(smt), "predicate_st32/ld32");
  s->clear();
  predicate_st16(v(x), addr, offs[0], m);
  predicate_st16(v(x >> 16), addr, offs[2], m);
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff));
  print_test_res(is_valid(smt), "predicate_st16/ld32");
  s->clear();

  // test st64 && ld64
  predicate_st64(v(x), addr, offs[0], m);
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff)) &&
        predicate_ld32(addr, offs[4], m, v((x >> 32) & 0xffffffff));
  print_test_res(is_valid(smt), "predicate_st64/ld32");
  smt = predicate_ld64(addr, offs[0], m, v(x));
  print_test_res(is_valid(smt), "predicate_st64/ld64");
  s->clear();
  predicate_st32(v(x), addr, offs[0], m);
  predicate_st32(v(x >> 32), addr, offs[4], m);
  smt = predicate_ld64(addr, offs[0], m, v(x));
  print_test_res(is_valid(smt), "predicate_st32/ld64");
  s->clear();
}

// z3::expr predicate_mem_eq_chk(mem_wt& x, mem_wt& y);
void test6() {
  cout << "Test 6: Memory output equivalence check" << endl;
  mem_wt x(false), y(false);
  // case 1.x: x._wt is the same as y._wt
  z3::expr expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 1.1");
  x._wt.add(v("a1"), v("v1"));
  y._wt = x._wt;
  expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 1.2");
  x._wt.add(v("a2"), v("v2"));
  x._wt.add(v("a3"), v("v3"));
  y._wt = x._wt;
  expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 1.3");

  // case 2.x: addrs in x._wt and in y._wt have different names,
  // test the logic of addrs_in_one_wt_not_allow_ur
  x.clear();
  y.clear();
  x._wt.add(v("a1"), v("v1"));
  expected = string_to_expr("false");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 2.1");
  y._wt.add(v("a2"), v("v2"));
  expected = (v("a1") == v("a2")) && (v("v1") == v("v2"));
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 2.2");
  x._wt.add(v("a2"), v("v2"));
  expected = (v("a1") == v("a2"));
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 2.3");
  y._wt.add(v("a1"), v("v1"));
  expected = z3::implies(v("a1") == v("a2"), v("v1") == v("v2"));
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 2.4");

  // case 3.x, allow read before write
  x._allow_ur = true;
  y._allow_ur = true;
  x.clear();
  y.clear();
  expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 3.1");
  x._wt.add(v("a1"), v("v1"));
  expected = string_to_expr("false");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 3.2");
  x._urt.add(v("a1"), v("v1"));
  expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 3.3");
  x._urt.clear();
  x._urt.add(v("a2"), v("v2"));
  expected = (v("a1") == v("a2")) && (v("v1") == v("v2"));
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 3.4");

  // case 4.x, test the property of _urt (uninitilized read table)
  x.clear();
  x._wt.add(v("a1"), v("v1"));
  // the property of _urt ensures that v("v1") == v("v2")
  x._urt.add(v("a1"), v("v1"));
  x._urt.add(v("a1"), v("v2"));
  y._wt.add(v("a1"), v("v2"));
  expected = string_to_expr("true");
  print_test_res(is_valid(predicate_mem_eq_chk(x, y) == expected), "memory output 4.1");
}

void test7() {
  cout << "Test 6: Uninitialized read in ld" << endl;
  smt_mem m;
  // modify to make stack allow read before write
  m._stack._allow_ur = true;
  m._stack._wt.add(v("a1"), v("v1"));
  // test ld: the address which is in the wt
  z3::expr f = predicate_ld_byte(v("a1"), v(0), m, v("v1"));
  print_test_res(is_valid(string_to_expr("true") == f), "uninitialized read 1");
  f = predicate_ld_byte(v("a2"), v(0), m, v("v2"));
  // test ld: the address may be in the wt, but not in the urt
  z3::expr f_expected = z3::implies(v("a1") == v("a2"), v("v1") == v("v2"));
  print_test_res(is_valid(f_expected == f), "uninitialized read 2");
  // test ld: the address may be in the wt, must be in the urt
  m._stack._urt.add(v("a2"), v("v2"));
  f = predicate_ld_byte(v("a2"), v(0), m, v("v2"));
  f_expected = z3::implies(v("a1") == v("a2"), v("v1") == v("v2"));
  print_test_res(is_valid(f_expected == f), "uninitialized read 3");
  // test ld: the address may be in the wt or in the urt
  f = predicate_ld_byte(v("a3"), v(0), m, v("v3"));
  f_expected = z3::implies(v("a1") == v("a3"), v("v1") == v("v3")) &&
               z3::implies((v("a1") != v("a3")) && (v("a2") == v("a3")), v("v2") == v("v3"));
  print_test_res(is_valid(f_expected == f), "uninitialized read 4");
}

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();

  return 0;
}
