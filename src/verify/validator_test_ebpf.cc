#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "validator.h"

using namespace z3;

#define TEST_PGM_MAX_LEN 25

void eq_check(inst* p1, int len1, inst* p2, int len2, int expected, string test_name) {
  validator vld(p1, len1);
  vld._enable_prog_eq_cache = false;
  print_test_res(vld.is_equal_to(p1, len1, p2, len2) == expected, test_name);
  // smt_var::enable_addr_off = false;
  // print_test_res(vld.is_equal_to(p1, len1, p2, len2) == expected, test_name);
  // smt_var::enable_addr_off = true;
}

void test1() {
  std::cout << "test 1:" << endl;
  mem_t::_layout.clear();
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
  validator vld(instructions1, 9);
  eq_check(instructions1, 9, instructions1, 9, 1, "instructions1 == instructions1");
  eq_check(instructions1, 9, instructions2, 9, 1, "instructions1 == instructions2");

  // output = L32(input)
  inst instructions3[2] = {inst(MOV32XY, 0, 1),
                           inst(EXIT),
                          };

  inst instructions4[3] = {inst(STXW, 10, -4, 1),
                           inst(LDXW, 0, 10, -4),
                           inst(EXIT),
                          };
  eq_check(instructions3, 2, instructions4, 3, 1, "instructions3 == instructions4");

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
  eq_check(instructions5, 3, instructions6, 9, 1, "instructions5 == instructions6");
  eq_check(instructions5, 3, instructions7, 4, 1, "instructions5 == instructions7");

  // test xadd64
  // r0 = 0x100000002 + 0x300000004 = 0x400000006
  inst instructions8[10] = {inst(MOV64XC, 1, 0x1),
                            inst(LSH64XC, 1, 32),
                            inst(ADD64XC, 1, 0x2), // r1 = 0x100000002
                            inst(STXDW, 10, -8, 1),
                            inst(MOV64XC, 2, 0x3),
                            inst(LSH64XC, 2, 32),
                            inst(ADD64XC, 2, 0x4), // r2 = 0x300000004
                            inst(XADD64, 10, -8, 2),
                            inst(LDXDW, 0, 10, -8),
                            inst(EXIT),
                           };
  inst instructions9[10] = {inst(MOV64XC, 1, 0x1),
                            inst(LSH64XC, 1, 32),
                            inst(ADD64XC, 1, 0x2), // r1 = 0x100000002
                            inst(STXDW, 10, -8, 1),
                            inst(MOV64XC, 2, 0x3),
                            inst(XADD32, 10, -4, 2),
                            inst(MOV64XC, 2, 0x4),
                            inst(XADD32, 10, -8, 2),
                            inst(LDXDW, 0, 10, -8),
                            inst(EXIT),
                           };
  // test xadd64
  // r0 = 0x100000002 + 0x300000004 = 0x400000006
  inst instructions10[8] = {inst(MOV64XC, 0, 0x1),
                            inst(LSH64XC, 0, 32),
                            inst(ADD64XC, 0, 0x2), // r1 = 0x100000002
                            inst(MOV64XC, 1, 0x3),
                            inst(LSH64XC, 1, 32),
                            inst(ADD64XC, 1, 0x4), // r2 = 0x300000004
                            inst(ADD64XY, 0, 1),
                            inst(EXIT),
                           };
  eq_check(instructions8, 10, instructions8, 10, 1, "instructions8 == instructions8");
  eq_check(instructions8, 10, instructions9, 10, 1, "instructions8 == instructions9");
  eq_check(instructions8, 10, instructions10, 8, 1, "instructions8 == instructions10");

  // test or64xc, or64xy
  // r0 = (r1 | 0x110) | 0x011 = r1 | 0x111
  inst instructions11[4] = {inst(OR64XC, 1, 0x110),
                            inst(MOV64XC, 0, 0x011),
                            inst(OR64XY, 0, 1),
                            inst(EXIT),
                           };
  inst instructions12[3] = {inst(OR64XC, 1, 0x111),
                            inst(MOV64XY, 0, 1),
                            inst(EXIT),
                           };
  eq_check(instructions11, 4, instructions11, 4, 1, "instructions11 == instructions11");
  eq_check(instructions11, 4, instructions12, 3, 1, "instructions11 == instructions12");

  // test and64xc, and64xy
  // r0 = (r1 & 0x011) & 0x110 = r1 & 0x10
  inst instructions13[4] = {inst(AND64XC, 1, 0x011),
                            inst(MOV64XC, 0, 0x110),
                            inst(AND64XY, 0, 1),
                            inst(EXIT),
                           };
  inst instructions14[3] = {inst(AND64XC, 1, 0x010),
                            inst(MOV64XY, 0, 1),
                            inst(EXIT),
                           };
  eq_check(instructions13, 4, instructions13, 4, 1, "instructions13 == instructions13");
  eq_check(instructions13, 4, instructions14, 3, 1, "instructions13 == instructions14");

  // test initial value of stack, stack has no initial value;
  inst instructions15[2] = {inst(LDXB, 0, 10, -1),
                            inst(EXIT),
                           };
  inst instructions16[3] = {inst(STB, 10, -1, 1),
                            inst(LDXB, 0, 10, -1),
                            inst(EXIT),
                           };
  eq_check(instructions15, 2, instructions15, 2, 0, "instructions15 != instructions15"); // prog_eq_cache is disabled
  eq_check(instructions15, 2, instructions16, 3, 0, "instructions15 != instructions16");
}

void test2() {
  std::cout << "test 2:" << endl;
  mem_t::_layout.clear();
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
  eq_check(p1, 6, p2, 4, 1, "p1 == p2");

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
  eq_check(p3, 5, p4, 4, 1, "p3 == p4");

  // test no jmp
  inst p5[9] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 1, 2, 2), // jmp case 1, r1 == r2
                inst(STXB, 10, -1, 2),
                inst(JEQXY, 1, 3, 2), // jmp case 2, r1 == r3
                inst(STXB, 10, -1, 3),
                inst(JEQXY, 1, 4, 0), // jmp case 3, r1 == r4
                inst(LDXB, 0, 10, -1),
                inst(MOV64XC, 0, 0),
                inst(EXIT),
               };
  inst p6[8] = {inst(STXB, 10, -1, 1),
                inst(JEQXY, 1, 2, 2), // jmp case 1, r1 == r2
                inst(STXB, 10, -1, 2),
                inst(JEQXY, 1, 3, 1), // jmp case 2, r1 == r3
                inst(STXB, 10, -1, 3),
                inst(LDXB, 0, 10, -1),
                inst(MOV64XC, 0, 0),
                inst(EXIT),
               };
  eq_check(p5, 9, p6, 8, 1, "p5 == p6");

  // test concrete execution of multiple memory tables
  // different addrs from different path conditions
  inst p7[8] = {inst(STXB, 10, -1, 1),
                inst(STXB, 10, -2, 2),
                inst(MOV64XY, 3, 10),
                inst(ADD64XC, 3, -1),
                inst(JGTXY, 1, 2, 1), // if r1 > r2, jmp 1
                inst(ADD64XC, 3, -1),
                inst(LDXB, 0, 3, 0),
                inst(EXIT),
               };
  eq_check(p7, 8, p7, 8, 1, "p7 == p7");
}

