#include <iostream>
#include <sstream>
#include "../../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

bool is_valid(z3::expr smt) {
  // use bv tactic to accelerate
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
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
  cout << "Test 5: Memory st/ld check" << endl;
  vector<z3::expr> offs = {v(0), v(1), v(2), v(3), v(4), v(5), v(6), v(7)};
  vector<uint8_t> vals = {0x12, 0x34};
  z3::expr addr = v((uint64_t)0xff12000000001234);
  mem_layout m_layout;
  m_layout._stack.set_range(addr, addr + 511);
  smt_mem m;
  smt_wt *s = &m._mem_table._wt;
  // (write addr+off, 8, in, s)
  // out == (read addr+off, 8, s)
  predicate_st8(v(vals[0]), addr, offs[0], m);
  z3::expr smt = (s->addr[0] == (addr + offs[0])) && (s->val[0] == to_expr(vals[0], 8));
  print_test_res(is_valid(smt), "predicate_st8 1");
  smt = predicate_ld8(addr, offs[0], m, v(vals[0]), m_layout);
  print_test_res(is_valid(smt), "predicate_ld8 1");

  predicate_st8(v(vals[1]), addr, offs[1], m);
  smt = (s->addr[1] == (addr + offs[1])) && (s->val[1] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 2");
  smt = predicate_ld8(addr, offs[1], m, v(vals[1]), m_layout);
  print_test_res(is_valid(smt), "predicate_ld8 2");

  // test rewrite
  predicate_st8(v(vals[1]), addr, offs[0], m);
  smt = (s->addr[2] == (addr + offs[0])) && (s->val[2] == to_expr(vals[1], 8));
  print_test_res(is_valid(smt), "predicate_st8 3");
  smt = predicate_ld8(addr, offs[0], m, v(vals[1]), m_layout);
  print_test_res(is_valid(smt), "predicate_ld8 3");
  s->clear();

  // test st16 && ld16
  uint64_t x = 0x0123456789abcdef;
  predicate_st16(v(x), addr, offs[0], m);
  smt = predicate_ld8(addr, offs[0], m, v(x & 0xff), m_layout) &&
        predicate_ld8(addr, offs[1], m, v((x >> 8) & 0xff), m_layout);
  print_test_res(is_valid(smt), "predicate_st16/ld8");
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st16/ld16");
  s->clear();
  predicate_st8(v(x), addr, offs[0], m);
  predicate_st8(v(x >> 8), addr, offs[1], m);
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st8/ld16");
  s->clear();

  // test st32 && ld32
  predicate_st32(v(x), addr, offs[0], m);
  smt = predicate_ld16(addr, offs[0], m, v(x & 0xffff), m_layout) &&
        predicate_ld16(addr, offs[2], m, v((x >> 16) & 0xffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st32/ld16");
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st32/ld32");
  s->clear();
  predicate_st16(v(x), addr, offs[0], m);
  predicate_st16(v(x >> 16), addr, offs[2], m);
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st16/ld32");
  s->clear();

  // test st64 && ld64
  predicate_st64(v(x), addr, offs[0], m);
  smt = predicate_ld32(addr, offs[0], m, v(x & 0xffffffff), m_layout) &&
        predicate_ld32(addr, offs[4], m, v((x >> 32) & 0xffffffff), m_layout);
  print_test_res(is_valid(smt), "predicate_st64/ld32");
  smt = predicate_ld64(addr, offs[0], m, v(x), m_layout);
  print_test_res(is_valid(smt), "predicate_st64/ld64");
  s->clear();
  predicate_st32(v(x), addr, offs[0], m);
  predicate_st32(v(x >> 32), addr, offs[4], m);
  smt = predicate_ld64(addr, offs[0], m, v(x), m_layout);
  print_test_res(is_valid(smt), "predicate_st32/ld64");
  s->clear();

  // safety check
  smt = predicate_ld8(NULL_ADDR, v(0), m, v(x), m_layout);
  print_test_res(is_valid(smt == string_to_expr("false")), "safety check when ld from NULL_ADDR");
}

void test6() {
  cout << "Test 6: Memory output equivalence check" << endl;
  mem_layout m_layout;
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout._stack.set_range(stack_s, stack_e);
  mem_wt x, y;
  // case 1.x: x is the same as y
  z3::expr expected = string_to_expr("true");
  print_test_res(is_valid(smt_stack_eq_chk(x._wt, y._wt, m_layout._stack) == expected), "stack output 1.1");
  x._wt.add(stack_s, v("v1"));
  y._wt = x._wt;
  expected = string_to_expr("true");
  print_test_res(is_valid(smt_stack_eq_chk(x._wt, y._wt, m_layout._stack) == expected), "stack output 1.2");
  x._wt.add(stack_s + 1, v("v2"));
  x._wt.add(stack_s + 2, v("v3"));
  y._wt = x._wt;
  expected = string_to_expr("true");
  print_test_res(is_valid(smt_stack_eq_chk(x._wt, y._wt, m_layout._stack) == expected), "stack output 1.3");
  y._wt.add(stack_s + 3, v("v4"));
  expected = string_to_expr("false");
  print_test_res(is_valid(smt_stack_eq_chk(x._wt, y._wt, m_layout._stack) == expected), "stack output 1.4");
}

void test7() {
  cout << "Test 7: Uninitialized read in ld" << endl;
  mem_layout m_layout;
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout._stack.set_range(stack_s, stack_e);
  z3::expr map_s = stack_e + 1;
  z3::expr map_e = stack_e + 512;
  m_layout.add_map(map_s, map_e);
  smt_mem m;
  z3::expr v1 = v(0xff);

  // stack safety check, read before write within stack address range implies "false"
  print_test_res(is_valid(predicate_ld8(stack_s, v(0), m, v1, m_layout) == string_to_expr("false")),
                 "safety check 1");
  predicate_st8(v1, stack_s, v(0), m);
  print_test_res(is_valid(predicate_ld8(stack_s, v(0), m, v1, m_layout) == string_to_expr("true")),
                 "safety check 2");
  print_test_res(is_valid(predicate_ld8(stack_s, v(8), m, v1, m_layout) == string_to_expr("false")),
                 "safety check 3");
  print_test_res(is_valid(predicate_ld8(stack_s, v(511), m, v1, m_layout) == string_to_expr("false")),
                 "safety check 4");
  print_test_res(is_valid(predicate_ld8(stack_s, v(512), m, v1, m_layout) == string_to_expr("true")),
                 "safety check 5");

  // test constrain on URT element (addr within map address range)
  // if addr cannot be found in the WT but found in URT,
  // the value in element is equal to the value(s) of the addr(s) in URT
  m.clear();
  predicate_ld8(map_s, v(0), m, v1, m_layout);
  print_test_res(is_valid(predicate_ld8(map_s, v(0), m, v1, m_layout) == string_to_expr("true")),
                 "URT element constrain 1");
  print_test_res(is_valid(predicate_ld8(map_s, v(0), m, v(0xfe), m_layout) == string_to_expr("false")),
                 "URT element constrain 2");
}

z3::expr new_out() {
  static int c = 0;
  string name = "out_" + to_string(c);
  c++;
  return v(name);
}

void test8() {
  cout << "Test 8: Map helper functions properties check" << endl;
  unsigned int prog_id = 0;
  unsigned int node_id = 0;
  unsigned int num_regs = 11;
  smt_var sv(prog_id, node_id, num_regs);
  mem_layout m_layout;
  // set memory layout: stack | map
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout._stack.set_range(stack_s, stack_e);
  z3::expr map_s = stack_e + 1;
  z3::expr map_e = stack_e + 512;
  m_layout.add_map(map_s, map_e);
  sv.mem_var.init_addrs_map_v_next(m_layout);

  cout << "  1. test properties of lookup after update/delete in map" << endl;
  // test *(lookup &k1 (update &k1 &v1 m))
  z3::expr k1 = to_expr("k1", 8);
  z3::expr v1 = to_expr("v1", 8);
  z3::expr addr_k1 = stack_s;
  z3::expr addr_v1 = stack_s + 1;
  z3::expr v_lookup_1 = v("v_lookup_1");
  z3::expr addr_v_lookup_1 = v("addr_v_lookup_1");
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(v1, addr_v1, v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  z3::expr f = predicate_map_update_helper(map_s, addr_k1, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s, addr_k1, addr_v_lookup_1, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_1, v(0), sv.mem_var, v_lookup_1, m_layout);
  z3::expr f_expected = z3::implies(f, v_lookup_1.extract(7, 0) == v1);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k1 &v1 m)) == v1");

  // test *(lookup &k1 (update &k2 &v2 (update &k1 &v1 m)))
  z3::expr k2 = to_expr("k2", 8);
  z3::expr v2 = to_expr("v2", 8);
  z3::expr addr_k2 = stack_s + 2;
  z3::expr addr_v2 = stack_s + 3;
  predicate_st8(k2, addr_k2, v(0), sv.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv.mem_var);
  z3::expr addr_map_v2 = v("addr_map_v2");
  z3::expr v_lookup_2 = v("v_lookup_2");
  z3::expr addr_v_lookup_2 = v("addr_v_lookup_2");
  f = f && predicate_map_update_helper(map_s, addr_k2, addr_v2, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s, addr_k1, addr_v_lookup_2, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_2, v(0), sv.mem_var, v_lookup_2, m_layout);
  f_expected = z3::implies(f && (k2 != k1), v_lookup_2.extract(7, 0) == v1);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k2 &v2 (update &k1 &v1 m))) == v1, if k2 != k1");
  f_expected = z3::implies(f && (k2 == k1), v_lookup_2.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k2 &v2 (update &k1 &v1 m))) == v2, if k2 == k1");

  // test *(lookup &k2 (update &k2 &v2 (update &k1 &v1 m)))
  z3::expr v_lookup_3 = v("v_lookup_3");
  z3::expr addr_v_lookup_3 = v("addr_v_lookup_3");
  f = f && predicate_map_lookup_helper(map_s, addr_k2, addr_v_lookup_3, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_3, v(0), sv.mem_var, v_lookup_3, m_layout);
  f_expected = z3::implies(f, v_lookup_3.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "*(lookup &k2 (update &k2 &v2 (update &k1 &v1 m))) == v2");

  // test *(lookup &k2 (delete &k2 (update &k2 &v2 (update &k1 &v1 m))))
  z3::expr addr_v_lookup_4 = v("addr_v_lookup_4");
  f = f && predicate_map_delete_helper(map_s, addr_k2, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s, addr_k2, addr_v_lookup_4, sv, m_layout);
  f_expected = z3::implies(f, addr_v_lookup_4 == NULL_ADDR);
  print_test_res(is_valid(f_expected), "lookup &k2 (delete &k2 (update &k2 &v2 (update &k1 &v1 m))) == NULL");

  // test *(lookup &k1 (delete &k2 (update &k2 &v2 (update &k1 &v1 m)))), if k2 != k1
  z3::expr v_lookup_5 = v("v_lookup_5");
  z3::expr addr_v_lookup_5 = v("addr_v_lookup_5");
  f = f && predicate_map_lookup_helper(map_s, addr_k1, addr_v_lookup_5, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_5, v(0), sv.mem_var, v_lookup_5, m_layout);
  f_expected = z3::implies(f && (k1 != k2), v_lookup_5.extract(7, 0) == v1);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (delete &k2 (update &k2 &v2 (update &k1 &v1 m)))) == v1, if k2 != k1");

  // test ur
  cout << "  2. test properties of uninitialized lookup in map" << endl;
  sv.clear();
  sv.mem_var.init_addrs_map_v_next(m_layout);
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(k2, addr_k2, v(0), sv.mem_var); // *addr_k2 = k2 (addr_k2 in the stack)
  z3::expr v_lookup_6 = v("v_lookup_6");
  z3::expr addr_v_lookup_6 = v("addr_v_lookup_6");
  z3::expr v_lookup_7 = v("v_lookup_7");
  z3::expr addr_v_lookup_7 = v("addr_v_lookup_7");
  z3::expr f1 = predicate_map_lookup_helper(map_s, addr_k1, addr_v_lookup_6, sv, m_layout);
  z3::expr f2 = predicate_map_lookup_helper(map_s, addr_k2, addr_v_lookup_7, sv, m_layout);
  z3::expr f3 = predicate_ld8(addr_v_lookup_6, v(0), sv.mem_var, v_lookup_6, m_layout);
  z3::expr f4 = predicate_ld8(addr_v_lookup_7, v(0), sv.mem_var, v_lookup_7, m_layout);

  cout << "a. address range" << endl;
  f_expected = z3::implies(f1 && (addr_v_lookup_6 != NULL_ADDR),
                           (uge(addr_v_lookup_6, map_s) && uge(map_e, addr_v_lookup_6)));
  print_test_res(is_valid(f_expected), "a = lookup &k1 m, a != NULL => a in map range");

  f_expected = z3::implies(f1 && (!(uge(addr_v_lookup_6, map_s) && uge(map_e, addr_v_lookup_6))),
                           addr_v_lookup_6 == NULL_ADDR);
  print_test_res(is_valid(f_expected), "a = lookup &k1 m, a not in map range => a == NULL");

  f_expected = z3::implies(f1 && f2 && (addr_v_lookup_7 != NULL_ADDR),
                           (uge(addr_v_lookup_7, map_s) && uge(map_e, addr_v_lookup_7)));
  print_test_res(is_valid(f_expected), "a1 = lookup &k1 m, a2 = lookup &k2 m, "\
                 "a2 != NULL => a2 in map range");

  f_expected = z3::implies(f1 && f2 && (uge(addr_v_lookup_7, map_s) && uge(map_e, addr_v_lookup_7)),
                           (addr_v_lookup_7 != NULL_ADDR));
  print_test_res(is_valid(f_expected), "a1 = lookup &k1 m, a2 = lookup &k2 m, "\
                 "a2 not in map range => a2 == NULL");

  cout << "b. address equivalence" << endl;
  f_expected = z3::implies(f1 && f2 && (k1 == k2), addr_v_lookup_6 == addr_v_lookup_7);
  print_test_res(is_valid(f_expected), "lookup &k1 m == lookup &k2 m, if k1 == k2");

  cout << "c. address uniqueness" << endl;
  f_expected = z3::implies(f1 && f2 && (k1 != k2) &&
                           (addr_v_lookup_6 != NULL_ADDR) &&
                           (addr_v_lookup_7 != NULL_ADDR),
                           addr_v_lookup_6 != addr_v_lookup_7);
  print_test_res(is_valid(f_expected), "lookup &k1 m != lookup &k2 m, if k1 != k2, k1 and k2 are in the map");

  cout << "d. value equivalence" << endl;
  f_expected = z3::implies(f1 && f2 && f3 && f4 && (k1 == k2) && (addr_v_lookup_6 != NULL_ADDR),
                           v_lookup_6 == v_lookup_7);

  print_test_res(is_valid(f_expected), "*(lookup &k1 m) == *(lookup &k2 m), " \
                 "if k1 == k2 and k1 is in map");

  cout << "  3. test operations with map helper functions and memory ld/st" << endl;
  sv.clear();
  sv.mem_var.init_addrs_map_v_next(m_layout);
  z3::expr p1 = v("p1");
  z3::expr p2 = v("p2");
  z3::expr v_p1 = v("v_p1");
  z3::expr v_p2 = v("v_p2");
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(v1, addr_v1, v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  f = predicate_map_update_helper(map_s, addr_k1, addr_v1, new_out(), sv, m_layout); // m[k1] = v1
  f = f && predicate_map_lookup_helper(map_s, addr_k1, p1, sv, m_layout); // p1 = &m[k1]
  f = f && predicate_map_lookup_helper(map_s, addr_k1, p2, sv, m_layout); // p2 = &m[k1]
  predicate_st8(v2, p1, v(0), sv.mem_var); // modify the map[k1] by p1
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout);
  f_expected = z3::implies(f && (v1 != v2), v_p2.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "test 1");

  cout << "  4. test properties of mutiple maps" << endl;;
  sv.clear();
  z3::expr map_s_2 = map_e + 1;
  z3::expr map_e_2 = map_e + 512;
  m_layout.add_map(map_s_2, map_e_2);
  sv.mem_var.init_addrs_map_v_next(m_layout);
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(k2, addr_k2, v(0), sv.mem_var); // *addr_k2 = k2 (addr_k2 in the stack)
  predicate_st8(v1, addr_v1, v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  predicate_st8(v2, addr_v2, v(0), sv.mem_var); // *addr_v2 = v2 (addr_v2 in the stack)
  f = predicate_map_update_helper(map_s, addr_k1, addr_v1, new_out(), sv, m_layout); // m1[k1] = v1
  f = f && predicate_map_update_helper(map_s_2, addr_k1, addr_v2, new_out(), sv, m_layout); // m2[k1] = v2
  f = f && predicate_map_lookup_helper(map_s, addr_k1, p1, sv, m_layout); // p1 = &m1[k1]
  f = f && predicate_ld8(p1, v(0), sv.mem_var, v_p1, m_layout); // v_p1 = *p1
  f = f && predicate_map_lookup_helper(map_s_2, addr_k1, p2, sv, m_layout); // p2 = &m2[k1]
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout); // v_p2 = *p2
  f_expected = z3::implies(f, v_p1.extract(7, 0) == v1);// &&
  z3::implies(f, v_p2.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k1 &v1 m1)) == v1 and "\
                 "*(lookup &k1 (update &k1 &v2 m2)) == v2");
  f_expected = z3::implies(f, (uge(p1, map_s) && uge(map_e, p1))) &&
               z3::implies(f, (uge(p2, map_s_2) && uge(map_e_2, p2)));
  print_test_res(is_valid(f_expected), "lookup &k1 (update &k1 &v1 m1) in m1 range, "\
                 "lookup &k1 (update &k1 &v2 m2) in m2 range");
  z3::expr v3 = to_expr("v3", 8);
  predicate_st8(v3, p1, v(0), sv.mem_var); // modify the m1[k1] by p1
  v_p1 = v("v_p1_1");
  v_p2 = v("v_p2_1");
  f = f && predicate_ld8(p1, v(0), sv.mem_var, v_p1, m_layout);
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout);
  f_expected = z3::implies(f, (v_p1.extract(7, 0) == v3) && (v_p2.extract(7, 0) == v2));
  print_test_res(is_valid(f_expected), "modify m1[k1] not affect m2[k1]");

  z3::expr p3 = v("p3");
  z3::expr p4 = v("p4");
  z3::expr v_p4 = v("v_p4");
  f = f && predicate_map_delete_helper(map_s, addr_k1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s, addr_k1, p3, sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s_2, addr_k1, p4, sv, m_layout);
  f = f && predicate_ld8(p4, v(0), sv.mem_var, v_p4, m_layout);
  f_expected = z3::implies(f, (p3 == NULL_ADDR) && (p4 == p2) && (v_p4.extract(7, 0) == v2));
  print_test_res(is_valid(f_expected), "delete m1[k1] not affect m2[k1]");

  f = f && predicate_map_update_helper(map_s, addr_k2, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s_2, addr_k2, p2, sv, m_layout);
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout);
  f_expected = z3::implies(f && (k1 != k2) && (!(uge(p2, map_s_2) && uge(map_e_2, p2))), p2 == NULL_ADDR) &&
               z3::implies(f && (k1 != k2) && (p2 != NULL_ADDR), (uge(p2, map_s_2) && uge(map_e_2, p2)));
  print_test_res(is_valid(f_expected), "m1 update not affect m2 uninitialized lookup");

  z3::expr addr_k3 = stack_s + 4;
  z3::expr k3 = to_expr("k3", 8);
  predicate_st8(k3, addr_k3, v(0), sv.mem_var); // *addr_k3 = k3 (addr_k3 in the stack)
  f = f && predicate_map_lookup_helper(map_s, addr_k3, p1, sv, m_layout);
  f = f && predicate_map_lookup_helper(map_s_2, addr_k3, p2, sv, m_layout);
  f_expected = z3::implies(f && (k3 != k1) && (k3 != k2),
                           (p1 == NULL_ADDR) || ((uge(p1, map_s) && uge(map_e, p1)))) &&
               z3::implies(f && (k3 != k1) && (k3 != k2),
                           (p2 == NULL_ADDR) || ((uge(p2, map_s_2) && uge(map_e_2, p2))));
  print_test_res(is_valid(f_expected), "uninitialized lookup in m1 and m2 not affect each other");

  cout << "  5. test return value of delete" << endl;;
  sv.clear();
  sv.mem_var.init_addrs_map_v_next(m_layout);
  z3::expr out = new_out();
  predicate_st8(k1, addr_k1, v(0), sv.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv.mem_var);
  f = predicate_map_delete_helper(map_s, addr_k1, out, sv, m_layout);
  f_expected = z3::implies(f && (out != MAP_DEL_RET_IF_KEY_INEXIST), out == MAP_DEL_RET_IF_KEY_EXIST) &&
               z3::implies(f && (out != MAP_DEL_RET_IF_KEY_EXIST), out == MAP_DEL_RET_IF_KEY_INEXIST);
  print_test_res(is_valid(f_expected), "ret_val(delete &k m) == EXIST or INEXIST");

  f = f && predicate_map_update_helper(map_s, addr_k1, addr_v1, new_out(), sv, m_layout);
  out = new_out();
  f = f && predicate_map_delete_helper(map_s, addr_k1, out, sv, m_layout);
  f_expected = z3::implies(f, out == MAP_DEL_RET_IF_KEY_EXIST);
  print_test_res(is_valid(f_expected), "ret_val(delete &k (update &k &v (delete &k m))) == EXIST");

  out = new_out();
  f = f && predicate_map_delete_helper(map_s, addr_k1, out, sv, m_layout);
  f_expected = z3::implies(f, out == MAP_DEL_RET_IF_KEY_EXIST);

  f_expected = z3::implies(f && (out != MAP_DEL_RET_IF_KEY_INEXIST), out == MAP_DEL_RET_IF_KEY_EXIST) &&
               z3::implies(f && (out != MAP_DEL_RET_IF_KEY_EXIST), out == MAP_DEL_RET_IF_KEY_INEXIST);
  print_test_res(is_valid(f_expected), "ret_val(delete &k (delete &k (update &k &v (delete &k m)))) == EXIST or INEXIST");
}

