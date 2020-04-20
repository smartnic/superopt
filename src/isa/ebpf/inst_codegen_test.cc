#include <iostream>
#include <sstream>
#include "../../../src/utils.h"
#include "inst_codegen.h"

using namespace std;

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

#define v(x) to_expr(x, 64)
#define v8(x) to_expr(x, 8)
#define v16(x) to_expr(x, 16)
#define v32(x) to_expr(x, 32)

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
  smt_mem_layout m_layout;
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

  // // safety check
  // smt = predicate_ld8(NULL_ADDR_EXPR, v(0), m, v(x), m_layout);
  // print_test_res(is_valid(smt == string_to_expr("false")), "safety check when ld from NULL_ADDR_EXPR");
}

void test6() {
  cout << "Test 6: Memory output equivalence check" << endl;
  smt_mem_layout m_layout;
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
  smt_mem_layout m_layout;
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout._stack.set_range(stack_s, stack_e);
  z3::expr map_s = stack_e + 1;
  z3::expr map_e = stack_e + 512;
  m_layout.add_map(map_s, map_e);
  smt_mem m;
  z3::expr v1 = v(0xff);

  // // stack safety check, read before write within stack address range implies "false"
  // print_test_res(is_valid(predicate_ld8(stack_s, v(0), m, v1, m_layout) == string_to_expr("false")),
  //                "safety check 1");
  // predicate_st8(v1, stack_s, v(0), m);
  // print_test_res(is_valid(predicate_ld8(stack_s, v(0), m, v1, m_layout) == string_to_expr("true")),
  //                "safety check 2");
  // print_test_res(is_valid(predicate_ld8(stack_s, v(8), m, v1, m_layout) == string_to_expr("false")),
  //                "safety check 3");
  // print_test_res(is_valid(predicate_ld8(stack_s, v(511), m, v1, m_layout) == string_to_expr("false")),
  //                "safety check 4");
  // print_test_res(is_valid(predicate_ld8(stack_s, v(512), m, v1, m_layout) == string_to_expr("true")),
  //                "safety check 5");

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

void test8() {
  cout << "Test 8: Map helper functions properties check" << endl;
  unsigned int prog_id = 0;
  unsigned int node_id = 0;
  unsigned int num_regs = 11;
  smt_var sv(prog_id, node_id, num_regs);
  smt_mem_layout m_layout;
  // set memory layout: stack | map
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout._stack.set_range(stack_s, stack_e);
  z3::expr map_s = stack_e + 1;
  z3::expr map_e = stack_e + 512;
  m_layout.add_map(map_s, map_e);
  z3::expr map1 = v(0);
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
  z3::expr f = predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map1, addr_k1, addr_v_lookup_1, sv, m_layout);
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
  f = f && predicate_map_update_helper(map1, addr_k2, addr_v2, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map1, addr_k1, addr_v_lookup_2, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_2, v(0), sv.mem_var, v_lookup_2, m_layout);
  f_expected = z3::implies(f && (k2 != k1), v_lookup_2.extract(7, 0) == v1);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k2 &v2 (update &k1 &v1 m))) == v1, if k2 != k1");
  f_expected = z3::implies(f && (k2 == k1), v_lookup_2.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k2 &v2 (update &k1 &v1 m))) == v2, if k2 == k1");

  // test *(lookup &k2 (update &k2 &v2 (update &k1 &v1 m)))
  z3::expr v_lookup_3 = v("v_lookup_3");
  z3::expr addr_v_lookup_3 = v("addr_v_lookup_3");
  f = f && predicate_map_lookup_helper(map1, addr_k2, addr_v_lookup_3, sv, m_layout);
  f = f && predicate_ld8(addr_v_lookup_3, v(0), sv.mem_var, v_lookup_3, m_layout);
  f_expected = z3::implies(f, v_lookup_3.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "*(lookup &k2 (update &k2 &v2 (update &k1 &v1 m))) == v2");

  // test *(lookup &k2 (delete &k2 (update &k2 &v2 (update &k1 &v1 m))))
  z3::expr addr_v_lookup_4 = v("addr_v_lookup_4");
  f = f && predicate_map_delete_helper(map1, addr_k2, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map1, addr_k2, addr_v_lookup_4, sv, m_layout);
  f_expected = z3::implies(f, addr_v_lookup_4 == NULL_ADDR_EXPR);
  print_test_res(is_valid(f_expected), "lookup &k2 (delete &k2 (update &k2 &v2 (update &k1 &v1 m))) == NULL");

  // test *(lookup &k1 (delete &k2 (update &k2 &v2 (update &k1 &v1 m)))), if k2 != k1
  z3::expr v_lookup_5 = v("v_lookup_5");
  z3::expr addr_v_lookup_5 = v("addr_v_lookup_5");
  f = f && predicate_map_lookup_helper(map1, addr_k1, addr_v_lookup_5, sv, m_layout);
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
  z3::expr f1 = predicate_map_lookup_helper(map1, addr_k1, addr_v_lookup_6, sv, m_layout);
  z3::expr f2 = predicate_map_lookup_helper(map1, addr_k2, addr_v_lookup_7, sv, m_layout);
  z3::expr f3 = predicate_ld8(addr_v_lookup_6, v(0), sv.mem_var, v_lookup_6, m_layout);
  z3::expr f4 = predicate_ld8(addr_v_lookup_7, v(0), sv.mem_var, v_lookup_7, m_layout);

  cout << "a. address range" << endl;
  f_expected = z3::implies(f1 && (addr_v_lookup_6 != NULL_ADDR_EXPR),
                           (uge(addr_v_lookup_6, map_s) && uge(map_e, addr_v_lookup_6)));
  print_test_res(is_valid(f_expected), "a = lookup &k1 m, a != NULL => a in map range");

  f_expected = z3::implies(f1 && (!(uge(addr_v_lookup_6, map_s) && uge(map_e, addr_v_lookup_6))),
                           addr_v_lookup_6 == NULL_ADDR_EXPR);
  print_test_res(is_valid(f_expected), "a = lookup &k1 m, a not in map range => a == NULL");

  f_expected = z3::implies(f1 && f2 && (addr_v_lookup_7 != NULL_ADDR_EXPR),
                           (uge(addr_v_lookup_7, map_s) && uge(map_e, addr_v_lookup_7)));
  print_test_res(is_valid(f_expected), "a1 = lookup &k1 m, a2 = lookup &k2 m, "\
                 "a2 != NULL => a2 in map range");

  f_expected = z3::implies(f1 && f2 && (uge(addr_v_lookup_7, map_s) && uge(map_e, addr_v_lookup_7)),
                           (addr_v_lookup_7 != NULL_ADDR_EXPR));
  print_test_res(is_valid(f_expected), "a1 = lookup &k1 m, a2 = lookup &k2 m, "\
                 "a2 not in map range => a2 == NULL");

  cout << "b. address equivalence" << endl;
  f_expected = z3::implies(f1 && f2 && (k1 == k2), addr_v_lookup_6 == addr_v_lookup_7);
  print_test_res(is_valid(f_expected), "lookup &k1 m == lookup &k2 m, if k1 == k2");

  cout << "c. address uniqueness" << endl;
  f_expected = z3::implies(f1 && f2 && (k1 != k2) &&
                           (addr_v_lookup_6 != NULL_ADDR_EXPR) &&
                           (addr_v_lookup_7 != NULL_ADDR_EXPR),
                           addr_v_lookup_6 != addr_v_lookup_7);
  print_test_res(is_valid(f_expected), "lookup &k1 m != lookup &k2 m, if k1 != k2, k1 and k2 are in the map");

  cout << "d. value equivalence" << endl;
  f_expected = z3::implies(f1 && f2 && f3 && f4 && (k1 == k2) && (addr_v_lookup_6 != NULL_ADDR_EXPR),
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
  f = predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout); // m[k1] = v1
  f = f && predicate_map_lookup_helper(map1, addr_k1, p1, sv, m_layout); // p1 = &m[k1]
  f = f && predicate_map_lookup_helper(map1, addr_k1, p2, sv, m_layout); // p2 = &m[k1]
  predicate_st8(v2, p1, v(0), sv.mem_var); // modify the map[k1] by p1
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout);
  f_expected = z3::implies(f && (v1 != v2), v_p2.extract(7, 0) == v2);
  print_test_res(is_valid(f_expected), "test 1");

  cout << "  4. test properties of mutiple maps" << endl;;
  sv.clear();
  z3::expr map_s_2 = map_e + 1;
  z3::expr map_e_2 = map_e + 512;
  z3::expr map2 = v(1);
  m_layout.add_map(map_s_2, map_e_2);
  sv.mem_var.init_addrs_map_v_next(m_layout);
  predicate_st8(k1, addr_k1, v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(k2, addr_k2, v(0), sv.mem_var); // *addr_k2 = k2 (addr_k2 in the stack)
  predicate_st8(v1, addr_v1, v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  predicate_st8(v2, addr_v2, v(0), sv.mem_var); // *addr_v2 = v2 (addr_v2 in the stack)
  f = predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout); // m1[k1] = v1
  f = f && predicate_map_update_helper(map2, addr_k1, addr_v2, new_out(), sv, m_layout); // m2[k1] = v2
  f = f && predicate_map_lookup_helper(map1, addr_k1, p1, sv, m_layout); // p1 = &m1[k1]
  f = f && predicate_ld8(p1, v(0), sv.mem_var, v_p1, m_layout); // v_p1 = *p1
  f = f && predicate_map_lookup_helper(map2, addr_k1, p2, sv, m_layout); // p2 = &m2[k1]
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
  f = f && predicate_map_delete_helper(map1, addr_k1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map1, addr_k1, p3, sv, m_layout);
  f = f && predicate_map_lookup_helper(map2, addr_k1, p4, sv, m_layout);
  f = f && predicate_ld8(p4, v(0), sv.mem_var, v_p4, m_layout);
  f_expected = z3::implies(f, (p3 == NULL_ADDR_EXPR) && (p4 == p2) && (v_p4.extract(7, 0) == v2));
  print_test_res(is_valid(f_expected), "delete m1[k1] not affect m2[k1]");

  f = f && predicate_map_update_helper(map1, addr_k2, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_lookup_helper(map2, addr_k2, p2, sv, m_layout);
  f = f && predicate_ld8(p2, v(0), sv.mem_var, v_p2, m_layout);
  f_expected = z3::implies(f && (k1 != k2) && (!(uge(p2, map_s_2) && uge(map_e_2, p2))), p2 == NULL_ADDR_EXPR) &&
               z3::implies(f && (k1 != k2) && (p2 != NULL_ADDR_EXPR), (uge(p2, map_s_2) && uge(map_e_2, p2)));
  print_test_res(is_valid(f_expected), "m1 update not affect m2 uninitialized lookup");

  z3::expr addr_k3 = stack_s + 4;
  z3::expr k3 = to_expr("k3", 8);
  predicate_st8(k3, addr_k3, v(0), sv.mem_var); // *addr_k3 = k3 (addr_k3 in the stack)
  f = f && predicate_map_lookup_helper(map1, addr_k3, p1, sv, m_layout);
  f = f && predicate_map_lookup_helper(map2, addr_k3, p2, sv, m_layout);
  f_expected = z3::implies(f && (k3 != k1) && (k3 != k2),
                           (p1 == NULL_ADDR_EXPR) || ((uge(p1, map_s) && uge(map_e, p1)))) &&
               z3::implies(f && (k3 != k1) && (k3 != k2),
                           (p2 == NULL_ADDR_EXPR) || ((uge(p2, map_s_2) && uge(map_e_2, p2))));
  print_test_res(is_valid(f_expected), "uninitialized lookup in m1 and m2 not affect each other");

  cout << "  5. test return value of delete" << endl;;
  sv.clear();
  sv.mem_var.init_addrs_map_v_next(m_layout);
  z3::expr out = new_out();
  predicate_st8(k1, addr_k1, v(0), sv.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv.mem_var);
  f = predicate_map_delete_helper(map1, addr_k1, out, sv, m_layout);
  f_expected = z3::implies(f && (out != MAP_DEL_RET_IF_KEY_INEXIST_EXPR), out == MAP_DEL_RET_IF_KEY_EXIST_EXPR) &&
               z3::implies(f && (out != MAP_DEL_RET_IF_KEY_EXIST_EXPR), out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR);
  print_test_res(is_valid(f_expected), "ret_val(delete &k m) == EXIST or INEXIST");

  f = f && predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout);
  out = new_out();
  f = f && predicate_map_delete_helper(map1, addr_k1, out, sv, m_layout);
  cout << "out: " << out << endl;
  f_expected = z3::implies(f, out == MAP_DEL_RET_IF_KEY_EXIST_EXPR);
  print_test_res(is_valid(f_expected, true), "ret_val(delete &k (update &k &v (delete &k m))) == EXIST");

  out = new_out();
  f = f && predicate_map_delete_helper(map1, addr_k1, out, sv, m_layout);
  f_expected = z3::implies(f, out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR);
  print_test_res(is_valid(f_expected), "ret_val(delete &k (delete &k (update &k &v (delete &k m)))) == INEXIST");

  cout << "  6. test size of k/v > 1 byte" << endl;
  sv.clear();
  map1 = v(0), map2 = v(1);
  m_layout.set_map_attr(map2, map_attr(16, 32)); // set map2 key size: 16 bits, value size: 32 bits
  sv.mem_var.init_addrs_map_v_next(m_layout);
  k1 = to_expr("k1", 8); // used by map1
  v1 = to_expr("v1", 8);
  k2 = to_expr("k2", 16); //used by map2
  v2 = to_expr("v2", 32);
  addr_k1 = stack_s;
  addr_v1 = stack_s + 1;
  addr_k2 = stack_s + 2;
  addr_v2 = stack_s + 4;
  z3::expr addr_v_lookup = string_to_expr("true");
  z3::expr v_lookup = string_to_expr("true");
  predicate_st8(k1, addr_k1, v(0), sv.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv.mem_var);
  predicate_st16(k2, addr_k2, v(0), sv.mem_var);
  predicate_st32(v2, addr_v2, v(0), sv.mem_var);

#define MAP1_LOOKUP_AND_LD(v_expected) \
  addr_v_lookup = new_addr_v_lookup(); \
  v_lookup = new_v_lookup(); \
  f = f && predicate_map_lookup_helper(map1, addr_k1, addr_v_lookup, sv, m_layout); \
  f = f && predicate_ld16(addr_v_lookup, v(0), sv.mem_var, v_lookup, m_layout); \
  f_expected = z3::implies(f, v_lookup.extract(7, 0) == v_expected);

#define MAP2_LOOKUP_AND_LD(v_expected) \
  addr_v_lookup = new_addr_v_lookup(); \
  v_lookup = new_v_lookup(); \
  f = f && predicate_map_lookup_helper(map2, addr_k2, addr_v_lookup, sv, m_layout); \
  f = f && predicate_ld32(addr_v_lookup, v(0), sv.mem_var, v_lookup, m_layout); \
  f_expected = z3::implies(f, v_lookup.extract(31, 0) == v_expected);

#define MAP_LOOKUP(map, addr_k, addr_v_expected) \
  addr_v_lookup = new_addr_v_lookup(); \
  v_lookup = new_v_lookup(); \
  f = f && predicate_map_lookup_helper(map, addr_k, addr_v_lookup, sv, m_layout); \
  f_expected = z3::implies(f, addr_v_lookup == addr_v_expected);

  f = predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_update_helper(map2, addr_k2, addr_v2, new_out(), sv, m_layout);
  MAP1_LOOKUP_AND_LD(v1)
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k1 &v1 m1)) == v1");
  MAP2_LOOKUP_AND_LD(v2)
  print_test_res(is_valid(f_expected), "*(lookup &k2 (update &k2 &v2 m2)) == v2");

  f = f && predicate_map_delete_helper(map1, addr_k1, new_out(), sv, m_layout);
  f = f && predicate_map_delete_helper(map2, addr_k2, new_out(), sv, m_layout);
  MAP_LOOKUP(map1, addr_k1, NULL_ADDR_EXPR)
  print_test_res(is_valid(f_expected), "lookup &k1 (delete &k1 (update &k1 &v1 m1)) == NULL_ADDR_EXPR");
  MAP_LOOKUP(map2, addr_k2, NULL_ADDR_EXPR)
  print_test_res(is_valid(f_expected), "lookup &k2 (delete &k2 (update &k2 &v2 m2)) == NULL_ADDR_EXPR");

  f = f && predicate_map_update_helper(map1, addr_k1, addr_v1, new_out(), sv, m_layout);
  f = f && predicate_map_update_helper(map2, addr_k2, addr_v2, new_out(), sv, m_layout);
  MAP1_LOOKUP_AND_LD(v1)
  print_test_res(is_valid(f_expected), "*(lookup &k1 (update &k1 &v1 (delete &k1 (update &k1 &v1 m1)))) == v1");
  MAP2_LOOKUP_AND_LD(v2)
  print_test_res(is_valid(f_expected), "*(lookup &k2 (update &k2 &v2 (delete &k2 (update &k2 &v2 m2)))) == v2");