void test3() {
  std::cout << "test 3:" << endl;
  // set memory layout: stack | map1 | map2
  mem_t::_layout.clear();
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  mem_t::add_map(map_attr(16, 32, 32));
  mem_t::add_map(map_attr(8, 8, 32));
  int map0 = 0, map1 = 1, map2 = 2;
  int k1 = 0x11, v1 = 0xff;
  string k1_str = "11";
  // TODO: when safety check is added, these map related programs need to be modified
  // after calling map_update function, r1-r5 are unreadable.
  // map0[k1] = L8(input), output = L8(input)
  inst p1[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = k1
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = map_id (0)
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update_elem), // map0[k] = v, i.e., map0[r1] = 0x11
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v = lookup k map0
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  // map0[k1] = L8(input), output = L8(input)
  inst p11[11] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(LDMAPID, 1, 0), // r1 = map_id (0)
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update_elem), // map0[k] = v, i.e., map0[r1] = 0x11
                  inst(LDXB, 0, 10, -2),
                  inst(EXIT),
                 };
  eq_check(p1, 13, p1, 13, 1, "map helper function 1.1");
  eq_check(p1, 13, p11, 11, 1, "map helper function 1.2");

  // r0 = *(lookup &k (delete &k (update &k &v m))), where k = 0x11, v = L8(input)
  inst p2[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = map_id (0)
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update_elem), // map0[k] = v, i.e., map0[r1] = 0x11
                 inst(CALL, BPF_FUNC_map_delete_elem), // delete map0[k]
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v = lookup k map0
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p21[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(LDMAPID, 1, 0), // r1 = map_id (0)
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_delete_elem), // delete map0[k]
                  inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v = lookup k map0
                  inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                  inst(LDXB, 0, 0, 0),
                  inst(EXIT),
                 };
  eq_check(p2, 14, p2, 14, 1, "map helper function 2.1");
  eq_check(p2, 14, p21, 13, 1, "map helper function 2.2");
  // r0 = *(lookup &k m), where k = 0x11, v = L8(input)
  inst p3[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, 0), // r1 = map_id (0)
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v = lookup k map0
                inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                inst(LDXB, 0, 0, 0),
                inst(EXIT),
               };
  eq_check(p3, 9, p3, 9, 1, "map helper function 3.1");

  inst p4[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = 0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update_elem),
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = map0[k1]
                 inst(LDXB, 0, 0, 0),  // r0 = map0[k1]
                 inst(EXIT),
                };
  inst p41[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = map0[k1]
                 inst(LDXB, 0, 0, 0),  // r0 = map0[k1]
                 inst(EXIT),
                };
  eq_check(p4, 13, p41, 9, 0, "map helper function 4.1");

  // upd &k1 &input (del &k1 m0), output = 0
  inst p5[12] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_delete_elem),
                 inst(CALL, BPF_FUNC_map_update_elem),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  // upd &k1 &input m0, output = 0
  inst p51[11] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                  inst(MOV64XC, 1, k1), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update_elem),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  eq_check(p5, 12, p5, 12, 1, "map helper function 5.1");
  eq_check(p5, 12, p51, 11, 1, "map helper function 5.2");

  // if k1 in map0, map0[k1]+=1, output=0
  inst p6[12] = {inst(MOV64XC, 1, k1),
                 inst(STXB, 10, -1, 1), // *(r10-1) = k1
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2 = (r10-1)
                 inst(ADD64XC, 2, -1),
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = &v = lookup k1 map0
                 inst(JEQXC, 0, 0, 4), // if r0 == 0, exit
                 inst(LDXB, 1, 0, 0), // r1 = v
                 inst(ADD64XC, 1, 1), // r1 += 1
                 inst(STXB, 0, 0, 1), // *r0 = r1
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  // r0 = *(lookup &k1 m0)
  inst p61[16] = {inst(MOV64XC, 1, k1),
                  inst(STXB, 10, -1, 1), // *(r10-1) = k1
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2 = (r10-1)
                  inst(ADD64XC, 2, -1),
                  inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = &v = lookup k1 map0
                  inst(JEQXC, 0, 0, 8), // if r0 == 0, exit
                  inst(LDXB, 1, 0, 0), // r1 = v
                  inst(ADD64XC, 1, 1), // r1 += 1
                  inst(STXB, 10, -2, 1), // *(r10-2) = r1
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 3, 10), // r3 = r10-2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update_elem),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  eq_check(p6, 12, p6, 12, 1, "map helper function 6.1");
  eq_check(p6, 12, p61, 16, 1, "map helper function 6.2");
  eq_check(p61, 16, p61, 16, 1, "map helper function 6.3");
  eq_check(p61, 16, p6, 12, 1, "map helper function 6.4");

  inst p7[21] = {inst(MOV64XC, 0, 0),
                 inst(MOV64XC, 9, 0),
                 inst(STB, 10, -1, 1), // *(r10-1) = 1
                 inst(STB, 10, -2, 2), // *(r10-2) = 2
                 inst(MOV64XY, 2, 10), // r2 = r10-1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3 = r10-1
                 inst(ADD64XC, 3, -1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(CALL, BPF_FUNC_map_update_elem), // map0[1] = 1
                 inst(MOV64XY, 3, 10), // r3 = r10-2
                 inst(ADD64XC, 3, -2),
                 inst(LDMAPID, 1, map2), // r1 = map2
                 inst(CALL, BPF_FUNC_map_update_elem), // map2[1] = 2
                 inst(LDMAPID, 1, map0), // r1 = map0, // 13
                 inst(JGTXY, 10, 9, 1),
                 inst(LDMAPID, 1, map2), // r1 = map2
                 inst(CALL, BPF_FUNC_map_lookup_elem),
                 inst(JEQXC, 0, 0, 1),
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p71[18] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XC, 9, 0),
                  inst(STB, 10, -1, 1), // *(r10-1) = 1
                  inst(STB, 10, -2, 2), // *(r10-2) = 2
                  inst(MOV64XY, 2, 10), // r2 = r10-1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3 = r10-1
                  inst(ADD64XC, 3, -1),
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(CALL, BPF_FUNC_map_update_elem), // map0[1] = 1
                  inst(MOV64XY, 3, 10), // r3 = r10-2
                  inst(ADD64XC, 3, -2),
                  inst(LDMAPID, 1, map2), // r1 = map2
                  inst(CALL, BPF_FUNC_map_update_elem), // map2[1] = 2
                  inst(MOV64XC, 0, 1),
                  inst(JGTXY, 10, 9, 1),
                  inst(MOV64XC, 0, 2),
                  inst(EXIT),
                 };
  eq_check(p7, 21, p7, 21, 1, "map helper function 7.1");
  eq_check(p7, 21, p71, 18, 1, "map helper function 7.2");

  inst p8[16] = {inst(STB, 10, -2, 0),
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update_elem),
                 inst(JEQXC, 10, 0xfff, 2),
                 inst(STB, 10, -2, 1),
                 inst(CALL, BPF_FUNC_map_update_elem),
                 inst(CALL, BPF_FUNC_map_lookup_elem),
                 inst(JEQXC, 0, 0, 1),
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p81[16] = {inst(STB, 10, -2, 1),
                  inst(MOV64XC, 1, k1), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update_elem),
                  inst(JNEXC, 10, 0xfff, 2),
                  inst(STB, 10, -2, 0),
                  inst(CALL, BPF_FUNC_map_update_elem),
                  inst(MOV64XC, 0, 0),
                  inst(JEQXC, 10, 0xfff, 1),
                  inst(MOV64XC, 0, 1),
                  inst(EXIT),
                 };
  eq_check(p8, 16, p81, 16, 1, "map helper function 8.1");

  // modify a part of map1[1]
  // mem_t::add_map(map_attr(16, 32, 32));  k_sz: 2 bytes, v_sz: 4 bytes
  inst p9[9] = {inst(STH, 10, -2, 1), // k = 1
                inst(MOV64XY, 2, 10),
                inst(ADD64XC, 2, -2),
                inst(LDMAPID, map1),
                inst(CALL, BPF_FUNC_map_lookup_elem),
                inst(JEQXC, 0, 0, 1),
                inst(STB, 0, 0, 1), // set map1[1][0] = 0, not modify map1[1][1...3]
                inst(MOV64XC, 0, 1), // set the return value as 1
                inst(EXIT),
               };
  eq_check(p9, 9, p9, 9, 1, "map helper function 9.1");
}