z3::expr eval_output(z3::expr smt, z3::expr output) {
  // use bv tactic to accelerate
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(smt);
  if (s.check() == z3::sat) {
    z3::model m = s.get_model();
    return m.eval(output);
  }
  return string_to_expr("false");
}

z3::expr new_addr_v_lookup() {
  static int c = 0;
  string name = "addr_v_lookup_" + to_string(c);
  c++;
  return v(name);
}

z3::expr new_v_lookup() {
  static int c = 0;
  string name = "v_lookup_" + to_string(c);
  c++;
  return v(name);
}

#define MAP_LOOKUP_AND_LD(map, addr_k, v_expected) \
  addr_v_lookup = new_addr_v_lookup(); \
  v_lookup = new_v_lookup(); \
  f = f && predicate_map_lookup_helper(map, addr_k, addr_v_lookup, sv, m_layout); \
  f = f && predicate_ld8(addr_v_lookup, v(0), sv.mem_var, v_lookup, m_layout); \
  f_expected = (eval_output(f, v_lookup).extract(7, 0) == v_expected);

#define MAP_LOOKUP(map, addr_k, addr_v_expected) \
  addr_v_lookup = new_addr_v_lookup(); \
  f = f && predicate_map_lookup_helper(map, addr_k, addr_v_lookup, sv, m_layout); \
  f_expected = (eval_output(f, addr_v_lookup) == addr_v_expected);