#undef MAP1_LOOKUP_AND_LD
#undef MAP2_LOOKUP_AND_LD
#undef MAP_LOOKUP
}

z3::expr eval_output(z3::expr smt, z3::expr output) {
  // use bv tactic to accelerate
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(smt);
  if (s.check() == z3::sat) {
    z3::model m = s.get_model();
    return m.eval(output);
  } else {
    cout << "Error: not able to evaluate" << endl;
    return output;
  }
}

#define MAP_UPDATE(map, addr_k, addr_v) \
  compute_map_update_helper(map, addr_k, addr_v, m); \
  f = f && predicate_map_update_helper(v(map), v(addr_k), v(addr_v), new_out(), sv, m_layout);

#define MAP_DELETE(map, addr_k) \
  compute_map_delete_helper(map, addr_k, m); \
  f = f && predicate_map_delete_helper(v(map), v(addr_k), new_out(), sv, m_layout);

#define MAP_LOOKUP_AND_LD(map, addr_k, v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  v_lookup_expr = new_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map, addr_k, m); \
  v_lookup = compute_ld8(addr_v_lookup, 0); \
  f = f && predicate_map_lookup_helper(v(map), v(addr_k), addr_v_lookup_expr, sv, m_layout); \
  f = f && predicate_ld8(addr_v_lookup_expr, v(0), sv.mem_var, v_lookup_expr, m_layout); \
  f_expected = (eval_output(f, v_lookup_expr) == v(v_lookup)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (v_lookup == v_expected), test_name);