void chk_counterex_by_vld_to_interpreter(inst* p1, int len1, inst* p2, int len2,
    string test_name, int counterex_type = COUNTEREX_eq_check, bool interpret_check = true,
    bool is_win = false, int win_start = 0, int win_end = inst::max_prog_len) {
  prog_state ps;
  ps.init();
  validator vld;
  if (is_win) {
    vld._enable_prog_eq_cache = false;
    vld._is_win = true;
    smt_var::is_win = is_win;
    vld.set_orig(p1, len1, win_start, win_end);
  } else {
    vld.set_orig(p1, len1);
  }
  int ret_val = 0;
  if (counterex_type == COUNTEREX_eq_check) ret_val = 0; // 0: not equal
  else if (counterex_type == COUNTEREX_safety_check) ret_val = ILLEGAL_CEX;
  else cout << "ERROR: no counterex_type matches" << endl;
  bool res_expected = (vld.is_equal_to(p1, len1, p2, len2) == ret_val);
  inout_t input, output0, output1;
  input.init();
  output0.init();
  output1.init();
  if (interpret_check) {
    interpret(output0, p1, len1, ps, input);
    interpret(output1, p2, len2, ps, input);
    res_expected = res_expected && (output0 == output1);
    output0.clear();
    output1.clear();
  }
  interpret(output0, p1, len1, ps, vld._last_counterex.input);
  if (counterex_type == COUNTEREX_eq_check) {
    interpret(output1, p2, len2, ps, vld._last_counterex.input);
    res_expected = res_expected && (!(output0 == output1));
    res_expected = res_expected && (output0 == vld._last_counterex.output);
  } else if (counterex_type == COUNTEREX_safety_check) {
    bool captured = false;
    try {
      interpret(output1, p2, len2, ps, vld._last_counterex.input);
    } catch (const string err_msg) {
      // unsafe synthesis is supposed to be captured by the counterex input
      captured = true;
    }
    res_expected = res_expected && captured;
  }

  print_test_res(res_expected, test_name);
}

void test4() {
  std::cout << "test 4: conversion from counter example to input memory "\
            "for interpreter" << endl;
  std::cout << "1. test map" << std::endl;
  // set memory layout: stack | map1 | map2
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  mem_t::add_map(map_attr(16, 32, 32)); // k_sz: 16 bits; v_sz: 32 bits; max_entirs: 32
  mem_t::set_pkt_sz(128);
  int map0 = 0, map1 = 1;
  int k1 = 0x11, v1 = 0xff;
  string k1_str = "11";
  // p1: r0 = v1 if map0[k1] == v1, else r0 = 0
  inst p1[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
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
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
                  inst(MOV64XC, 0, 0), // set the return value r0 = 0
                  inst(EXIT),
                 };
  validator vld(p1, 14);
  inout_t input_expected;
  input_expected.init();
  // check counter example is generated and the value of input memory
  bool res_expected = (vld.is_equal_to(p1, 14, p11, 11) == 0);// 0: not equal
  // do not need to check the input register
  res_expected = res_expected && (vld._last_counterex.input.k_in_map(map0, k1_str));
  prog_state ps;
  ps.init();
  // check the outputs of p1 and p11 with/without input memory
  inout_t input, output0, output1;
  input.init();
  output0.init();
  output1.init();
  interpret(output0, p1, 14, ps, input);
  interpret(output1, p11, 11, ps, input);
  res_expected = res_expected && (output0 == output1);
  interpret(output0, p1, 14, ps, vld._last_counterex.input);
  interpret(output1, p11, 11, ps, vld._last_counterex.input);
  res_expected = res_expected && (!(output0 == output1));
  res_expected = res_expected && (output0 == vld._last_counterex.output);
  print_test_res(res_expected, "1");

  // update k1 0 map0, output r0 = 0
  inst p12[14] = {inst(MOV64XC, 1, 0), // *addr_v = 0
                  inst(STXB, 10, -2, 1),
                  inst(MOV64XC, 1, k1), // *addr_k = 0x11
                  inst(STXB, 10, -1, 1),
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_lookup_elem),
                  inst(JEQXC, 0, 0, 1),
                  inst(CALL, BPF_FUNC_map_update_elem),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  inst p13[2] = {inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  vld.set_orig(p12, 14);
  // check counter example is generated and the value of input memory
  res_expected = (vld.is_equal_to(p12, 14, p13, 2) == 0); // 0: not equal
  res_expected = res_expected && (vld._last_counterex.input.k_in_map(map0, k1_str));
  print_test_res(res_expected, "1.1");

  // p2: r0 = map1[k1] if k1 in map1, else r0 = 0
  inst p2[9] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                inst(STXH, 10, -2, 1),
                inst(LDMAPID, 1, map1), // r1 = map1
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 2
                inst(ADD64XC, 2, -2),
                inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
                inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = map1[k1]
                inst(LDXB, 0, 0, 0),
                inst(EXIT),
               };
  // p21: r0 = 0
  inst p21[2] = {inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  // p22: r0 = map1[k1] if map1[k1] == v1, else r0 = 0
  inst p22[11] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                  inst(STXH, 10, -2, 1),
                  inst(LDMAPID, 1, map1), // r1 = map1
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 2
                  inst(ADD64XC, 2, -2),
                  inst(CALL, BPF_FUNC_map_lookup_elem), // r0 = addr_v1
                  inst(JEQXC, 0, 0, 3), // if r0 == 0, exit else r0 = map1[k1]
                  inst(LDXB, 0, 0, 0),
                  inst(JEQXC, 0, v1, 1), //if r0 == v1 exit else r0 = 0
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  chk_counterex_by_vld_to_interpreter(p2, 9, p21, 2, "2.1");
  chk_counterex_by_vld_to_interpreter(p2, 9, p22, 11, "2.2");

  // r0 = 0xfffffffe if k1 not in map0, else r0 = 0; del map0[k1]
  inst p3[7] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, map0), // r1 = map0
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_delete_elem),
                inst(EXIT),
               };
  // r0 = 0xfffffffe
  inst p31[2] = {inst(MOV32XC, 0, 0xfffffffe),
                 inst(EXIT),
                };
  chk_counterex_by_vld_to_interpreter(p3, 7, p31, 2, "3");

  // r0 = 1 if k1 in map0, else r0 = 0
  inst p4[9] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, map0), // r1 = map0
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup_elem),
                inst(JEQXC, 0, 0, 1),
                inst(MOV64XC, 0, 1),
                inst(EXIT),
               };
  // r0 = 0
  inst p41[2] = {inst(MOV32XC, 0, 0),
                 inst(EXIT),
                };
  chk_counterex_by_vld_to_interpreter(p4, 9, p41, 2, "4");

  std::cout << "2. test packet" << std::endl;
  // r0 = pkt[0]
  inst p5[3] = {inst(MOV64XY, 6, 1),
                inst(LDXB, 0, 6, 0),
                inst(EXIT),
               };
  // r0 = 0x11
  inst p51[6] = {inst(MOV64XY, 6, 1),
                 inst(LDXB, 0, 6, 0),     // r0 = pkt[0]
                 inst(JEQXC, 0, 0x11, 1), // if r0 == 0x11, r0 = 0xff
                 inst(EXIT),
                 inst(MOV64XC, 0, 0xff),
                 inst(EXIT),
                };
  chk_counterex_by_vld_to_interpreter(p5, 3, p51, 6, "1");

  // test skb
  std::cout << "3. test skb" << std::endl;
  // set memory layout: stack | map1 | map2
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_skb);
  mem_t::set_pkt_sz(96);
  mem_t::set_skb_max_sz(16);
  inst p2_1[2] = {inst(LDABSH, 0),
                  inst(EXIT),
                 };
  inst p2_2[2] = {inst(LDABSH, 1),
                  inst(EXIT),
                 };
  chk_counterex_by_vld_to_interpreter(p2_1, 2, p2_2, 2, "1");

  cout << "4. counter example from safety check" << endl;
  inst p4_1[5] = {inst(MOV64XC, 0, 0xff),
                  inst(JNEXC, 1, 0xff, 2),
                  inst(STXH, 10, -2, 0),
                  inst(LDXH, 0, 10, -2),
                  inst(EXIT),
                 };
  inst p4_2[5] = {inst(MOV64XC, 0, 0xff),
                  inst(JNEXC, 1, 0xff, 2),
                  inst(STXH, 10, -3, 0),
                  inst(LDXH, 0, 10, -3),
                  inst(EXIT),
                 };
  chk_counterex_by_vld_to_interpreter(p4_1, 5, p4_2, 5, "4.1", COUNTEREX_safety_check);
}