void test9() {
  cout << "Test 9: Map helper functions evaluation check" << endl;
  unsigned int prog_id = 0, node_id = 0, num_regs = 11;
  smt_var sv(prog_id, node_id, num_regs);
  mem_layout m_layout;
  // set memory layout: stack | map1 | map2
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  z3::expr map1_s = stack_e + 1;
  z3::expr map1_e = stack_e + 512;
  z3::expr map2_s = map1_e + 1;
  z3::expr map2_e = map1_e + 512;
  m_layout.set_stack_range(stack_s, stack_e);
  m_layout.add_map(map1_s, map1_s);
  m_layout.add_map(map2_s, map2_e);
  sv.mem_var.init_addrs_map_v_next(m_layout);
  z3::expr k1 = to_expr(0x1, 8);
  z3::expr v1 = to_expr(0x11, 8);
  z3::expr k2 = to_expr(0x2, 8);
  z3::expr v2 = to_expr(0x22, 8);
  z3::expr addr_k1 = stack_s;
  z3::expr addr_v1 = stack_s + 1;
  z3::expr addr_k2 = stack_s + 2;
  z3::expr addr_v2 = stack_s + 3;
  z3::expr addr_v_lookup = string_to_expr("true");
  z3::expr v_lookup = string_to_expr("true");
  z3::expr f_expected = string_to_expr("true");
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(v1, addr_v1, v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  predicate_st8(k2, addr_k2, v(0), sv.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv.mem_var);

  cout << "1. test 1" << endl;
  cout << "m1_1 = update &k2 &v2 (update &k1 &v1 m1_0)" << endl;
  cout << "m2_1 = update &k2 &v1 (update &k1 &v2 m2_0)" << endl;
  z3::expr f = predicate_map_update_helper(map1_s, addr_k1, addr_v1, new_out(), sv, m_layout); // m1[k1] = v1
  f = f && predicate_map_update_helper(map1_s, addr_k2, addr_v2, new_out(), sv, m_layout); // m1[k2] = v2
  f = f && predicate_map_update_helper(map2_s, addr_k1, addr_v2, new_out(), sv, m_layout); // m2[k1] = v2
  f = f && predicate_map_update_helper(map2_s, addr_k2, addr_v1, new_out(), sv, m_layout); // m2[k2] = v1
  // check lookup m1[k1]
  MAP_LOOKUP_AND_LD(map1_s, addr_k1, v1)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(*(lookup &k1 m1_1)) == v1");
  // check lookup m1[k2]
  MAP_LOOKUP_AND_LD(map1_s, addr_k2, v2)
  print_test_res(f_expected == string_to_expr("true"), "eval(*(lookup &k2 m1_1)) == v2");
  // check lookup m2[k1]
  MAP_LOOKUP_AND_LD(map2_s, addr_k1, v2)
  print_test_res(f_expected == string_to_expr("true"), "eval(*(lookup &k1 m2_1)) == v2");
  // check lookup m2[k2]
  MAP_LOOKUP_AND_LD(map2_s, addr_k2, v1)
  print_test_res(f_expected == string_to_expr("true"), "eval(*(lookup &k2 m2_1)) == v1");

  cout << "2. test 2" << endl;
  cout << "m1_2 = delete &k2 m1_1" << endl;
  f = f && predicate_map_delete_helper(map1_s, addr_k1, new_out(), sv, m_layout); // del m1[k1]
  MAP_LOOKUP(map1_s, addr_k1, NULL_ADDR)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(lookup &k1 m1_2) == NULL");
  MAP_LOOKUP_AND_LD(map1_s, addr_k2, v2)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(*(lookup &k2 m1_2)) == v2");
  MAP_LOOKUP_AND_LD(map2_s, addr_k1, v2)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(*(lookup &k1 m2_1)) == v2");
  MAP_LOOKUP_AND_LD(map2_s, addr_k2, v1)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(*(lookup &k2 m2_1)) == v1");

  cout << "3. test 3" << endl;
  cout << "m1_3 = delete &k2 m1_2" << endl;
  cout << "m2_2 = delete &k2 (delete &k1 m2_1)" << endl;
  f = f && predicate_map_delete_helper(map1_s, addr_k2, new_out(), sv, m_layout); // del m1[k2]
  f = f && predicate_map_delete_helper(map2_s, addr_k1, new_out(), sv, m_layout); // del m2[k1]
  f = f && predicate_map_delete_helper(map2_s, addr_k2, new_out(), sv, m_layout); // del m2[k2]
  MAP_LOOKUP(map1_s, addr_k1, NULL_ADDR)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(lookup &k1 m1_3) == NULL");
  MAP_LOOKUP(map1_s, addr_k2, NULL_ADDR)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(lookup &k2 m1_3) == NULL");
  MAP_LOOKUP(map2_s, addr_k1, NULL_ADDR)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(lookup &k1 m2_2) == NULL");
  MAP_LOOKUP(map2_s, addr_k2, NULL_ADDR)
  print_test_res(is_valid(f_expected == string_to_expr("true")), "eval(lookup &k2 m2_2) == NULL");
}

#undef MAP_LOOKUP_AND_LD
#undef MAP_LOOKUP

int main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();

  return 0;
}