#define MAP_LOOKUP(map, addr_k, addr_v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map, addr_k, m); \
  f = f && predicate_map_lookup_helper(v(map), v(addr_k), addr_v_lookup_expr, sv, m_layout); \
  f_expected = (eval_output(f, addr_v_lookup_expr) == v(addr_v_lookup)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (addr_v_lookup == addr_v_expected), test_name);

mem_layout mem_t::_layout;

void test9() {
  cout << "Test 9: Map helper functions evaluation check" << endl;
  mem_t::add_map(map_attr(8, 8, 512)); // add two maps with 1-byte k/v size, 512 max entries
  mem_t::add_map(map_attr(8, 8, 512));
  mem_t m;
  m.init_mem_by_layout();
  int map1 = 0, map2 = 1;
  int64_t k1 = 0x1, v1 = 0x11;
  int64_t k2 = 0x2, v2 = 0x22;
  uint64_t stack_s = (uint64_t)m.get_stack_start_addr();
  uint64_t addr_k1 = stack_s, addr_v1 = stack_s + 1;
  uint64_t addr_k2 = stack_s + 2, addr_v2 = stack_s + 3;

  unsigned int prog_id = 0, node_id = 0, num_regs = 11;
  smt_var sv(prog_id, node_id, num_regs);
  smt_mem_layout m_layout;
  // set memory layout: stack | map1 | map2
  z3::expr stack_s_expr = v(stack_s);
  z3::expr stack_e_expr = stack_s_expr + 511;
  z3::expr map1_s_expr = stack_e_expr + 1;
  z3::expr map1_e_expr = stack_e_expr + 512;
  z3::expr map2_s_expr = map1_e_expr + 1;
  z3::expr map2_e_expr = map1_e_expr + 512;
  m_layout.set_stack_range(stack_s_expr, stack_e_expr);
  m_layout.add_map(map1_s_expr, map1_s_expr);
  m_layout.add_map(map2_s_expr, map2_e_expr);
  sv.mem_var.init_addrs_map_v_next(m_layout);

  uint64_t addr_v_lookup = 0;
  int64_t v_lookup = 0;
  z3::expr addr_v_lookup_expr = string_to_expr("true");
  z3::expr v_lookup_expr = string_to_expr("true");
  z3::expr f_expected = string_to_expr("true");
  cout << "  1 test lookup after deletes/updates" << endl;
  compute_st8(k1, addr_k1, 0); compute_st8(v1, addr_v1, 0);
  compute_st8(k2, addr_k2, 0); compute_st8(v2, addr_v2, 0);
  predicate_st8(v8(k1), v(addr_k1), v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(v8(v1), v(addr_v1), v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  predicate_st8(v8(k2), v(addr_k2), v(0), sv.mem_var);
  predicate_st8(v8(v2), v(addr_v2), v(0), sv.mem_var);
  cout << "1.1" << endl;
  cout << "m1_1 = update &k2 &v2 (update &k1 &v1 m1_0)" << endl;
  cout << "m2_1 = update &k2 &v1 (update &k1 &v2 m2_0)" << endl;

  z3::expr f = string_to_expr("true");
  MAP_UPDATE(map1, addr_k1, addr_v1) // m1[k1] = v1
  MAP_UPDATE(map1, addr_k2, addr_v2) // m1[k2] = v2
  MAP_UPDATE(map2, addr_k1, addr_v2) // m2[k1] = v2
  MAP_UPDATE(map2, addr_k2, addr_v1) // m2[k2] = v1
  // check lookup m1[k1]
  string test_name = "eval(*(lookup &k1 m1_1)) == v1";
  MAP_LOOKUP_AND_LD(map1, addr_k1, v1, test_name)
  // check lookup m1[k2]
  test_name = "eval(*(lookup &k2 m1_1)) == v2";
  MAP_LOOKUP_AND_LD(map1, addr_k2, v2, test_name)
  // check lookup m2[k1]
  test_name = "eval(*(lookup &k1 m2_1)) == v2";
  MAP_LOOKUP_AND_LD(map2, addr_k1, v2, test_name)
  // check lookup m2[k2]
  test_name = "eval(*(lookup &k2 m2_1)) == v1";
  MAP_LOOKUP_AND_LD(map2, addr_k2, v1, test_name)

  cout << "1.2" << endl;
  cout << "m1_2 = delete &k1 m1_1" << endl;
  MAP_DELETE(map1, addr_k1) // del m1[k1]
  test_name = "eval(lookup &k1 m1_2) == NULL";
  MAP_LOOKUP(map1, addr_k1, NULL_ADDR, test_name)
  test_name = "eval(*(lookup &k2 m1_2)) == v2";
  MAP_LOOKUP_AND_LD(map1, addr_k2, v2, test_name)
  test_name = "eval(*(lookup &k1 m2_1)) == v2";
  MAP_LOOKUP_AND_LD(map2, addr_k1, v2, test_name)
  test_name = "eval(*(lookup &k2 m2_1)) == v1";
  MAP_LOOKUP_AND_LD(map2, addr_k2, v1, test_name)

  cout << "1.3" << endl;
  cout << "m1_3 = delete &k2 m1_2" << endl;
  cout << "m2_2 = delete &k2 (delete &k1 m2_1)" << endl;
  MAP_DELETE(map1, addr_k2) // del m1[k2]
  MAP_DELETE(map2, addr_k1) // del m2[k1]
  MAP_DELETE(map2, addr_k2) // del m2[k2]
  test_name = "eval(lookup &k1 m1_3) == NULL";
  MAP_LOOKUP(map1, addr_k1, NULL_ADDR, test_name)
  test_name = "eval(lookup &k2 m1_3) == NULL";
  MAP_LOOKUP(map1, addr_k2, NULL_ADDR, test_name)
  test_name = "eval(lookup &k1 m2_2) == NULL";
  MAP_LOOKUP(map2, addr_k1, NULL_ADDR, test_name)
  test_name = "eval(lookup &k2 m2_2) == NULL";
  MAP_LOOKUP(map2, addr_k2, NULL_ADDR, test_name)

  cout << "1.4" << endl;
  cout << "m1_4 = update &k2 &v2 (update &k1 &v1 m1_3)" << endl;
  cout << "m2_3 = update &k2 &v1 (update &k1 &v2 m2_2)" << endl;
  MAP_UPDATE(map1, addr_k1, addr_v1)
  MAP_UPDATE(map1, addr_k2, addr_v2)
  MAP_UPDATE(map2, addr_k1, addr_v2)
  MAP_UPDATE(map2, addr_k2, addr_v1)
  // check lookup m1[k1]
  test_name = "eval(*(lookup &k1 m1_4)) == v1";
  MAP_LOOKUP_AND_LD(map1, addr_k1, v1, test_name)
  // check lookup m1[k2]
  test_name = "eval(*(lookup &k2 m1_4)) == v2";
  MAP_LOOKUP_AND_LD(map1, addr_k2, v2, test_name)
  // check lookup m2[k1]
  test_name = "eval(*(lookup &k1 m2_3)) == v2";
  MAP_LOOKUP_AND_LD(map2, addr_k1, v2, test_name)
  // check lookup m2[k2]
  test_name = "eval(*(lookup &k2 m2_3)) == v1";
  MAP_LOOKUP_AND_LD(map2, addr_k2, v1, test_name)

  // test the return value of delete
  cout << "  2 test return value of delete" << endl;
  m.clear();
  sv.clear();
  sv.mem_var.init_addrs_map_v_next(m_layout);
  f = string_to_expr("true");
  compute_st8(k1, addr_k1, 0);
  compute_st8(v1, addr_v1, 0);
  predicate_st8(v8(k1), v(addr_k1), v(0), sv.mem_var); // *addr_k1 = k1 (addr_k1 in the stack)
  predicate_st8(v8(v1), v(addr_v1), v(0), sv.mem_var); // *addr_v1 = v1 (addr_v1 in the stack)
  z3::expr del_ret_expr = string_to_expr("true");
  uint64_t del_ret;
#define MAP_DELETE_RET(map, addr_k) \
  del_ret_expr = new_out(); \
  del_ret = compute_map_delete_helper(map, addr_k, m); \
  f = f && predicate_map_delete_helper(v(map), v(addr_k), del_ret_expr, sv, m_layout);

  MAP_DELETE_RET(map1, addr_k1)  // del m1[k1]
  z3::expr f_out = eval_output(f, del_ret_expr);
  bool res = (! is_valid(f_out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR)) &&
             (! is_valid(f_out == MAP_DEL_RET_IF_KEY_EXIST_EXPR)) &&
             (is_valid((f_out == MAP_DEL_RET_IF_KEY_EXIST_EXPR) || (f_out == MAP_DEL_RET_IF_KEY_INEXIST_EXPR)));
  print_test_res(is_valid(f_expected), "predicate: eval_ret(delete &k m) == MAP_DEL_RET_IF_KEY_INEXIST "\
                 "or MAP_DEL_RET_IF_KEY_INEXIST");
  print_test_res(del_ret == MAP_DEL_RET_IF_KEY_INEXIST,
                 "compute: eval_ret(delete &k m) == MAP_DEL_RET_IF_KEY_INEXIST");

  MAP_UPDATE(map1, addr_k1, addr_v1) // m1[k1] = v1
  MAP_DELETE_RET(map1, addr_k1)
  f_expected = (eval_output(f, del_ret_expr) == v(del_ret));
  print_test_res(is_valid(f_expected) && (del_ret == MAP_DEL_RET_IF_KEY_EXIST),
                 "eval_ret(delete &k (update &k &v (delete &k m))) "\
                 "== MAP_DEL_RET_IF_KEY_EXIST");

  MAP_DELETE_RET(map1, addr_k1)
  f_expected = (eval_output(f, del_ret_expr) == v(del_ret));
  print_test_res(is_valid(f_expected) && (del_ret == MAP_DEL_RET_IF_KEY_INEXIST),
                 "eval_ret(delete &k (delete &k (update &k &v (delete &k m)))) "\
                 "== MAP_DEL_RET_IF_KEY_INEXIST");

#undef MAP_DELETE_RET
  cout << "  3 test size of k/v > 1 byte" << endl;
  m.clear();
  sv.clear();
  map1 = 0, map2 = 1;
  // set map2 key size: 16 bits, value size: 32 bits, max_entries: 512
  mem_t::set_map_attr(map2, map_attr(16, 32, 128));
  m_layout.set_map_attr(map2, map_attr(16, 32, 128));
  sv.mem_var.init_addrs_map_v_next(m_layout);
  k1 = 0x1, v1 = 0x11; // used by map1
  k2 = 0x1111, v2 = (int64_t)0xffffffff; //used by map2
  stack_s = (uint64_t)m.get_stack_start_addr();
  addr_k1 = stack_s, addr_v1 = stack_s + 1;
  addr_k2 = stack_s + 2, addr_v2 = stack_s + 4;
  f = string_to_expr("true");
  compute_st8(k1, addr_k1, 0);
  compute_st8(v1, addr_v1, 0);
  compute_st16(k2, addr_k2, 0);
  compute_st32(v2, addr_v2, 0);
  predicate_st8(v8(k1), v(addr_k1), v(0), sv.mem_var);
  predicate_st8(v8(v1), v(addr_v1), v(0), sv.mem_var);
  predicate_st16(v16(k2), v(addr_k2), v(0), sv.mem_var);
  predicate_st32(v32(v2), v(addr_v2), v(0), sv.mem_var);

#define MAP1_LOOKUP_AND_LD(v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  v_lookup_expr = new_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map1, addr_k1, m); \
  v_lookup = compute_ld8(addr_v_lookup, 0); \
  f = f && predicate_map_lookup_helper(v(map1), v(addr_k1), addr_v_lookup_expr, sv, m_layout); \
  f = f && predicate_ld8(addr_v_lookup_expr, v(0), sv.mem_var, v_lookup_expr, m_layout); \
  f_expected = (eval_output(f, v_lookup_expr) == v(v_lookup)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (v_lookup == v_expected), test_name);

#define MAP2_LOOKUP_AND_LD(v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  v_lookup_expr = new_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map2, addr_k2, m); \
  v_lookup = compute_ld32(addr_v_lookup, 0); \
  f = f && predicate_map_lookup_helper(v(map2), v(addr_k2), addr_v_lookup_expr, sv, m_layout); \
  f = f && predicate_ld32(addr_v_lookup_expr, v(0), sv.mem_var, v_lookup_expr, m_layout); \
  f_expected = (eval_output(f, v_lookup_expr) == v(v_lookup)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (v_lookup == v_expected), test_name);

#define MAP1_LOOKUP(addr_v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map1, addr_k1, m); \
  f = f && predicate_map_lookup_helper(v(map1), v(addr_k1), addr_v_lookup_expr, sv, m_layout); \
  f_expected = (eval_output(f, addr_v_lookup_expr) == v(addr_v_expected)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (addr_v_lookup == addr_v_expected), test_name);

#define MAP2_LOOKUP(addr_v_expected, test_name) \
  addr_v_lookup_expr = new_addr_v_lookup(); \
  addr_v_lookup = compute_map_lookup_helper(map2, addr_k2, m); \
  f = f && predicate_map_lookup_helper(v(map2), v(addr_k2), addr_v_lookup_expr, sv, m_layout); \
  f_expected = (eval_output(f, addr_v_lookup_expr) == v(addr_v_expected)); \
  print_test_res(is_valid(f_expected == string_to_expr("true")) && (addr_v_lookup == addr_v_expected), test_name);

  MAP_UPDATE(map1, addr_k1, addr_v1)
  MAP_UPDATE(map2, addr_k2, addr_v2)
  test_name = "eval(*(lookup &k1 (update &k1 &v1 m1))) == v1";
  MAP1_LOOKUP_AND_LD(v1, test_name)
  test_name = "eval(*(lookup &k2 (update &k2 &v2 m2))) == v2";
  MAP2_LOOKUP_AND_LD(v2, test_name)

  MAP_DELETE(map1, addr_k1)
  MAP_DELETE(map2, addr_k2)
  test_name = "eval(lookup &k1 (delete &k1 (update &k1 &v1 m1))) == NULL_ADDR_EXPR";
  MAP1_LOOKUP(NULL_ADDR, test_name)
  test_name = "eval(lookup &k2 (delete &k2 (update &k2 &v2 m2))) == NULL_ADDR_EXPR";
  MAP2_LOOKUP(NULL_ADDR, test_name)

  MAP_UPDATE(map1, addr_k1, addr_v1)
  MAP_UPDATE(map2, addr_k2, addr_v2)
  test_name = "eval(*(lookup &k1 (update &k1 &v1 (delete &k1 (update &k1 &v1 m1))))) == v1";
  MAP1_LOOKUP_AND_LD(v1, test_name)
  test_name = "eval(*(lookup &k2 (update &k2 &v2 (delete &k2 (update &k2 &v2 m2))))) == v2";
  MAP2_LOOKUP_AND_LD(v2, test_name)
#undef MAP1_LOOKUP_AND_LD
#undef MAP2_LOOKUP_AND_LD
#undef MAP1_LOOKUP
#undef MAP2_LOOKUP
}

#undef MAP_LOOKUP_AND_LD
#undef MAP_LOOKUP

void test10() {
  cout << "Test 10: Map equivalence check" << endl;
  unsigned int prog_id = 0, node_id = 0, num_regs = 11;
  smt_var sv1(prog_id, node_id, num_regs);
  prog_id = 1;
  smt_var sv2(prog_id, node_id, num_regs);
  smt_mem_layout m_layout;
  // set memory layout: stack | map
  z3::expr stack_s = v((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  z3::expr map1_s = stack_e + 1, map1_e = stack_e + 512;
  int map1 = 0;
  z3::expr addr_map1 = v(0);
  m_layout.set_stack_range(stack_s, stack_e);
  m_layout.add_map(map1_s, map1_e, map_attr(8, 8, 512));
  sv1.mem_var.init_addrs_map_v_next(m_layout);
  sv2.mem_var.init_addrs_map_v_next(m_layout);
  z3::expr k1 = to_expr("k1", 8), v1 = to_expr("v1", 8);
  z3::expr k2 = to_expr("k2", 8), v2 = to_expr("v2", 8);
  z3::expr addr_k1 = stack_s + 0, addr_v1 = stack_s + 1;
  z3::expr addr_k2 = stack_s + 2, addr_v2 = stack_s + 3;
  // test map without process, i.e., no elements in map tables
  cout << "1. case: both map WTs are empty" << endl;
  z3::expr f = string_to_expr("true");
  z3::expr f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  z3::expr f_equal = smt_one_map_eq_chk(map1, sv1, sv2, m_layout);
  print_test_res(is_valid(z3::implies(f && f_same_input, f_equal)), "m_p1_0 == m_p2_0 (maps without process)");
  cout << "2. case: keys can be found in both map WTs" << endl;
  // 1.1 update &k1 m_p1_0 == update &k1 m_p2_0
  predicate_st8(k1, addr_k1, v(0), sv1.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv1.mem_var);
  predicate_st8(k2, addr_k2, v(0), sv1.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv1.mem_var);
  predicate_st8(k1, addr_k1, v(0), sv2.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv2.mem_var);
  predicate_st8(k2, addr_k2, v(0), sv2.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv2.mem_var);
  f = (k1 != k2) && (v1 != v2);
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout);
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv2, m_layout);

#define MAP_EQ_CHK(addr_map, test_name, test_res) \
  f_same_input = smt_one_map_set_same_input(addr_map, sv1, sv2, m_layout); \
  f_equal = smt_one_map_eq_chk(addr_map, sv1, sv2, m_layout); \
  print_test_res(is_valid(z3::implies(f && f_same_input, f_equal)) == test_res, test_name);

#define MAP_EQ_CHK_WITH_PC(addr_map, test_name, test_res) \
  f_same_input = smt_one_map_set_same_input(addr_map, sv1, sv2, m_layout); \
  f_equal = smt_one_map_eq_chk(addr_map, sv1, sv2, m_layout); \
  print_test_res(is_valid(z3::implies(f && f_same_input && f_path_cond, f_equal)) == test_res, test_name);

  string test_name = "m_p1_1 == m_p2_1, m_p1_1 = update &k1 &v1 m_p1, m_p2_1 = update &k1 &v1 m_p2";
  MAP_EQ_CHK(map1, test_name, true)

  // test latest write
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v2, new_out(), sv2, m_layout);
  test_name = "m_p1_2 != m_p2_1, m_p1_2 = update &k1 &v2 m_p1_1";
  MAP_EQ_CHK(map1, test_name, false)

  // test latest write
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv2, m_layout);
  test_name = "m_p1_3 == m_p2_1, m_p1_3 = update &k1 &v1 m_p1_2";
  MAP_EQ_CHK(map1, test_name, true)
  // test delete
  f = f && predicate_map_delete_helper(addr_map1, addr_k1, new_out(), sv2, m_layout);
  test_name = "m_p1_4 != m_p2_1, m_p1_4 = delete &k1 m_p1_3";
  MAP_EQ_CHK(map1, test_name, false)

  f = f && predicate_map_delete_helper(addr_map1, addr_k1, new_out(), sv1, m_layout);
  test_name = "m_p1_4 == m_p2_2, m_p2_2 = delete &k1 m_p2_1";
  MAP_EQ_CHK(map1, test_name, true)

  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v2, new_out(), sv1, m_layout);
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v2, new_out(), sv2, m_layout);
  test_name = "m_p1_5 == m_p2_3, m_p1_5 = update &k1 &v2 m_p1_4, m_p2_3 = update &k1 &v2 m_p2_2";
  MAP_EQ_CHK(map1, test_name, true)

  cout << "3. case: there is(are) key(s) that can only be found in one map WT" << endl;
  sv1.clear(); sv2.clear();
  sv1.mem_var.init_addrs_map_v_next(m_layout);
  sv2.mem_var.init_addrs_map_v_next(m_layout);
  predicate_st8(k1, addr_k1, v(0), sv1.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv1.mem_var);
  predicate_st8(k2, addr_k2, v(0), sv1.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv1.mem_var);
  predicate_st8(k1, addr_k1, v(0), sv2.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv2.mem_var);
  predicate_st8(k2, addr_k2, v(0), sv2.mem_var);
  predicate_st8(v2, addr_v2, v(0), sv2.mem_var);
  f = (k1 != k2) && (v1 != v2);
  // store v_lookup_k1_p1 (= m_p1[k1]) in the stack (addr: stack_addr_v_lookup_p1)
  z3::expr addr_v_lookup_p1 = new_addr_v_lookup();
  z3::expr v_lookup_p1 = new_v_lookup();
  f = f && predicate_map_lookup_helper(addr_map1, addr_k1, addr_v_lookup_p1, sv1, m_layout);
  f = f && predicate_ld8(addr_v_lookup_p1, v(0), sv1.mem_var, v_lookup_p1, m_layout);
  z3::expr stack_addr_v_lookup_p1 = stack_s + 4;
  predicate_st8(v_lookup_p1, stack_addr_v_lookup_p1, v(0), sv1.mem_var);
  // store v_lookup_k1_p2 (= m_p2[k1]) in the stack (addr: stack_addr_v_lookup_p2)
  z3::expr addr_v_lookup_p2 = new_addr_v_lookup();
  z3::expr v_lookup_p2 = new_v_lookup();
  f = f && predicate_map_lookup_helper(addr_map1, addr_k1, addr_v_lookup_p2, sv2, m_layout);
  f = f && predicate_ld8(addr_v_lookup_p2, v(0), sv2.mem_var, v_lookup_p2, m_layout);
  z3::expr stack_addr_v_lookup_p2 = stack_s + 5;
  predicate_st8(v_lookup_p2, stack_addr_v_lookup_p2, v(0), sv2.mem_var);

  test_name = "m_p1_0 == m_p2_0";
  MAP_EQ_CHK(map1, test_name, true)

  f = f && predicate_map_update_helper(addr_map1, addr_k2, addr_v2, new_out(), sv1, m_layout); // m_p1[k2] = v2
  f = f && predicate_map_update_helper(addr_map1, addr_k2, addr_v2, new_out(), sv2, m_layout); // m_p2[k2] = v2
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout); // m_p1[k1] = v1
  test_name = "m_p1_1 != m_p2_1, m_p1_1 = update &k1 &v1 (update &k2 &v2 m_p1_0), m_p2_1 = update &k2 &v2 m_p2_0";
  MAP_EQ_CHK(map1, test_name, false)

  f = f && predicate_map_delete_helper(addr_map1, addr_k1, new_out(), sv1, m_layout); // del m_p1[k1]
  test_name = "m_p1_2 != m_p2_1, m_p1_2 = delete &k1 m_p1_1";
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_one_map_eq_chk(map1, sv1, sv2, m_layout);
  print_test_res(!is_valid(z3::implies(f && f_same_input, f_equal)), test_name);
  test_name = "m_p1_2 != m_p2_1, m_p1_2 = delete &k1 m_p1_1, if k1 not in the input map";
  z3::expr f_path_cond = (addr_v_lookup_p1 != NULL_ADDR_EXPR);
  print_test_res(!is_valid(z3::implies(f && f_same_input && f_path_cond, f_equal)), test_name);

  f = f && predicate_map_update_helper(addr_map1, addr_k1, stack_addr_v_lookup_p1, new_out(), sv1, m_layout);
  test_name = "m_p1_3 != m_p2_1, m_p1_3 = update &k1 &v_lookup_k1_p1 m_p1_2";
  MAP_EQ_CHK(map1, test_name, false)
  test_name = "m_p1_3 == m_p2_1, if k1 in m_p1_0";
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_one_map_eq_chk(map1, sv1, sv2, m_layout);
  f_path_cond = (addr_v_lookup_p1 != NULL_ADDR_EXPR);
  print_test_res(is_valid(z3::implies(f && f_same_input && f_path_cond, f_equal)), test_name);

  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout);
  test_name = "m_p1_4 != m_p2_1, m_p1_4 = update &k1 &v1 m_p1_3";
  MAP_EQ_CHK(map1, test_name, false)
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);

  // test input equivalence constrain
  cout << "4. test input equivalence constrain" << endl;
  f = f && predicate_map_update_helper(addr_map1, addr_k1, stack_addr_v_lookup_p1, new_out(), sv1, m_layout);
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_one_map_eq_chk(map1, sv1, sv2, m_layout);
  f = f && predicate_map_update_helper(addr_map1, addr_k1, stack_addr_v_lookup_p2, new_out(), sv2, m_layout);
  test_name = "m_p1_5 != m_p2_2, m_p1_5 = update &k1 &v_lookup_k1_p1 m_p1_4, "\
              "m_p2_2 = update &k1 &v_lookup_k1_p2 m_p2_1";
  print_test_res(!is_valid(z3::implies(f && f_same_input, f_equal)), test_name);
  test_name = "m_p1_5 == m_p2_2, if k1 in the input map";
  f_path_cond = (addr_v_lookup_p1 != NULL_ADDR_EXPR);
  print_test_res(is_valid(z3::implies(f && f_same_input && f_path_cond, f_equal)), test_name);

  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout);
  test_name = "m_p1_5 != m_p2_3, m_p2_3 = update &k1 &v1 m_p2_2 if k1 in the input map";
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_one_map_eq_chk(map1, sv1, sv2, m_layout);
  f_path_cond = (addr_v_lookup_p2 != NULL_ADDR_EXPR);
  print_test_res(!is_valid(z3::implies(f && f_same_input && f_path_cond, f_equal)), test_name);

  z3::expr addr_v_lookup = new_addr_v_lookup();
  z3::expr v_lookup = new_v_lookup();
  f = f && predicate_map_lookup_helper(addr_map1, addr_k1, addr_v_lookup, sv1, m_layout);
  f = f && predicate_ld8(addr_v_lookup, v(0), sv1.mem_var, v_lookup, m_layout);
  addr_v_lookup = new_addr_v_lookup();
  f = f && predicate_map_lookup_helper(addr_map1, addr_k2, addr_v_lookup, sv2, m_layout);
  test_name = "lookup does not affect map equivalence check";
  MAP_EQ_CHK(map1, test_name, true)

  cout << "5. test k/v size > 1 byte" << endl;
  sv1.clear(); sv2.clear();
  m_layout.set_map_attr(map1, map_attr(32, 16));
  sv1.mem_var.init_addrs_map_v_next(m_layout);
  sv2.mem_var.init_addrs_map_v_next(m_layout);
  k1 = to_expr("k1", 32), v1 = to_expr("v1", 16);
  addr_k1 = stack_s + 0, addr_v1 = stack_s + 4;
  predicate_st32(k1, addr_k1, v(0), sv1.mem_var);
  predicate_st16(v1, addr_v1, v(0), sv1.mem_var);
  predicate_st32(k1, addr_k1, v(0), sv2.mem_var);
  predicate_st16(v1, addr_v1, v(0), sv2.mem_var);
  addr_v_lookup_p1 = new_addr_v_lookup();
  addr_v_lookup_p2 = new_addr_v_lookup();
  v_lookup_p1 = new_v_lookup();
  v_lookup_p2 = new_v_lookup();
  f = predicate_map_lookup_helper(addr_map1, addr_k1, addr_v_lookup_p1, sv1, m_layout);
  f = f && predicate_ld16(addr_v_lookup_p1, v(0), sv1.mem_var, v_lookup_p1, m_layout);
  f = f && predicate_map_lookup_helper(addr_map1, addr_k1, addr_v_lookup_p2, sv2, m_layout);
  f = f && predicate_ld16(addr_v_lookup_p2, v(0), sv2.mem_var, v_lookup_p2, m_layout);
  stack_addr_v_lookup_p1 = stack_s + 6;
  stack_addr_v_lookup_p2 = stack_s + 8;
  predicate_st16(v_lookup_p1, stack_addr_v_lookup_p1, v(0), sv1.mem_var);
  predicate_st16(v_lookup_p2, stack_addr_v_lookup_p2, v(0), sv2.mem_var);
  test_name = "m_p1_1 != m_p2_0, m_p1_1 = update &k1 &v1 m_p1_0";
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout);
  MAP_EQ_CHK(map1, test_name, false)

  f = f && predicate_map_delete_helper(addr_map1, addr_k1, new_out(), sv1, m_layout);
  test_name = "m_p1_2 != m_p2_0, m_p1_2 = delete &k1 m_p1_1";
  MAP_EQ_CHK(map1, test_name, false)
  f_path_cond = (addr_v_lookup_p1 == NULL_ADDR_EXPR);
  test_name = "m_p1_2 == m_p2_0, if k1 not in the input map";
  MAP_EQ_CHK_WITH_PC(map1, test_name, true)

  f = f && predicate_map_delete_helper(addr_map1, addr_k1, new_out(), sv2, m_layout);
  test_name = "m_p1_2 == m_p2_1, m_p2_1 = delete &k1 m_p2_0";
  MAP_EQ_CHK(map1, test_name, true)

  f = f && predicate_map_update_helper(addr_map1, addr_k1, stack_addr_v_lookup_p1, new_out(), sv1, m_layout);
  f_path_cond = (addr_v_lookup_p1 != NULL_ADDR_EXPR);
  test_name = "m_p1_3 != m_p2_1, m_p1_3 = update &k1 &v_lookup_k1_p1 m_p1_2, if k1 in the input map";
  MAP_EQ_CHK_WITH_PC(map1, test_name, false)
  f = f && predicate_map_update_helper(addr_map1, addr_k1, stack_addr_v_lookup_p2, new_out(), sv2, m_layout);
  f_path_cond = (addr_v_lookup_p1 != NULL_ADDR_EXPR);
  test_name = "m_p1_3 == m_p2_2, m_p2_2 = update &k1 &v_lookup_k1_p2 m_p2_1, if k1 in the input map";
  MAP_EQ_CHK_WITH_PC(map1, test_name, true)

  cout << "6. test mutiple maps" << endl;
  sv1.clear(); sv2.clear();
  z3::expr map2_s = map1_e + 1, map2_e = map1_e + 512;
  int map2 = 1;
  z3::expr addr_map2 = v(1);
  m_layout.set_map_attr(map1, map_attr(8, 8));
  m_layout.add_map(map2_s, map2_e);
  sv1.mem_var.init_addrs_map_v_next(m_layout);
  sv2.mem_var.init_addrs_map_v_next(m_layout);
  k1 = to_expr("k1", 8), v1 = to_expr("v1", 8);
  addr_k1 = stack_s + 0, addr_v1 = stack_s + 1;
  predicate_st8(k1, addr_k1, v(0), sv1.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv1.mem_var);
  predicate_st8(k1, addr_k1, v(0), sv2.mem_var);
  predicate_st8(v1, addr_v1, v(0), sv2.mem_var);
  f = predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv1, m_layout);
  f = f && predicate_map_update_helper(addr_map1, addr_k1, addr_v1, new_out(), sv2, m_layout);
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_map_eq_chk(sv1, sv2, m_layout);
  print_test_res(is_valid(z3::implies(f && f_same_input, f_equal) == string_to_expr("true")), "1");
  f = f && predicate_map_update_helper(addr_map2, addr_k1, addr_v1, new_out(), sv1, m_layout);
  f = f && predicate_map_update_helper(addr_map2, addr_k1, addr_v1, new_out(), sv2, m_layout);
  f_same_input = smt_one_map_set_same_input(map1, sv1, sv2, m_layout);
  f_equal = smt_map_eq_chk(sv1, sv2, m_layout);
  print_test_res(is_valid(z3::implies(f && f_same_input, f_equal) == string_to_expr("true")), "2");
}

void test11() {
  cout << "Test 11: load n bytes from address" << endl;
  uint8_t a[2] = {0x1, 0xff};
  string s = ld_n_bytes_from_addr(a, 2);
  print_test_res(s == "ff01", "1");

  uint8_t a1[4] = {0x12, 0x34, 0x56, 0x08};
  s = ld_n_bytes_from_addr(a1, 4);
  print_test_res(s == "08563412", "2");
}

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
  test10();
  test11();

  return 0;
}