void test5() { // test pkt
  std::cout << "test 5: packet related program equivalence check" << endl;
  // set memory layout: stack | map1 | map2
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  unsigned int pkt_sz = 128;
  mem_t::set_pkt_sz(pkt_sz); // pkt size: 128 bytes
  // test pkt, r0 = pkt[0]
  inst p1[3] = {inst(MOV64XY, 6, 1),
                inst(LDXB, 0, 6, 0),
                inst(EXIT),
               };
  eq_check(p1, 3, p1, 3, 1, "1");

  // r0 = 0x11, pkt[0] = 0x11
  inst p2[4] = {inst(MOV64XY, 6, 1),
                inst(MOV64XC, 0, 0x11),
                inst(STXB, 6, 0, 0),
                inst(EXIT),
               };
  // r0 = 0x11, pkt[0] = 0x11, pkt[sz-1] = 0x11
  inst p3[5] = {inst(MOV64XY, 6, 1),
                inst(MOV64XC, 0, 0x11),
                inst(STXB, 6, 0, 0),
                inst(STXB, 6, pkt_sz - 1, 0),
                inst(EXIT),
               };
  // r0 = 0x11, pkt[0] = 0x11, pkt[sz-1] = pkt[sz-1]_input
  inst p4[6] = {inst(MOV64XY, 6, 1),
                inst(MOV64XC, 0, 0x11),
                inst(STXB, 6, 0, 0),
                inst(LDXB, 1, 6, pkt_sz - 1), // r1 = *(uint8*)pkt[sz-1]
                inst(STXB, 6, pkt_sz - 1, 1), // *(uint8*)pkt[sz-1] = r1
                inst(EXIT),
               };
  eq_check(p2, 4, p1, 3, 0, "2");
  eq_check(p2, 4, p2, 4, 1, "3");
  eq_check(p2, 4, p3, 5, 0, "4");
  eq_check(p2, 4, p4, 6, 1, "5");
  eq_check(p3, 5, p3, 5, 1, "6");
  eq_check(p4, 6, p4, 6, 1, "7");

  inst p5[11] = {inst(MOV64XC, 0, 0),
                 inst(STB, 10, -1, 1),
                 inst(MOV64XC, 5, 2),
                 inst(STXB, 1, 1, 5),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -1),
                 inst(JGTXY, 10, 0, 2),
                 inst(MOV64XY, 2, 1),
                 inst(ADD64XC, 2, 1),
                 inst(LDXB, 0, 2, 0),
                 inst(EXIT),
                };
  inst p6[7] = {inst(MOV64XC, 0, 0),
                inst(MOV64XC, 5, 2),
                inst(STXB, 1, 1, 5),
                inst(MOV64XC, 0, 1),
                inst(JGTXY, 10, 0, 1),
                inst(MOV64XC, 1, 2),
                inst(EXIT),
               };
  eq_check(p5, 11, p5, 11, 1, "8");
  eq_check(p5, 11, p6, 7, 1, "9");

  // test address track of addxy
  inst p7[5] = {inst(MOV64XC, 0, 0),
                inst(ADD64XC, 1, 1),
                inst(MOV64XC, 5, 0xff),
                inst(STXB, 1, 0, 5),
                inst(EXIT),
               };
  inst p8[4] = {inst(MOV64XC, 0, 0),
                inst(MOV64XC, 5, 0xff),
                inst(STXB, 1, 1, 5),
                inst(EXIT),
               };
  eq_check(p7, 5, p8, 4, 1, "9");
}

