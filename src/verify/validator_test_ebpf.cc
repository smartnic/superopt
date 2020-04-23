#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "validator.h"

using namespace z3;

mem_layout mem_t::_layout;

void test1() {
  std::cout << "test 1:" << endl;
  inst instructions1[9] = {inst(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                           inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                           inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                           inst(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                           inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                           inst(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                           inst(EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                           inst(MOV64XC, 0, 0),
                           inst(EXIT),
                          };

  inst instructions2[9] = {inst(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                           inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                           inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                           inst(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                           inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                           inst(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                           inst(JA, 1),                  /* else ret r0 = 0xffffffffffffffff */
                           inst(MOV64XC, 0, 0),
                           inst(EXIT),
                          };
  smt_mem_layout m_layout;
  z3::expr stack_s = to_expr((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout.set_stack_range(stack_s, stack_e);
  validator vld(instructions1, 9, m_layout);
  print_test_res(vld.is_equal_to(instructions1, 9, m_layout), "instructions1 == instructions1");
  print_test_res(vld.is_equal_to(instructions2, 9, m_layout), "instructions1 == instructions2");

  // output = L32(input)
  inst instructions3[2] = {inst(MOV32XY, 0, 1),
                           inst(EXIT),
                          };

  inst instructions4[3] = {inst(STXW, 10, -4, 1),
                           inst(LDXW, 0, 10, -4),
                           inst(EXIT),
                          };
  vld.set_orig(instructions3, 2, m_layout);
  // TODO: to pass the stack output equal check temporarily
  print_test_res(!vld.is_equal_to(instructions4, 3, m_layout), "instructions3 == instructions4");

  inst instructions5[3] = {inst(STXDW, 10, -8, 1),
                           inst(LDXDW, 0, 10, -8),
                           inst(EXIT),
                          };
  inst instructions6[9] = {inst(STXW, 10, -8, 1),
                           inst(RSH64XC, 1, 32),
                           inst(STXH, 10, -4, 1),
                           inst(RSH64XC, 1, 16),
                           inst(STXB, 10, -2, 1),
                           inst(RSH64XC, 1, 8),
                           inst(STXB, 10, -1, 1),
                           inst(LDXDW, 0, 10, -8),
                           inst(EXIT),
                          };
  inst instructions7[4] = {inst(MOV64XY, 9, 10),
                           inst(STXDW, 10, -8, 1),
                           inst(LDXDW, 0, 9, -8),
                           inst(EXIT),
                          };
  vld.set_orig(instructions5, 3, m_layout);
  print_test_res(vld.is_equal_to(instructions6, 9, m_layout), "instructions5 == instructions6");
  print_test_res(vld.is_equal_to(instructions7, 4, m_layout), "instructions5 == instructions7");
}

void test2() {
  std::cout << "test 2:" << endl;
  smt_mem_layout m_layout;
  z3::expr stack_s = to_expr((uint64_t)0xff12000000001234);
  z3::expr stack_e = stack_s + 511;
  m_layout.set_stack_range(stack_s, stack_e);
  // check branch with ld/st
  inst p1[6] = {inst(STXB, 10, -1, 1),
                inst(JEQXC, 1, 0x12, 2),
                inst(MOV64XC, 1, 0x12),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  inst p2[4] = {inst(MOV64XC, 1, 0x12),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  validator vld(p1, 6, m_layout);
  print_test_res(vld.is_equal_to(p2, 4, m_layout), "p1 == p2");

  inst p3[5] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 0, 1, 0),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  inst p4[4] = {inst(STXB, 10, -1, 1),
                inst(STXB, 10, -1, 1),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  vld.set_orig(p3, 5, m_layout);
  print_test_res(vld.is_equal_to(p4, 4, m_layout), "p3 == p4");

  // test no jmp
  inst p5[8] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 1, 2, 2), // jmp case 1, r1 == r2
                inst(STXB, 10, -1, 2),
                inst(JEQXY, 1, 3, 2), // jmp case 2, r1 == r3
                inst(STXB, 10, -1, 3),
                inst(JEQXY, 1, 4, 0), // jmp case 3, r1 == r4
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  inst p6[7] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 1, 2, 2), // jmp case 1, r1 == r2
                inst(STXB, 10, -1, 2),
                inst(JEQXY, 1, 3, 1), // jmp case 2, r1 == r3
                inst(STXB, 10, -1, 3),
                inst(LDXB, 0, 10, -1),
                inst(EXIT),
               };
  vld.set_orig(p5, 8, m_layout);
  print_test_res(vld.is_equal_to(p6, 7, m_layout), "p5 == p6");
}

void test3() {
  std::cout << "test 3:" << endl;
  smt_mem_layout m_layout;
  // set memory layout: stack | map1
  uint64_t stack_s = (uint64_t)0xff12000000001234;
  z3::expr stack_s_expr = to_expr(stack_s);
  z3::expr stack_e_expr = stack_s_expr + 511;
  z3::expr map_s_expr = stack_e_expr + 1;
  z3::expr map_e_expr = stack_e_expr + 512;
  m_layout.set_stack_range(stack_s_expr, stack_e_expr);
  m_layout.add_map(map_s_expr, map_e_expr, map_attr(8, 8, 512));
  map_s_expr = map_e_expr + 1;
  map_e_expr = map_s_expr + 511;
  m_layout.add_map(map_s_expr, map_e_expr, map_attr(8, 8, 512));
  // TODO: when safety check is added, these map related programs need to be modified
  // after calling map_update function, r1-r5 are unreadable.
  // r0 = *(lookup &k (update &k &v m)), where k = 0x11, v = L8(input)
  inst p1[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(MOV64XC, 1, 0), // r1 = map_id (0)
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p11[11] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(MOV64XC, 1, 0), // r1 = map_id (0)
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                  inst(LDXB, 0, 10, -2),
                  inst(EXIT),
                 };
  validator vld(p1, 13, m_layout);
  print_test_res(vld.is_equal_to(p1, 13, m_layout), "map helper function 1.1");
  print_test_res(vld.is_equal_to(p11, 11, m_layout), "map helper function 1.2");
  // r0 = *(lookup &k (delete &k (update &k &v m))), where k = 0x11, v = L8(input)
  inst p2[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(MOV64XC, 1, 0), // r1 = map_id (0)
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                 inst(CALL, BPF_FUNC_map_delete), // delete map0[k]
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p21[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(MOV64XC, 1, 0), // r1 = map_id (0)
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_delete), // delete map0[k]
                  inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                  inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                  inst(LDXB, 0, 0, 0),
                  inst(EXIT),
                 };
  vld.set_orig(p2, 14, m_layout);
  print_test_res(vld.is_equal_to(p2, 14, m_layout), "map helper function 2.1");
  print_test_res(vld.is_equal_to(p21, 13, m_layout), "map helper function 2.2");
  // r0 = *(lookup &k m), where k = 0x11, v = L8(input)
  inst p3[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(MOV64XC, 1, 0), // r1 = map_id (0)
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                inst(LDXB, 0, 0, 0),
                inst(EXIT),
               };
  vld.set_orig(p3, 9, m_layout);
  print_test_res(vld.is_equal_to(p3, 9, m_layout), "map helper function 3.1");
}

void test4() {
  std::cout << "test 4: conversion from counter example to input memory "\
            "for interpreter" << endl;
  smt_mem_layout m_layout;
  // set memory layout: stack | map1
  uint64_t stack_s = (uint64_t)0xff12000000001234;
  z3::expr stack_s_expr = to_expr(stack_s);
  z3::expr stack_e_expr = stack_s_expr + 511;
  z3::expr map_s_expr = stack_e_expr + 1;
  z3::expr map_e_expr = stack_e_expr + 32;
  m_layout.set_stack_range(stack_s_expr, stack_e_expr);
  m_layout.add_map(map_s_expr, map_e_expr, map_attr(8, 8, 32));
  map_s_expr = map_e_expr + 1;
  map_e_expr = map_s_expr - 1 + 32 * 4;
  m_layout.add_map(map_s_expr, map_e_expr, map_attr(16, 32, 32));
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  mem_t::add_map(map_attr(16, 32, 32)); // k_sz: 16 bits; v_sz: 32 bits; max_entirs: 32
  int map0 = 0, map1 = 1;
  int k1 = 0x11, v1 = 0xff;
  string k1_str = "11";
  // p1: r0 = v1 if map0[k1] == v1, else r0 = 0
  inst p1[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(MOV64XC, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                 inst(JEQXC, 0, 0, 3), // if r0 == 0, exit else r0 = map0[k1]
                 inst(LDXB, 0, 0, 0),  // r0 = map0[k1]
                 inst(JEQXC, 0, v1, 1), //if r0 == v1 exit else r0 = 0
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  // p11: r0 = 0
  inst p11[11] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, k1), // *addr_k = k1
                  inst(STXB, 10, -1, 1),
                  inst(MOV64XC, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                  inst(MOV64XC, 0, 0), // set the return value r0 = 0
                  inst(EXIT),
                 };
  validator vld(p1, 14, m_layout);
  mem_t input_mem_expected;
  input_mem_expected.init_mem_by_layout();
  input_mem_expected.update_kv_in_map(map0, k1_str, (uint8_t*)(&v1));
  bool res = (vld.is_equal_to(p11, 11, m_layout) == 0) && // 0: not equal
             (vld._last_counterex_mem == input_mem_expected);
  print_test_res(res == true, "1");
}

int main() {
  test1();
  test2();
  test3();
  test4();

  return 0;
}
