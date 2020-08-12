#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "validator.h"

using namespace z3;

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
  print_test_res(vld.is_equal_to(instructions1, 9, instructions1, 9), "instructions1 == instructions1");
  print_test_res(vld.is_equal_to(instructions1, 9, instructions2, 9), "instructions1 == instructions2");

  // output = L32(input)
  inst instructions3[2] = {inst(MOV32XY, 0, 1),
                           inst(EXIT),
                          };

  inst instructions4[3] = {inst(STXW, 10, -4, 1),
                           inst(LDXW, 0, 10, -4),
                           inst(EXIT),
                          };
  vld.set_orig(instructions3, 2);
  print_test_res(vld.is_equal_to(instructions3, 2, instructions4, 3), "instructions3 == instructions4");

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
  vld.set_orig(instructions5, 3);
  print_test_res(vld.is_equal_to(instructions5, 3, instructions6, 9), "instructions5 == instructions6");
  print_test_res(vld.is_equal_to(instructions5, 3, instructions7, 4), "instructions5 == instructions7");

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
  vld.set_orig(instructions8, 10);
  print_test_res(vld.is_equal_to(instructions8, 10, instructions8, 10), "instructions8 == instructions8");
  print_test_res(vld.is_equal_to(instructions8, 10, instructions9, 10), "instructions8 == instructions9");
  print_test_res(vld.is_equal_to(instructions8, 10, instructions10, 8), "instructions8 == instructions10");

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
  vld.set_orig(instructions11, 4);
  print_test_res(vld.is_equal_to(instructions11, 4, instructions11, 4), "instructions11 == instructions11");
  print_test_res(vld.is_equal_to(instructions11, 4, instructions12, 3), "instructions11 == instructions12");

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
  vld.set_orig(instructions13, 4);
  print_test_res(vld.is_equal_to(instructions13, 4, instructions13, 4), "instructions13 == instructions13");
  print_test_res(vld.is_equal_to(instructions13, 4, instructions14, 3), "instructions13 == instructions14");
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
  validator vld(p1, 6);
  print_test_res(vld.is_equal_to(p1, 6, p2, 4), "p1 == p2");

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
  vld.set_orig(p3, 5);
  print_test_res(vld.is_equal_to(p3, 5, p4, 4), "p3 == p4");

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
  vld.set_orig(p5, 8);
  print_test_res(vld.is_equal_to(p5, 8, p6, 7), "p5 == p6");

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
  vld.set_orig(p7, 8);
  print_test_res(vld.is_equal_to(p7, 8, p7, 8), "p7 == p7");
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
                 inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
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
                  inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                  inst(LDXB, 0, 10, -2),
                  inst(EXIT),
                 };
  validator vld(p1, 13);
  print_test_res(vld.is_equal_to(p1, 13, p1, 13), "map helper function 1.1");
  print_test_res(vld.is_equal_to(p1, 13, p11, 11), "map helper function 1.2");

  // r0 = *(lookup &k (delete &k (update &k &v m))), where k = 0x11, v = L8(input)
  inst p2[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = map_id (0)
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
                  inst(LDMAPID, 1, 0), // r1 = map_id (0)
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
  vld.set_orig(p2, 14);
  print_test_res(vld.is_equal_to(p2, 14, p2, 14), "map helper function 2.1");
  print_test_res(vld.is_equal_to(p2, 14, p21, 13), "map helper function 2.2");
  // r0 = *(lookup &k m), where k = 0x11, v = L8(input)
  inst p3[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, 0), // r1 = map_id (0)
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                inst(LDXB, 0, 0, 0),
                inst(EXIT),
               };
  vld.set_orig(p3, 9);
  print_test_res(vld.is_equal_to(p3, 9, p3, 9), "map helper function 3.1");

  inst p4[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = 0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update),
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = map0[k1]
                 inst(LDXB, 0, 0, 0),  // r0 = map0[k1]
                 inst(EXIT),
                };
  inst p41[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, 0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                 inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = map0[k1]
                 inst(LDXB, 0, 0, 0),  // r0 = map0[k1]
                 inst(EXIT),
                };
  vld.set_orig(p4, 13);
  print_test_res(vld.is_equal_to(p4, 13, p41, 9) == 0, "map helper function 4.1");

  // upd &k1 &input (del &k1 m0), output = 0
  inst p5[12] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_delete),
                 inst(CALL, BPF_FUNC_map_update),
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
                  inst(CALL, BPF_FUNC_map_update),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  vld.set_orig(p5, 12);
  print_test_res(vld.is_equal_to(p5, 12, p5, 12) == 1, "map helper function 5.1");
  print_test_res(vld.is_equal_to(p5, 12, p51, 11) == 1, "map helper function 5.2");

  // if k1 in map0, map0[k1]+=1, output=0
  inst p6[12] = {inst(MOV64XC, 1, k1),
                 inst(STXB, 10, -1, 1), // *(r10-1) = k1
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2 = (r10-1)
                 inst(ADD64XC, 2, -1),
                 inst(CALL, BPF_FUNC_map_lookup), // r0 = &v = lookup k1 map0
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
                  inst(CALL, BPF_FUNC_map_lookup), // r0 = &v = lookup k1 map0
                  inst(JEQXC, 0, 0, 8), // if r0 == 0, exit
                  inst(LDXB, 1, 0, 0), // r1 = v
                  inst(ADD64XC, 1, 1), // r1 += 1
                  inst(STXB, 10, -2, 1), // *(r10-2) = r1
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 3, 10), // r3 = r10-2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_update),
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  vld.set_orig(p6, 12);
  print_test_res(vld.is_equal_to(p6, 12, p6, 12) == 1, "map helper function 6.1");
  print_test_res(vld.is_equal_to(p6, 12, p61, 16) == 1, "map helper function 6.2");
  vld.set_orig(p61, 16);
  print_test_res(vld.is_equal_to(p61, 16, p61, 16) == 1, "map helper function 6.3");
  print_test_res(vld.is_equal_to(p61, 16, p6, 12) == 1, "map helper function 6.4");

  inst p7[19] = {inst(STB, 10, -1, 1), // *(r10-1) = 1
                 inst(STB, 10, -2, 2), // *(r10-2) = 2
                 inst(MOV64XY, 2, 10), // r2 = r10-1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3 = r10-1
                 inst(ADD64XC, 3, -1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(CALL, BPF_FUNC_map_update), // map0[1] = 1
                 inst(MOV64XY, 3, 10), // r3 = r10-2
                 inst(ADD64XC, 3, -2),
                 inst(LDMAPID, 1, map2), // r1 = map2
                 inst(CALL, BPF_FUNC_map_update), // map2[1] = 2
                 inst(LDMAPID, 1, map0), // r1 = map0, // 13
                 inst(JGTXY, 10, 9, 1),
                 inst(LDMAPID, 1, map2), // r1 = map2
                 inst(CALL, BPF_FUNC_map_lookup),
                 inst(JEQXC, 0, 0, 1),
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                };
  inst p71[16] = {inst(STB, 10, -1, 1), // *(r10-1) = 1
                  inst(STB, 10, -2, 2), // *(r10-2) = 2
                  inst(MOV64XY, 2, 10), // r2 = r10-1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3 = r10-1
                  inst(ADD64XC, 3, -1),
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(CALL, BPF_FUNC_map_update), // map0[1] = 1
                  inst(MOV64XY, 3, 10), // r3 = r10-2
                  inst(ADD64XC, 3, -2),
                  inst(LDMAPID, 1, map2), // r1 = map2
                  inst(CALL, BPF_FUNC_map_update), // map2[1] = 2
                  inst(MOV64XC, 0, 1),
                  inst(JGTXY, 10, 9, 1),
                  inst(MOV64XC, 0, 2),
                  inst(EXIT),
                 };
  vld.set_orig(p7, 19);
  print_test_res(vld.is_equal_to(p7, 19, p7, 19) == 1, "map helper function 7.1");
  print_test_res(vld.is_equal_to(p7, 19, p71, 16) == 1, "map helper function 7.2");

  inst p8[16] = {inst(STB, 10, -2, 0),
                 inst(MOV64XC, 1, k1), // *addr_k = 0x11
                 inst(STXB, 10, -1, 1),
                 inst(LDMAPID, 1, map0), // r1 = map0
                 inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                 inst(ADD64XC, 2, -1),
                 inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                 inst(ADD64XC, 3, -2),
                 inst(CALL, BPF_FUNC_map_update),
                 inst(JEQXC, 10, 0xfff, 2),
                 inst(STB, 10, -2, 1),
                 inst(CALL, BPF_FUNC_map_update),
                 inst(CALL, BPF_FUNC_map_lookup),
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
                  inst(CALL, BPF_FUNC_map_update),
                  inst(JNEXC, 10, 0xfff, 2),
                  inst(STB, 10, -2, 0),
                  inst(CALL, BPF_FUNC_map_update),
                  inst(MOV64XC, 0, 0),
                  inst(JEQXC, 10, 0xfff, 1),
                  inst(MOV64XC, 0, 1),
                  inst(EXIT),
                 };
  vld.set_orig(p8, 16);
  print_test_res(vld.is_equal_to(p8, 16, p81, 16) == 1, "map helper function 8.1");

  // modify a part of map1[1]
  // mem_t::add_map(map_attr(16, 32, 32));  k_sz: 2 bytes, v_sz: 4 bytes
  inst p9[9] = {inst(STH, 10, -2, 1), // k = 1
                inst(MOV64XY, 2, 10),
                inst(ADD64XC, 2, -2),
                inst(LDMAPID, map1),
                inst(CALL, BPF_FUNC_map_lookup),
                inst(JEQXC, 0, 0, 1),
                inst(STB, 0, 0, 1), // set map1[1][0] = 0, not modify map1[1][1...3]
                inst(MOV64XC, 0, 1), // set the return value as 1
                inst(EXIT),
               };
  vld.set_orig(p9, 9);
  print_test_res(vld.is_equal_to(p9, 9, p9, 9) == 1, "map helper function 9.1");
}