void test6() {
  std::cout << "test 6: test \"PGM_INPUT_pkt_ptrs\" program input type" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(16);
  inst p1[5] = {inst(MOV64XC, 0, 0),
                inst(LDXW, 2, 1, 4),
                inst(LDXW, 1, 1, 0),
                inst(STB, 1, 0, 1),
                inst(STB, 2, 0, 2),
               };
  inst p11[9] = {inst(MOV64XC, 0, 0),
                 inst(LDXW, 2, 1, 4),
                 inst(LDXW, 1, 1, 0),
                 inst(STB, 1, 0, 1),
                 inst(MOV64XY, 3, 2),
                 inst(ADD64XC, 3, 0),
                 inst(MOV64XC, 4, 0),
                 inst(ADD64XY, 3, 4),
                 inst(STB, 3, 0, 2),
                };
  inst p12[3] = {inst(MOV64XC, 0, 0),
                 inst(LDXW, 1, 1, 0),
                 inst(STB, 1, 0, 1),
                };
  validator vld(p1, 5);
  print_test_res(vld.is_equal_to(p1, 5, p1, 5) == 1, "1");
  print_test_res(vld.is_equal_to(p1, 5, p11, 9) == 1, "2");
  print_test_res(vld.is_equal_to(p1, 5, p12, 3) == 0, "3");

  inst p2[11] = {inst(LDXW, 2, 1, 4), // r2 = pkt_end_addr
                 inst(LDXW, 1, 1, 0), // r1 = pkt_start_addr
                 inst(MOV64XY, 3, 1),
                 inst(ADD64XC, 3, 1),  // r3 = pkt_start_addr + 1
                 inst(JGTXY, 2, 3, 2),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                 inst(LDXB, 3, 1, 0), // r3 = pkt[0]
                 inst(LDXB, 4, 2, 0), // r4 = pkt[1]
                 inst(ADD64XY, 3, 4),
                 inst(MOV64XY, 0, 3),
                };
  inst p21[11] = {inst(LDXW, 2, 1, 4), // r2 = pkt_end_addr
                  inst(LDXW, 1, 1, 0), // r1 = pkt_start_addr
                  inst(MOV64XY, 3, 1),
                  inst(ADD64XC, 3, 1),  // r3 = pkt_start_addr + 1
                  inst(JGTXY, 2, 3, 2),
                  inst(MOV64XC, 0, 1),
                  inst(EXIT),
                  inst(LDXB, 3, 1, 0), // r3 = pkt[0]
                  inst(LDXB, 4, 2, 0), // r4 = pkt[1]
                  inst(ADD64XY, 3, 4),
                  inst(MOV64XY, 0, 3),
                 };
  vld.set_orig(p2, 11);
  print_test_res(vld.is_equal_to(p2, 11, p2, 11) == 1, "2.1");
  print_test_res(vld.is_equal_to(p2, 11, p21, 11) == 0, "2.2");

  inst p3[9] = {inst(LDXW, 2, 1, 4),
                inst(LDXW, 1, 1, 0),
                inst(MOV64XY, 3, 1),
                inst(ADD64XC, 3, 14),
                inst(JGTXY, 3, 2, 2),
                inst(MOV64XC, 0, 0),
                inst(EXIT),
                inst(MOV64XC, 0, 1),
                inst(EXIT),
               };
  inst p31[9] = {inst(LDXW, 2, 1, 4),
                 inst(LDXW, 1, 1, 0),
                 inst(MOV64XY, 3, 1),
                 inst(ADD64XC, 3, 14),
                 inst(JGTXY, 3, 2, 2),
                 inst(MOV64XC, 0, 2),
                 inst(EXIT),
                 inst(MOV64XC, 0, 1),
                 inst(EXIT),
                };
  vld.set_orig(p3, 9);
  print_test_res(vld.is_equal_to(p3, 9, p3, 9) == 1, "3.1");
  print_test_res(vld.is_equal_to(p3, 9, p31, 9) == 0, "3.2");
}


void test7() {
  std::cout << "test 7: test \"PGM_INPUT_skb\" program input type" << endl;
  std::cout << "1. test equivalence check" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_skb);
  mem_t::set_skb_max_sz(16);
  mem_t::set_pkt_sz(96);
  inst p1[2] = {inst(LDABSH, 0),
                inst(EXIT),
               };
  inst p1_2[2] = {inst(LDXH, 0, 1, 0),
                  inst(EXIT),
                 };
  validator vld(p1, 2);
  print_test_res(vld.is_equal_to(p1, 2, p1, 2) == 1, "1.1");
  print_test_res(vld.is_equal_to(p1, 2, p1_2, 2) == 1, "1.2");

  inst p2[3] = {inst(LDXW, 2, 1, SKB_data_s_off),  // r2 = skb_data_s
                inst(LDXB, 0, 2, 0),   // r0 = skb[0]
                inst(EXIT),
               };
  inst p2_1[3] = {inst(LDXW, 2, 1, SKB_data_s_off),  // r2 = skb_data_s
                  inst(LDXB, 0, 2, 1), // r0 = skb[1]
                  inst(EXIT),
                 };
  vld.set_orig(p2, 3);
  print_test_res(vld.is_equal_to(p2, 3, p2, 3) == 1, "2.1");
  print_test_res(vld.is_equal_to(p2, 3, p2_1, 3) == 0, "2.2");

  inst p3[4] = {inst(LDXW, 2, 1, SKB_data_s_off),  // r2 = skb_data_s
                inst(STB, 2, 0, 0xff), // skb[0] = 0xff
                inst(LDXB, 0, 2, 0),   // r0 = skb[0]
                inst(EXIT),
               };
  inst p3_1[4] = {inst(LDXW, 2, 1, SKB_data_s_off),  // r2 = skb_data_s
                  inst(STB, 2, 0, 0xff), // skb[0] = 0xff
                  inst(MOV64XC, 0, 0xff),   // r0 = 0xff
                  inst(EXIT),
                 };
  vld.set_orig(p3, 4);
  print_test_res(vld.is_equal_to(p3, 4, p3, 4) == 1, "3.1");
  print_test_res(vld.is_equal_to(p3, 4, p3_1, 4) == 1, "3.2");
}

void test8() {
  std::cout << "test 8: test tail call helper" << endl;
  std::cout << "1. test equivalence check" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::add_map(map_attr(8, 8, 3)); // k_sz: 1 bytes; v_sz: 1 byte; max_entirs: 3
  mem_t::add_map(map_attr(8, 8, 16));
  mem_t::set_pkt_sz(16);
  inst p1[6] = {inst(MOV64XC, 0, 0),
                inst(MOV64XY, 1, 1),
                inst(LDMAPID, 2, 0),
                inst(MOV64XC, 3, 1),
                inst(CALL, BPF_FUNC_tail_call),
                inst(EXIT),
               };
  inst p1_1[6] = {inst(MOV64XC, 0, 1), // r0 = 1
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_2[5] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1),
                  inst(EXIT),
                 };
  inst p1_3[6] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 2), // r3 = 2
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_4[6] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 0), // r1 = r0 = 1
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_5[6] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 1), // r2 = map1
                  inst(MOV64XC, 3, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_6[8] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1),
                  inst(JEQXC, 10, 514, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_7[7] = {inst(MOV64XC, 0, 0),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1),
                  inst(JEQXC, 10, 514, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  inst p1_8[8] = {inst(MOV64XC, 0, 1),
                  inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 0),
                  inst(JEQXC, 10, 514, 1),
                  inst(MOV64XC, 3, 1),
                  inst(CALL, BPF_FUNC_tail_call),
                  inst(EXIT),
                 };
  validator vld(p1, 6);
  print_test_res(vld.is_equal_to(p1, 6, p1, 6) == 1, "1.1");
  print_test_res(vld.is_equal_to(p1, 6, p1_1, 6) == 1, "1.2");
  print_test_res(vld.is_equal_to(p1, 6, p1_2, 5) == 0, "1.3");
  print_test_res(vld.is_equal_to(p1, 6, p1_3, 6) == 0, "1.4");
  print_test_res(vld.is_equal_to(p1, 6, p1_4, 6) == 0, "1.5");
  print_test_res(vld.is_equal_to(p1, 6, p1_5, 6) == 0, "1.6");
  print_test_res(vld.is_equal_to(p1, 6, p1_6, 8) == 1, "1.7");
  print_test_res(vld.is_equal_to(p1, 6, p1_7, 7) == 0, "1.8");
  print_test_res(vld.is_equal_to(p1, 6, p1_8, 8) == 0, "1.9");
}