void chk_counterex_by_vld_to_interpreter(inst* p1, int len1, inst* p2, int len2,
    string test_name, validator& vld, prog_state& ps) {
  vld.set_orig(p1, len1);
  bool res_expected = (vld.is_equal_to(p1, len1, p2, len2) == 0); // 0: not equal
  inout_t input, output0, output1;
  input.init();
  output0.init();
  output1.init();
  interpret(output0, p1, len1, ps, input);
  interpret(output1, p2, len2, ps, input);
  res_expected = res_expected && (output0 == output1);
  output0.clear();
  output1.clear();
  interpret(output0, p1, len1, ps, vld._last_counterex.input);
  interpret(output1, p2, len2, ps, vld._last_counterex.input);
  res_expected = res_expected && (!(output0 == output1));
  res_expected = res_expected && (output0 == vld._last_counterex.output);
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
                  inst(LDMAPID, 1, map0), // r1 = map0
                  inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                  inst(ADD64XC, 2, -1),
                  inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                  inst(ADD64XC, 3, -2),
                  inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                  inst(MOV64XC, 0, 0), // set the return value r0 = 0
                  inst(EXIT),
                 };
  validator vld(p1, 14);
  inout_t input_expected;
  input_expected.init();
  // check counter example is generated and the value of input memory
  bool res_expected = (vld.is_equal_to(p1, 14, p11, 11) == 0);// 0: not equal
  // do not need to check the input register
  input_expected.reg = vld._last_counterex.input.reg;
  input_expected.update_kv(map0, k1_str, vector<uint8_t> {(uint8_t)v1});
  res_expected = res_expected && (vld._last_counterex.input == input_expected);
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
                  inst(CALL, BPF_FUNC_map_lookup),
                  inst(JEQXC, 0, 0, 1),
                  inst(CALL, BPF_FUNC_map_update),
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
                inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
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
                  inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v1
                  inst(JEQXC, 0, 0, 3), // if r0 == 0, exit else r0 = map1[k1]
                  inst(LDXB, 0, 0, 0),
                  inst(JEQXC, 0, v1, 1), //if r0 == v1 exit else r0 = 0
                  inst(MOV64XC, 0, 0),
                  inst(EXIT),
                 };
  chk_counterex_by_vld_to_interpreter(p2, 9, p21, 2, "2.1", vld, ps);
  chk_counterex_by_vld_to_interpreter(p2, 9, p22, 11, "2.2", vld, ps);

  // r0 = 0xfffffffe if k1 not in map0, else r0 = 0; del map0[k1]
  inst p3[7] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, map0), // r1 = map0
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_delete),
                inst(EXIT),
               };
  // r0 = 0xfffffffe
  inst p31[2] = {inst(MOV32XC, 0, 0xfffffffe),
                 inst(EXIT),
                };
  chk_counterex_by_vld_to_interpreter(p3, 7, p31, 2, "3", vld, ps);

  // r0 = 1 if k1 in map0, else r0 = 0
  inst p4[9] = {inst(MOV64XC, 1, k1), // *addr_k = 0x11
                inst(STXB, 10, -1, 1),
                inst(LDMAPID, 1, map0), // r1 = map0
                inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup),
                inst(JEQXC, 0, 0, 1),
                inst(MOV64XC, 0, 1),
                inst(EXIT),
               };
  // r0 = 0
  inst p41[2] = {inst(MOV32XC, 0, 0),
                 inst(EXIT),
                };
  chk_counterex_by_vld_to_interpreter(p4, 9, p41, 2, "4", vld, ps);

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
  chk_counterex_by_vld_to_interpreter(p5, 3, p51, 7, "1", vld, ps);
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
  validator vld(p1, 3);
  print_test_res(vld.is_equal_to(p1, 3, p1, 3) == 1, "1");

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
  vld.set_orig(p2, 4);
  print_test_res(vld.is_equal_to(p2, 4, p1, 3) == 0, "2");
  print_test_res(vld.is_equal_to(p2, 4, p2, 4) == 1, "3");
  print_test_res(vld.is_equal_to(p2, 4, p3, 5) == 0, "4");
  print_test_res(vld.is_equal_to(p2, 4, p4, 6) == 1, "5");
  vld.set_orig(p3, 5);
  print_test_res(vld.is_equal_to(p3, 5, p3, 5) == 1, "6");
  vld.set_orig(p4, 6);
  print_test_res(vld.is_equal_to(p4, 6, p4, 6) == 1, "7");

  inst p5[9] = {inst(STB, 10, -1, 1),
                inst(STB, 1, 1, 2),
                inst(MOV64XY, 2, 10),
                inst(ADD64XC, 2, -1),
                inst(JGTXY, 10, 0, 2),
                inst(MOV64XY, 2, 1),
                inst(ADD64XC, 2, 1),
                inst(LDXB, 0, 2, 0),
                inst(EXIT),
               };
  inst p6[5] = {inst(STB, 1, 1, 2),
                inst(MOV64XC, 0, 1),
                inst(JGTXY, 10, 0, 1),
                inst(MOV64XC, 1, 2),
                inst(EXIT),
               };
  vld.set_orig(p5, 9);
  print_test_res(vld.is_equal_to(p5, 9, p5, 9) == 1, "8");
  print_test_res(vld.is_equal_to(p5, 9, p6, 5) == 1, "9");

  // test address track of addxy
  inst p7[3] = {inst(ADD64XC, 1, 1),
                inst(STB, 1, 0, 0xff),
                inst(EXIT),
               };
  inst p8[2] = {inst(STB, 1, 1, 0xff),
                inst(EXIT),
               };
  vld.set_orig(p7, 3);
  print_test_res(vld.is_equal_to(p7, 3, p8, 2) == 1, "9");
}

int main() {
  try {
    test1();
    test2();
    test3();
    test4();
    test5();
  } catch (const string err_msg) {
    cout << err_msg << endl;
  }


  return 0;
}