void test9() {
  std::cout << "test 8: test BPF_FUNC_get_prandom_u32 helper" << endl;
  std::cout << "1. test equivalence check" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_constant);
  mem_t::_layout._n_randoms_u32 = 2;
  smt_var::init_static_variables();
  inst p1[5] = {inst(CALL, BPF_FUNC_get_prandom_u32),
                inst(MOV64XY, 1, 0),
                inst(CALL, BPF_FUNC_get_prandom_u32),
                inst(ADD64XY, 0, 1),
                inst(EXIT),
               };
  validator vld(p1, 5);
  print_test_res(vld.is_equal_to(p1, 5, p1, 5) == 1, "1.1");
}

void safety_check_of_orig_program(inst* p, int len, bool is_safe_expected, string test_name) {
  bool is_safe = true;
  try {
    validator vld(p, len);
  } catch (const string err_msg) {
    string unsafe_str = "original program is unsafe";
    if (err_msg == unsafe_str) {
      is_safe = false;
    }
  }
  print_test_res(is_safe == is_safe_expected, test_name);
}

void test10() {
  std::cout << "test 10: test safety check" << endl;
  std::cout << "1. stack aligned check" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(16);
  smt_var::init_static_variables();
  inst p1[2] = {inst(STDW, 10, -15, 1),
                inst(EXIT),
               };
  safety_check_of_orig_program(p1, 2, false, "1.1");
  inst p2[2] = {inst(STDW, 10, -16, 1),
                inst(EXIT),
               };
  safety_check_of_orig_program(p2, 2, true, "1.2");

  cout << "2. test counter example from unsafe synthesis" << endl;
  // validator vld();
  // chk_counterex_by_vld_to_interpreter(p2, 9, p21, 2, "2.1", vld, ps);
}

void test11() {
  cout << "test 11: test program equal cache" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(16);
  smt_var::init_static_variables();
  inst p1[4] = {inst(MOV64XC, 2, 0xff),
                inst(STXB, 1, 0, 2),
                inst(MOV64XC, 0, 0), // set the return register
                inst(EXIT),
               };
  validator vld(p1, 4);
  vld.is_equal_to(p1, 4, p1, 4);
  vld.is_equal_to(p1, 4, p1, 4);
  vld.is_equal_to(p1, 4, p1, 4);
  print_test_res(vld._count_solve_eq == 0, "1");

  inst p2[3] = {inst(MOV64XC, 0, 0xff),
                inst(MOV64XC, 0, 0),
                inst(EXIT),
               };
  inst p2_1[3] = {inst(NOP),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  vld.set_orig(p2, 3);
  vld.is_equal_to(p2, 3, p2_1, 3);
  vld.is_equal_to(p2, 3, p2, 3);
  print_test_res(vld._count_solve_eq == 1, "2");
}

void chk_counterex_by_vld_to_interpreter_delta(inst* p, inst* delta, int start, int len,
    string test_name, int counterex_type) {
  inst* p_1 = new inst[inst::max_prog_len];
  for (int i = 0; i < inst::max_prog_len; i++) {
    p_1[i] = p[i];
  }
  for (int i = 0; i < len; i++) {
    p_1[i + start] = delta[i];
  }
  chk_counterex_by_vld_to_interpreter(p, inst::max_prog_len, p_1, inst::max_prog_len,
                                      test_name, counterex_type, false);
  delete []p_1;
}

// bpf_sock.o section: rcv-sock4
inst rcv_sock4[91] = {inst(191, 1, 6, 0, 0),
                      inst(183, 0, 1, 0, 0),
                      inst(97, 6, 2, 36, 0),
                      inst(86, 0, 2, 4, 6),
                      inst(CALL, BPF_FUNC_get_prandom_u32),
                      inst(188, 0, 1, 0, 0),
                      inst(103, 0, 1, 0, 32),
                      inst(119, 0, 1, 0, 32),
                      inst(123, 1, 10, -40, 0),
                      inst(97, 6, 1, 4, 0),
                      inst(99, 1, 10, -32, 0),
                      inst(97, 6, 1, 24, 0),
                      inst(99, 1, 10, -16, 0),
                      inst(180, 0, 8, 0, 0),
                      inst(107, 8, 10, -26, 0),
                      inst(97, 10, 1, -16, 0),
                      inst(107, 1, 10, -28, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -40),
                      inst(LDMAPID, 1, 0),
                      inst(0, 0, 0, 0),
                      inst(133, 0, 0, 0, 1),
                      inst(191, 0, 7, 0, 0),
                      inst(21, 0, 7, 65, 0),
                      inst(97, 7, 1, 0, 0),
                      inst(99, 1, 10, -56, 0),
                      inst(105, 7, 1, 4, 0),
                      inst(107, 8, 10, -48, 0),
                      inst(107, 8, 10, -50, 0),
                      inst(107, 8, 10, -46, 0),
                      inst(107, 1, 10, -52, 0),
                      inst(22, 0, 1, 10, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -56),
                      inst(LDMAPID, 1, 1),
                      inst(0, 0, 0, 0),
                      inst(133, 0, 0, 0, 1),
                      inst(21, 0, 0, 2, 0),
                      inst(105, 0, 1, 4, 0),
                      inst(86, 0, 1, 10, 0),
                      inst(180, 0, 1, 0, 0),
                      inst(107, 1, 10, -52, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -56),
                      inst(LDMAPID, 1, 1),
                      inst(0, 0, 0, 0),
                      inst(133, 0, 0, 0, 1),
                      inst(21, 0, 0, 5, 0),
                      inst(105, 0, 1, 4, 0),
                      inst(22, 0, 1, 3, 0),
                      inst(105, 7, 1, 6, 0),
                      inst(105, 0, 2, 6, 0),
                      inst(30, 1, 2, 20, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -40),
                      inst(LDMAPID, 1, 0),
                      inst(0, 0, 0, 0),
                      inst(133, 0, 0, 0, 3),
                      inst(183, 0, 6, 0, 0),
                      inst(123, 6, 10, -8, 0),
                      inst(123, 6, 10, -16, 0),
                      inst(183, 0, 1, 0, 264),
                      inst(123, 1, 10, -24, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -24),
                      inst(LDMAPID, 1, 2),
                      inst(0, 0, 0, 0),
                      inst(133, 0, 0, 0, 1),
                      inst(21, 0, 0, 9, 0),
                      inst(121, 0, 1, 0, 0),
                      inst(7, 0, 1, 0, 1),
                      inst(123, 1, 0, 0, 0),
                      inst(5, 0, 0, 16, 0),
                      inst(97, 7, 1, 0, 0),
                      inst(99, 1, 6, 4, 0),
                      inst(105, 7, 1, 4, 0),
                      inst(99, 1, 6, 24, 0),
                      inst(5, 0, 0, 11, 0),
                      inst(123, 6, 10, -8, 0),
                      inst(183, 0, 1, 0, 1),
                      inst(123, 1, 10, -16, 0),
                      inst(191, 10, 2, 0, 0),
                      inst(7, 0, 2, 0, -24),
                      inst(191, 10, 3, 0, 0),
                      inst(7, 0, 3, 0, -16),
                      inst(LDMAPID, 1, 2),
                      inst(0, 0, 0, 0),
                      inst(180, 0, 4, 0, 0),
                      inst(133, 0, 0, 0, 2),
                      inst(180, 0, 0, 0, 1),
                      inst(149, 0, 0, 0, 0),
                     };

void test12() {
  cout << "1. counter-example from cilium rcv-sock4" << endl;
  const int pgm_len = 91;
  mem_t::_layout.clear();
  inst::max_prog_len = 91;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, 91));
  mem_t::add_map(map_attr(96, 96, 91));
  mem_t::add_map(map_attr(64, 128, 91));
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();
  inst p_unsafe_1[] = {inst(LDXB, 8, 10, 10),
                       inst(AND32XC, 2, 264),
                       inst(STXH, 10, -26, 8),
                       inst(MOV64XY, 5, 6),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_1, 12, 4, "1.1", COUNTEREX_safety_check);
  inst p_unsafe_2[] = {inst(LDXH, 8, 10, 2),
                       inst(NOP),
                       inst(NOP),
                       inst(STW, 10, -28, 5),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_2, 12, 4, "1.2", COUNTEREX_safety_check);
  inst p_unsafe_3[] = {inst(LDXB, 8, 10, 4),
                       inst(ARSH32XC, 8, 7),
                       inst(STXH, 10, -26, 8),
                       inst(ARSH64XC, 2, 36),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_3, 12, 4, "1.3", COUNTEREX_safety_check);
  inst p_unsafe_4[] = {inst(LDXH, 7, 10, -1),
                       inst(MOV32XC, 8, 2),
                       inst(STXH, 10, -26, 7),
                       inst(RSH32XC, 8, 22),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_4, 12, 4, "1.4", COUNTEREX_safety_check);
  inst p_unsafe_5[] = {inst(LDMAPID, 0, 0),
                       inst(LDXW, 8, 10, -1),
                       inst(STXH, 10, -26, 8),
                       inst(JEQ32XC, 8, -40, 33),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_5, 12, 4, "1.5", COUNTEREX_safety_check);
  // p_unsafe_6 sometimes unsafe
  inst p_unsafe_6[] = {inst(STXH, 10, -48, 8),
                       inst(STXW, 10, -52, 1),
                       inst(STXH, 10, -46, 8),
                       inst(MOV64XY, 1, 6),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_6, 27, 4, "1.6", COUNTEREX_safety_check);
  // p_unsafe_7 sometimes unsafe
  inst p_unsafe_7[] = {inst(STXH, 10, -48, 8),
                       inst(STXW, 10, -52, 1),
                       inst(STXH, 10, -46, 8),
                       inst(MOV32XC, 1, -56),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_7, 27, 4, "1.7", COUNTEREX_safety_check);
  // p_unsafe_8 sometimes unsafe
  inst p_unsafe_8[] = {inst(STXH, 10, -48, 8),
                       inst(STXW, 10, -52, 1),
                       inst(STXH, 10, -46, 8),
                       inst(OR64XY, 1, 6),
                      };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_unsafe_8, 27, 4, "1.8", COUNTEREX_safety_check);

  inst p_uneq_1[] = {inst(STXH, 10, -48, 8),
                     inst(STXW, 10, -52, 1),
                     inst(STXH, 10, -46, 8),
                     inst(MOV32XY, 1, 7),
                    };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_uneq_1, 27, 4, "2.1", COUNTEREX_eq_check);
  inst p_uneq_2[] = {inst(MOV64XY, 5, 6),
                     inst(MOV32XC, 8, 0),
                     inst(STXH, 10, -26, 2),
                     inst(NOP),
                    };
  chk_counterex_by_vld_to_interpreter_delta(rcv_sock4, p_uneq_2, 12, 4, "2.2", COUNTEREX_eq_check);

  // set the static variable back
  inst::max_prog_len = TEST_PGM_MAX_LEN;
}

inst from_network[38] = {inst(180, 0, 2, 0, 0),
                         inst(99, 2, 1, 64, 0),
                         inst(183, 0, 2, 0, 0),
                         inst(99, 2, 1, 60, 0),
                         inst(99, 2, 1, 56, 0),
                         inst(99, 2, 1, 52, 0),
                         inst(99, 2, 1, 48, 0),
                         inst(97, 1, 6, 0, 0),
                         inst(123, 2, 10, -8, 0),
                         inst(123, 2, 10, -16, 0),
                         inst(183, 0, 1, 0, 259),
                         inst(123, 1, 10, -24, 0),
                         inst(191, 10, 2, 0, 0),
                         inst(7, 0, 2, 0, -24),
                         inst(24, 0, 1, 0, 0), // ldmapid r1 = 0
                         inst(0, 0, 0, 0, 0),
                         inst(133, 0, 0, 0, 1),
                         inst(21, 0, 0, 7, 0),
                         inst(121, 0, 1, 0, 0),
                         inst(7, 0, 1, 0, 1),
                         inst(123, 1, 0, 0, 0),
                         inst(121, 0, 1, 8, 0),
                         inst(15, 6, 1, 0, 0),
                         inst(123, 1, 0, 8, 0),
                         inst(5, 0, 0, 11, 0),
                         inst(183, 0, 1, 0, 1),
                         inst(123, 1, 10, -16, 0),
                         inst(123, 6, 10, -8, 0),
                         inst(191, 10, 2, 0, 0),
                         inst(7, 0, 2, 0, -24),
                         inst(191, 10, 3, 0, 0),
                         inst(7, 0, 3, 0, -16),
                         inst(24, 0, 1, 0, 0), // ldmapid r1 = 0
                         inst(0, 0, 0, 0),
                         inst(180, 0, 4, 0, 0),
                         inst(133, 0, 0, 0, 2),
                         inst(180, 0, 0, 0, 0),
                         inst(149, 0, 0, 0, 0),
                        };

void test13() {
  cout << "test 13: window program test" << endl;
  inst p1[] = {inst(),
               inst(MOV64XC, 2, 2),
               inst(MOV64XY, 0, 2),
              };
  inst p1_2[] = {inst(),
                 inst(MOV64XC, 2, 3),
                 inst(MOV64XY, 0, 2),
                };
  int win_start = 1, win_end = 1;
  bool enable_win = true;
  validator vld(p1, 3, enable_win, win_start, win_end);
  vld._enable_prog_eq_cache = false;

  validator vld1;
  vld1._enable_prog_eq_cache = false;
  print_test_res(vld.is_equal_to(p1, 3, p1_2, 3) == 0, "1");

  inst p2[] = {inst(),
               inst(),
               inst(STB, 10, -1, 0xff),
               inst(LDXB, 1, 10, -1),
               inst(MOV64XY, 0, 1),
              };
  inst p2_1[] = {inst(),
                 inst(MOV64XC, 2, 0xff),
                 inst(STXB, 10, -1, 2),
                 inst(LDXB, 1, 10, -1),
                 inst(MOV64XY, 0, 1),
                };
  win_start = 1, win_end = 2;
  vld.set_orig(p2, 5, win_start, win_end);
  print_test_res(vld.is_equal_to(p2, 5, p2_1, 5) == 1, "2");

  mem_t::_layout.clear();
  const int prog_len = 91;
  inst::max_prog_len = prog_len;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(128);
  mem_t::add_map(map_attr(128, 64, 91));
  mem_t::add_map(map_attr(96, 96, 91));
  mem_t::add_map(map_attr(64, 128, 91));
  mem_t::_layout._n_randoms_u32 = 1;
  smt_var::init_static_variables();
  win_start = 4;
  win_end = 7;
  vld.set_orig(rcv_sock4, prog_len, win_start, win_end);
  inst rcv_sock4_1[prog_len];
  for (int i = 0; i < prog_len; i++) rcv_sock4_1[i] = rcv_sock4[i];
  rcv_sock4_1[6] = inst();
  rcv_sock4_1[7] = inst();
  print_test_res(vld.is_equal_to(rcv_sock4, prog_len, rcv_sock4_1, prog_len) == 1, "rcv-sock4 1");
  vld1.set_orig(rcv_sock4, prog_len);
  cout << "vld1" << endl;
  vld1.is_equal_to(rcv_sock4, prog_len, rcv_sock4_1, prog_len);
  cout << "vld1 end" << endl;

  win_start = 5;
  win_end = 7;
  vld.set_orig(rcv_sock4, prog_len, win_start, win_end);
  print_test_res(vld.is_equal_to(rcv_sock4, prog_len, rcv_sock4_1, prog_len) == 1, "rcv-sock4 2");
  cout << "vld1" << endl;
  vld1.set_orig(rcv_sock4, prog_len);
  vld1.is_equal_to(rcv_sock4, prog_len, rcv_sock4_1, prog_len);
  cout << "vld1 end" << endl;

  win_start = 27;
  win_end = 30;
  vld.set_orig(rcv_sock4, prog_len, win_start, win_end);
  print_test_res(vld.is_equal_to(rcv_sock4, prog_len, rcv_sock4, prog_len) == 1, "rcv-sock4 3");
  cout << "vld1" << endl;
  vld1.set_orig(rcv_sock4, prog_len);
  vld1.is_equal_to(rcv_sock4, prog_len, rcv_sock4, prog_len);
  cout << "vld1 end" << endl;
  inst::max_prog_len = TEST_PGM_MAX_LEN;

  // test from-network
  mem_t::_layout.clear();
  const int prog_len_fn = 38;
  inst::max_prog_len = prog_len_fn;
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(68);
  mem_t::add_map(map_attr(64, 128, prog_len_fn));
  smt_var::init_static_variables();
  win_start = 0;
  win_end = 6;
  vld.set_orig(from_network, prog_len_fn, win_start, win_end);
  vld1.set_orig(from_network, prog_len_fn);

  inst from_network_1[prog_len_fn];
  for (int i = 0; i < prog_len_fn; i++) from_network_1[i] = from_network[i];
  from_network_1[2] = inst();
  vld.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn);
  cout << "vld1" << endl;
  vld1.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn);
  cout << "vld1 end" << endl;

  for (int i = 0; i < prog_len_fn; i++) from_network_1[i] = from_network[i];
  from_network_1[3] = inst();
  from_network_1[4] = inst();
  from_network_1[5] = inst(STXDW, 1, 48, 2);
  from_network_1[6] = inst(STXDW, 1, 56, 2);
  print_test_res(vld.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn) == 1, "from-network 1");
  cout << "vld1" << endl;
  vld1.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn);
  cout << "vld1 end" << endl;

  win_start = 8;
  win_end = 9;
  vld.set_orig(from_network, prog_len_fn, win_start, win_end);
  for (int i = 0; i < prog_len_fn; i++) from_network_1[i] = from_network[i];
  from_network_1[8] = inst();
  from_network_1[9] = inst();
  print_test_res(vld.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn) == 1, "from-network 2");
  cout << "vld1" << endl;
  vld1.is_equal_to(from_network, prog_len_fn, from_network_1, prog_len_fn);
  cout << "vld1 end" << endl;

  inst::max_prog_len = TEST_PGM_MAX_LEN;
}

void chk_counterex_by_vld_to_interpreter_win(inst* p1, int len1, inst* p2, int len2,
    int win_start, int win_end, string test_name) {
  bool interpret_check = false;
  bool is_win = true;
  chk_counterex_by_vld_to_interpreter(p1, len1, p2, len2, test_name, COUNTEREX_eq_check,
                                      interpret_check, is_win, win_start, win_end);
}

void test14() {
  cout << "test 13: window program counter-example" << endl;
  inst p1[] = {inst(MOV64XC, 2, 2),
               inst(MOV64XY, 0, 2),
              };
  inst p1_2[] = {inst(MOV64XC, 2, 3),
                 inst(MOV64XY, 0, 2),
                };
  int win_start = 0, win_end = 0;
  chk_counterex_by_vld_to_interpreter_win(p1, 2, p1_2, 2, win_start, win_end, "1");

  inst p2[] = {inst(STB, 10, -1, 0xff),
               inst(LDXB, 1, 10, -1),
               inst(ADD64XC, 1, 1),
               inst(MOV64XY, 0, 1),
               inst(EXIT),
              };
  inst p2_2[] = {inst(STB, 10, -1, 0xff),
                 inst(LDXB, 1, 10, -1),
                 inst(ADD64XC, 1, 2),
                 inst(MOV64XY, 0, 1),
                 inst(EXIT),
                };
  win_start = 1;
  win_end = 2;
  chk_counterex_by_vld_to_interpreter_win(p2, 5, p2_2, 5, win_start, win_end, "2");
}

int main() {
  // set for prog_eq_cache, if the prog len in the unit tests > 20,
  // please update inst::max_prog_len here
  inst::max_prog_len = TEST_PGM_MAX_LEN;
  try {
    test13();
    test14();
    return 0;
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
    test12();
  } catch (const string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }


  return 0;
}
