#include <iostream>
#include "../../../src/utils.h"
#include "canonicalize.h"

bool are_same_progs(inst* p1, int len1, inst* p2, int len2) {
  if (len1 != len2) return false;
  for (int i = 0; i < len1; i++) {
    if (! (p1[i] == p2[i])) return false;
  }
  return true;
}

void canonicalize_check(inst* prog, int len, inst* expected_prog, string test_name) {
  canonicalize(prog, len);
  bool is_equal = are_same_progs(prog, len, expected_prog, len);
  print_test_res(is_equal, test_name);
}

bool check_min_pkt_sz(vector<unsigned int>& expected, vector<inst_static_state>& actual) {
  if (expected.size() != actual.size()) return false;
  for (int i = 0; i < expected.size(); i++) {
    if (expected[i] != actual[i].min_pkt_sz) return false;
  }
  return true;
}

void test1() {
  cout << "Test 1: program canonicalize check" << endl;
  inst p1[] = {inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  inst expected_prog1[] = {inst(MOV64XC, 0, 0),
                           inst(EXIT),
                          };
  canonicalize_check(p1, sizeof(p1) / sizeof(inst), expected_prog1, "1");

  inst p2[] = {inst(MOV64XC, 0, -1),
               inst(ADD64XY, 0, 0),
               inst(EXIT),
              };
  inst expected_prog2[] = {inst(MOV64XC, 0, -1),
                           inst(ADD64XY, 0, 0),
                           inst(EXIT),
                          };
  canonicalize_check(p2, sizeof(p2) / sizeof(inst), expected_prog2, "2");

  inst p3[] = {inst(STXB, 10, -2, 1), // *addr_v = r1
               inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
               inst(STXB, 10, -1, 1),
               INSN_LDMAPID(1, 0), // r1 = map_id (0)
               inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
               inst(ADD64XC, 2, -1),
               inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
               inst(ADD64XC, 3, -2),
               inst(CALL, BPF_FUNC_map_update_elem), // map0[k] = v, i.e., map0[0x11] = L8(input)
               inst(LDXB, 0, 0, 0),
               inst(EXIT),
              };
  inst expected_prog3[] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                           inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                           inst(STXB, 10, -1, 1),
                           INSN_LDMAPID(1, 0), // r1 = map_id (0)
                           inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                           inst(ADD64XC, 2, -1),
                           inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                           inst(ADD64XC, 3, -2),
                           inst(CALL, BPF_FUNC_map_update_elem), // map0[k] = v, i.e., map0[0x11] = L8(input)
                           inst(LDXB, 0, 0, 0),
                           inst(EXIT),
                          };
  canonicalize_check(p3, sizeof(p3) / sizeof(inst), expected_prog3, "3");

  inst p4[] = {inst(MOV64XC, 2, 10),
               inst(ADD64XC, 2, -8),
               inst(CALL, BPF_FUNC_map_lookup_elem),
              };
  inst expected_prog4[] = {inst(MOV64XC, 2, 10),
                           inst(ADD64XC, 2, -8),
                           inst(CALL, BPF_FUNC_map_lookup_elem),
                          };
  canonicalize_check(p4, sizeof(p4) / sizeof(inst), expected_prog4, "4");

  inst p5[] = {inst(LDXB, 1, 1, 0),
               inst(ADD64XC, 1, 5),
               inst(MOV64XY, 2, 10),
               inst(EXIT),
              };
  inst expected_prog5[] = {inst(EXIT),
                           inst(),
                           inst(),
                           inst(),
                          };
  canonicalize_check(p5, sizeof(p5) / sizeof(inst), expected_prog5, "5");

  inst p6[] = {inst(LDXB, 1, 1, 0),
               inst(STXB, 10, -1, 1),
               inst(MOV64XY, 2, 10),
               inst(EXIT),
              };
  inst expected_prog6[] = {inst(LDXB, 1, 1, 0),
                           inst(STXB, 10, -1, 1),
                           inst(EXIT),
                           inst(),
                          };
  canonicalize_check(p6, sizeof(p6) / sizeof(inst), expected_prog6, "6");

  cout << "Program with branches" << endl;
  inst p2_1[] = {inst(JEQXY, 1, 2, 2),
                 inst(ADD64XC, 3, 1),
                 inst(EXIT),
                 inst(ADD64XC, 4, 1),
                 inst(EXIT),
                };
  inst expected_prog2_1[] = {inst(JEQXY, 1, 2, 1),
                             inst(EXIT),
                             inst(EXIT),
                             inst(),
                             inst(),
                            };
  canonicalize_check(p2_1, sizeof(p2_1) / sizeof(inst), expected_prog2_1, "2.1");

  inst p2_2[] = {inst(MOV64XC, 0, 3),
                 inst(MOV64XC, 2, 1),
                 inst(JEQXY, 1, 2, 3),
                 inst(ADD64XC, 3, 1),
                 inst(MOV64XY, 0, 3),
                 inst(EXIT),
                 inst(ADD64XC, 4, 1),
                 inst(EXIT),
                };
  inst expected_prog2_2[] = {inst(MOV64XC, 0, 3),
                             inst(MOV64XC, 2, 1),
                             inst(JEQXY, 1, 2, 3),
                             inst(ADD64XC, 3, 1),
                             inst(MOV64XY, 0, 3),
                             inst(EXIT),
                             inst(EXIT),
                             inst(),
                            };
  canonicalize_check(p2_2, sizeof(p2_2) / sizeof(inst), expected_prog2_2, "2.2");
}

void remove_nops_check(inst* prog, int len, inst* expected_prog, string test_name) {
  remove_nops(prog, len);
  bool is_equal = true;
  for (int i = 0; i < len; i++) {
    is_equal &= (prog[i] == expected_prog[i]);
  }
  print_test_res(is_equal, test_name);
}

void test2() {
  cout << "Test 1: program remove nops test" << endl;
  inst p1[] = {inst(JA, 1),
               inst(NOP),
               inst(EXIT),
              };
  inst p1_expected[] = {inst(JA, 0),
                        inst(EXIT),
                        inst(NOP),
                       };
  remove_nops_check(p1, sizeof(p1) / sizeof(inst), p1_expected, "1");

  inst p2[] = {inst(JA, 1),
               inst(MOV64XC, 1, 0),
               inst(NOP),
               inst(EXIT),
               inst(JA, 2),
               inst(NOP),
               inst(NOP),
               inst(EXIT),
              };
  inst p2_expected[] = {inst(JA, 1),
                        inst(MOV64XC, 1, 0),
                        inst(EXIT),
                        inst(JA, 0),
                        inst(EXIT),
                        inst(NOP),
                        inst(NOP),
                        inst(NOP),
                       };
  remove_nops_check(p2, sizeof(p2) / sizeof(inst), p2_expected, "2");

  inst p3[] = {inst(MOV64XC, 1, 0),
               inst(JEQXC, 1, 2, 3),
               inst(NOP),
               inst(MOV64XC, 1, 0),
               inst(NOP),
               inst(JA, 1),
               inst(NOP),
               inst(EXIT),
               inst(NOP),
               inst(JA, -4),
              };
  inst p3_expected[] = {inst(MOV64XC, 1, 0),
                        inst(JEQXC, 1, 2, 1),
                        inst(MOV64XC, 1, 0),
                        inst(JA, 0),
                        inst(EXIT),
                        inst(JA, -2),
                        inst(),
                        inst(),
                        inst(),
                        inst(),
                       };
  remove_nops_check(p3, sizeof(p3) / sizeof(inst), p3_expected, "3");

  inst p4[] = {inst(JA, 3),
               INSN_LDMAPID(1, 0),
               inst(),
               inst(),
               inst(EXIT),
              };
  inst p4_expected[] = {inst(JA, 2),
                        INSN_LDMAPID(1, 0),
                        inst(),
                        inst(EXIT),
                        inst(),
                       };
  remove_nops_check(p4, sizeof(p4) / sizeof(inst), p4_expected, "4");

  inst p5[] = {inst(),
               inst(MOV64XC, 0, 1),
               inst(EXIT),
              };
  inst p5_expected[] = {inst(MOV64XC, 0, 1),
                        inst(EXIT),
                        inst(),
                       };
  remove_nops_check(p5, sizeof(p5) / sizeof(inst), p5_expected, "5");
}

bool reg_state_is_equal(vector<register_state>& x, vector<register_state>& y) {
  if (x.size() != y.size()) return false;
  bool is_equal = true;
  // x[i] can be found in y
  for (int i = 0; i < x.size(); i++) {
    bool is_equal_i = false;
    for (int j = 0; j < y.size(); j++) {
      is_equal_i = is_equal_i || (x[i] == y[j]);
    }
    is_equal = is_equal && is_equal_i;
  }
  return is_equal;
}

bool live_var_is_equal(live_variables& x, live_variables& y) {
  if (x.regs.size() != y.regs.size()) return false;
  for (auto reg : x.regs) {
    // `reg` can be found in y.regs
    if (y.regs.find(reg) == y.regs.end()) return false;
  }

  if (x.mem.size() != y.mem.size()) return false;
  for (auto it1 = x.mem.begin(); it1 != x.mem.end(); it1++) {
    int type = it1->first;
    auto it2 = y.mem.find(type);
    if (it2 == y.mem.end()) return false;
    for (auto off : it1->second) {
      if (it2->second.find(off) == it2->second.end()) {
        return false;
      }
    }
  }
  return true;
}

void test3() {
  cout << "Test 3: program static analysis" << endl;
  cout << "3.1 test register type inference" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(16);
  mem_t::add_map(map_attr(16, 32, 16)); // key_sz = 2 bytes, val_sz = 4 bytes
  mem_t::add_map(map_attr(16, 32, 16));

  inst p1[] = {inst(MOV64XY, 2, 1),
               inst(ADD64XC, 2, 2),
               inst(LDXB, 0, 2, 2),
               inst(EXIT),
              };
  prog_static_state pss;
  static_analysis(pss, p1, sizeof(p1) / sizeof(inst));
  // check r2 state before executing insn 2
  vector<register_state> expected_insn2_r2_p1;
  expected_insn2_r2_p1.push_back(register_state{PTR_TO_CTX, 2, 0});
  print_test_res(reg_state_is_equal(expected_insn2_r2_p1, pss.static_state[2].reg_state[2]), "1");

  inst p2[] = {inst(MOV64XY, 2, 1),
               inst(MOV64XC, 3, 5),
               inst(JEQXY, 3, 3, 1),
               inst(ADD64XC, 2, 2),
               inst(LDXB, 0, 2, 2),
               inst(EXIT),
              };
  static_analysis(pss, p2, sizeof(p2) / sizeof(inst));
  // check r2 state before executing insn 3
  vector<register_state> expected_insn4_r2_p2;
  expected_insn4_r2_p2.push_back(register_state{PTR_TO_CTX, 0, 0});
  expected_insn4_r2_p2.push_back(register_state{PTR_TO_CTX, 2, 0});
  print_test_res(reg_state_is_equal(expected_insn4_r2_p2, pss.static_state[4].reg_state[2]), "2");

  // test the return value of map lookup
  inst p3[] = {inst(STH, 10, -2, 0xff),
               INSN_LDMAPID(1, 1),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -2),
               inst(CALL, BPF_FUNC_map_lookup_elem),
               inst(EXIT),
              };
  static_analysis(pss, p3, sizeof(p3) / sizeof(inst));
  // check r0 state before executing insn 5
  vector<register_state> expected_insn5_r0_p3;
  register_state rs_p3;
  rs_p3.type = PTR_TO_MAP_VALUE_OR_NULL;
  rs_p3.map_id = 1;
  rs_p3.off = 0;
  expected_insn5_r0_p3.push_back(rs_p3);
  print_test_res(reg_state_is_equal(expected_insn5_r0_p3, pss.static_state[5].reg_state[0]), "3");

  inst p4[] = {inst(STH, 10, -2, 0xff),
               inst(MOV64XC, 1, 5),
               INSN_LDMAPID(1, 1),
               inst(JEQXY, 1, 1, 1),
               INSN_LDMAPID(1, 0),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -2),
               inst(CALL, BPF_FUNC_map_lookup_elem),
               inst(EXIT),
              };
  static_analysis(pss, p4, sizeof(p4) / sizeof(inst));
  // check r0 state before executing insn 8
  vector<register_state> expected_insn8_r0_p4;
  register_state rs_p4;
  rs_p4.type = PTR_TO_MAP_VALUE_OR_NULL;
  rs_p4.map_id = 1;
  rs_p4.off = 0;
  expected_insn8_r0_p4.push_back(rs_p4);
  rs_p4.map_id = 0;
  expected_insn8_r0_p4.push_back(rs_p4);
  print_test_res(reg_state_is_equal(expected_insn8_r0_p4, pss.static_state[8].reg_state[0]), "4");

  inst p5[] = {inst(STH, 10, -2, 0xff),
               INSN_LDMAPID(1, 1),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -2),
               inst(CALL, BPF_FUNC_map_lookup_elem),
               inst(JEQXC, 0, 0, 2),
               inst(LDXB, 0, 0, 0), // insn 6
               inst(EXIT),
               inst(MOV64XC, 0, 2), // insn 8
               inst(EXIT),
              };
  static_analysis(pss, p5, sizeof(p5) / sizeof(inst));
  // check r0 state before executing insn 6 and 8
  vector<register_state> expected_insn6_r0_p5;
  vector<register_state> expected_insn8_r0_p5;
  register_state rs_p5_1;
  rs_p5_1.type = PTR_TO_MAP_VALUE;
  rs_p5_1.map_id = 1;
  rs_p5_1.off = 0;
  expected_insn6_r0_p5.push_back(rs_p5_1);
  register_state rs_p5_2;
  rs_p5_2.type = SCALAR_VALUE;
  rs_p5_2.val_flag = true;
  rs_p5_2.val = 0;
  expected_insn8_r0_p5.push_back(rs_p5_2);
  print_test_res(reg_state_is_equal(expected_insn6_r0_p5, pss.static_state[6].reg_state[0]), "5.1");
  print_test_res(reg_state_is_equal(expected_insn8_r0_p5, pss.static_state[8].reg_state[0]), "5.2");

  inst p6[] = {inst(STH, 10, -2, 0xff),
               INSN_LDMAPID(1, 1),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -2),
               inst(CALL, BPF_FUNC_map_lookup_elem),
               inst(MOV64XY, 1, 0),
               inst(),
               inst(JEQXC, 1, 0, 3),
               inst(ADD64XC, 1, 1), // insn 8
               inst(LDXB, 0, 1, 0), // insn 9
               inst(EXIT),
               inst(MOV64XC, 0, 2),
               inst(EXIT),
              };
  static_analysis(pss, p6, sizeof(p6) / sizeof(inst));
  // check r1 state before executing insn 8 and 9
  vector<register_state> expected_insn8_r1_p6;
  vector<register_state> expected_insn9_r1_p6;
  register_state rs_p6;
  rs_p6.type = PTR_TO_MAP_VALUE;
  rs_p6.map_id = 1;
  rs_p6.off = 0;
  expected_insn8_r1_p6.push_back(rs_p6);
  rs_p6.off = 1;
  expected_insn9_r1_p6.push_back(rs_p6);
  print_test_res(reg_state_is_equal(expected_insn8_r1_p6, pss.static_state[8].reg_state[1]), "6.1");
  print_test_res(reg_state_is_equal(expected_insn9_r1_p6, pss.static_state[9].reg_state[1]), "6.2");

  // test r1-r5 are set unreadable after function call
  inst p7[] = {inst(MOV64XC, 1, 0),
               inst(MOV64XC, 2, 0),
               inst(MOV64XC, 3, 0),
               inst(MOV64XC, 4, 0),
               inst(MOV64XC, 5, 0),
               inst(CALL, BPF_FUNC_get_prandom_u32), // insn 5
               inst(EXIT), // insn 6
              };
  static_analysis(pss, p7, sizeof(p7) / sizeof(inst));
  bool p7_res = true;
  // check r1-r5 state before executing insn 5 and 6
  for (int r = 1; r <= 5; r++) {
    p7_res &= (pss.static_state[5].reg_state[r].size() == 1);
  }
  print_test_res(p7_res, "7.1");
  p7_res = true;
  for (int r = 1; r <= 5; r++) {
    p7_res &= (pss.static_state[6].reg_state[r].size() == 0);
  }
  print_test_res(p7_res, "7.2");

  cout << "3.2 test live analysis" << endl;
  inst p2_1[] = {inst(),
                 inst(STH, 10, -8, 0xff),
                 INSN_LDMAPID(1, 0),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -8),
                 inst(CALL, BPF_FUNC_map_lookup_elem),
                 inst(EXIT),
                };
  static_analysis(pss, p2_1, sizeof(p2_1) / sizeof(inst));
  // check live variables after executing insn 4
  live_variables expected_insn4_p21;
  expected_insn4_p21.regs = {1, 2};
  expected_insn4_p21.mem[PTR_TO_STACK] = {STACK_SIZE - 8, STACK_SIZE - 7};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn4_p21.mem[PTR_TO_CTX].insert(i);
  }
  print_test_res(live_var_is_equal(expected_insn4_p21, pss.static_state[4].live_var), "1.1");
  // check live variables after executing insn 0
  live_variables expected_insn0_p21;
  expected_insn0_p21.regs = {10};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn0_p21.mem[PTR_TO_CTX].insert(i);
  }
  print_test_res(live_var_is_equal(expected_insn0_p21, pss.static_state[0].live_var), "1.2");

  inst p2_2[] = {inst(MOV64XC, 3, 5),
                 inst(JEQXY, 3, 3, 2),
                 inst(MOV64XY, 1, 5),
                 inst(LDXB, 2, 10, -1),
                 inst(MOV64XY, 1, 6),
                 inst(LDXB, 2, 10, -2),
                 inst(EXIT),
                };
  static_analysis(pss, p2_2, sizeof(p2_2) / sizeof(inst));
  // check live variables after executing insn 0
  live_variables expected_insn1_p22;
  expected_insn1_p22.regs = {0, 5, 6, 10};
  expected_insn1_p22.mem[PTR_TO_STACK] = {STACK_SIZE - 1, STACK_SIZE - 2};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn1_p22.mem[PTR_TO_CTX].insert(i);
  }
  print_test_res(live_var_is_equal(expected_insn1_p22, pss.static_state[1].live_var), "2");

  // test output pkt
  inst p2_3[] = {inst(STB, 1, 2, 0xff),
                 inst(STB, 1, 1, 0xff),
                };
  static_analysis(pss, p2_3, sizeof(p2_3) / sizeof(inst));
  // check live variables after executing insn 0
  live_variables expected_insn0_p23;
  expected_insn0_p23.regs = {0, 1};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn0_p23.mem[PTR_TO_CTX].insert(i);
  }
  expected_insn0_p23.mem[PTR_TO_CTX].erase(1);
  print_test_res(live_var_is_equal(expected_insn0_p23, pss.static_state[0].live_var), "3");

  // test nop
  inst p2_4[] = {inst(NOP),
                 inst(NOP),
                 inst(EXIT),// r0 is the output
                };
  static_analysis(pss, p2_4, sizeof(p2_4) / sizeof(inst));
  // live variable after executing insn 0
  live_variables expected_insn0_p24;
  expected_insn0_p24.regs = {0};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn0_p24.mem[PTR_TO_CTX].insert(i);
  }
  print_test_res(live_var_is_equal(expected_insn0_p24, pss.static_state[0].live_var), "4");

  inst p2_5[] = {INSN_LDMAPID(2, 1),
                 inst(MOV64XC, 3, 0),
                 inst(CALL, BPF_FUNC_tail_call),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  static_analysis(pss, p2_5, sizeof(p2_5) / sizeof(inst));
  // live variable after executing insn 0
  live_variables expected_insn0_p25, expected_insn2_p25;
  expected_insn0_p25.regs = {1, 2};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn0_p25.mem[PTR_TO_CTX].insert(i);
  }
  expected_insn2_p25.regs = {1, 2, 3};
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    expected_insn2_p25.mem[PTR_TO_CTX].insert(i);
  }
  print_test_res(live_var_is_equal(expected_insn0_p25, pss.static_state[0].live_var), "5.1");
  print_test_res(live_var_is_equal(expected_insn2_p25, pss.static_state[2].live_var), "5.2");

  cout << "3.3 test register constant inference" << endl;
  inst p3_1[] = {inst(MOV64XC, 1, 5),
                 inst(MOV64XY, 2, 1),
                 inst(EXIT),
                };
  static_analysis(pss, p3_1, sizeof(p3_1) / sizeof(inst));
  vector<register_state> expected_insn2_r1_p31;
  expected_insn2_r1_p31.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  print_test_res(reg_state_is_equal(expected_insn2_r1_p31, pss.static_state[2].reg_state[1]), "1.1");
  vector<register_state> expected_insn2_r2_p31;
  expected_insn2_r2_p31.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  print_test_res(reg_state_is_equal(expected_insn2_r2_p31, pss.static_state[2].reg_state[2]), "1.2");

  inst p3_2[] = {inst(MOV64XC, 1, 5),
                 inst(JA, 1),
                 inst(ADD64XC, 1, 4),
                 inst(MOV64XY, 2, 1),
                 inst(EXIT),
                };
  static_analysis(pss, p3_2, sizeof(p3_2) / sizeof(inst));
  vector<register_state> expected_insn4_r1_p32;
  expected_insn4_r1_p32.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  print_test_res(reg_state_is_equal(expected_insn4_r1_p32, pss.static_state[4].reg_state[1]), "2.1");
  vector<register_state> expected_insn4_r2_p32;
  expected_insn4_r2_p32.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  print_test_res(reg_state_is_equal(expected_insn4_r2_p32, pss.static_state[4].reg_state[2]), "2.2");

  inst p3_3[] = {inst(MOV64XC, 1, 5),
                 inst(JEQXC, 1, 7, 1),
                 inst(ADD64XC, 1, 4),
                 inst(MOV64XY, 2, 1),
                 inst(EXIT),
                };
  static_analysis(pss, p3_3, sizeof(p3_3) / sizeof(inst));
  vector<register_state> expected_insn4_r1_p33;
  expected_insn4_r1_p33.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  expected_insn4_r1_p33.push_back(register_state{SCALAR_VALUE, 0, 9, true});
  print_test_res(reg_state_is_equal(expected_insn4_r1_p33, pss.static_state[4].reg_state[1]), "3.1");
  vector<register_state> expected_insn4_r2_p33;
  expected_insn4_r2_p33.push_back(register_state{SCALAR_VALUE, 0, 5, true});
  expected_insn4_r2_p33.push_back(register_state{SCALAR_VALUE, 0, 9, true});
  print_test_res(reg_state_is_equal(expected_insn4_r2_p33, pss.static_state[4].reg_state[2]), "3.2");

  inst p3_4[] = {INSN_MOVDWXC(0, 0x1234567890),
                 inst(EXIT),
                };
  convert_bpf_pgm_to_superopt_pgm(p3_4, sizeof(p3_4) / sizeof(inst));
  static_analysis(pss, p3_4, sizeof(p3_4) / sizeof(inst));
  vector<register_state> expected_insn1_r0_p34;
  expected_insn1_r0_p34.push_back(register_state{SCALAR_VALUE, 0, 0x1234567890, true});
  print_test_res(reg_state_is_equal(expected_insn1_r0_p34, pss.static_state[1].reg_state[0]), "4.1");
}

// expected_safe is either true for safe or false for unsafe
void test_safety_check(inst* program, int len, bool expected_safe, string test_name) {
  bool is_succ = false;
  try {
    static_safety_check_pgm(program, len);
    if (expected_safe) is_succ = true;
  } catch (string err_msg) {
    if (! expected_safe) is_succ = true;
  }
  print_test_res(is_succ, test_name);
}

void test_safety_check_win(inst* program, int len,
                           int win_start, int win_end,
                           bool expected_safe, string test_name) {
  prog_static_state pss;
  static_analysis(pss, program, len);

  bool is_succ = false;
  try {
    static_safety_check_win(program, win_start, win_end, pss);
    if (expected_safe) is_succ = true;
  } catch (string err_msg) {
    if (! expected_safe) is_succ = true;
  }
  print_test_res(is_succ, test_name);
}

void test4() {
  cout << "Test 4: program static safety check" << endl;
  cout << "4.1 illegal pointer operations" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  inst p1_1[] = {inst(JEQXC, 1, 0, 0),
                 inst(EXIT),
                };
  test_safety_check(p1_1, sizeof(p1_1) / sizeof(inst), false, "1");
  test_safety_check_win(p1_1, sizeof(p1_1) / sizeof(inst), 0, 0, false, "1.1");

  inst p1_2[] = {inst(AND64XC, 1),
                 inst(EXIT),
                };
  test_safety_check(p1_2, sizeof(p1_2) / sizeof(inst), false, "2");
  test_safety_check_win(p1_2, sizeof(p1_2) / sizeof(inst), 0, 0, false, "2.1");

  inst p1_3[] = {inst(STB, 10, -1, 0x1),
                 INSN_LDMAPID(1, 0),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -1),
                 inst(CALL, BPF_FUNC_map_lookup_elem),
                 inst(JEQXC, 0, 0, 2),
                 inst(LDXB, 0, 0, 0),
                 inst(EXIT),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  test_safety_check(p1_3, sizeof(p1_3) / sizeof(inst), true, "3");
  test_safety_check_win(p1_3, sizeof(p1_3) / sizeof(inst), 6, 6, true, "3.1");
  test_safety_check_win(p1_3, sizeof(p1_3) / sizeof(inst), 8, 8, true, "3.2");
  test_safety_check_win(p1_3, sizeof(p1_3) / sizeof(inst), 5, 5, true, "3.3");
}

void test5() {
  // test PGM_INPUT_pkt_ptrs related
  cout << "Test 5: program static analysis of PGM_INPUT_pkt_ptrs" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(32);
  cout << "5.1 test register type inference" << endl;
  inst p1[] = {inst(LDXW, 2, 1, 0),
               inst(EXIT),
              };
  prog_static_state pss;
  static_analysis(pss, p1, sizeof(p1) / sizeof(inst));
  // check r2 state before executing insn 1
  vector<register_state> expected_insn1_r2_p1;
  expected_insn1_r2_p1.push_back(register_state{PTR_TO_PKT, 0});
  print_test_res(reg_state_is_equal(expected_insn1_r2_p1, pss.static_state[1].reg_state[2]), "1");

  inst p2[] = {inst(LDXW, 2, 1, 0),
               inst(MOV64XY, 3, 2),
               inst(ADD64XC, 3, 1), // r3: PTR_TO_PKT with offset 1
               inst(EXIT),
              };
  static_analysis(pss, p2, sizeof(p2) / sizeof(inst));
  vector<register_state> expected_insn3_r3_p2;
  expected_insn3_r3_p2.push_back(register_state{PTR_TO_PKT, 1});
  print_test_res(reg_state_is_equal(expected_insn3_r3_p2, pss.static_state[3].reg_state[3]), "2");

  inst p3[] = {inst(LDXW, 2, 1, 2),
               inst(LDXW, 3, 2, 0),
               inst(EXIT),
              };
  static_analysis(pss, p3, sizeof(p3) / sizeof(inst));
  vector<register_state> expected_insn1_r2_p3;
  expected_insn1_r2_p3.push_back(register_state{SCALAR_VALUE});
  print_test_res(reg_state_is_equal(expected_insn1_r2_p3, pss.static_state[1].reg_state[2]), "3.1");
  vector<register_state> expected_insn2_r3_p3;
  expected_insn2_r3_p3.push_back(register_state{SCALAR_VALUE});
  print_test_res(reg_state_is_equal(expected_insn2_r3_p3, pss.static_state[2].reg_state[3]), "3.2");

  // p4: before executing EXIT, reg 0 does not have states, since one incoming branch does not have r0 state
  inst p4[] = {inst(LDXW, 1, 1, 0),
               inst(JA, 1),
               inst(MOV64XC, 0, 1),
               inst(EXIT),
              };
  static_analysis(pss, p4, sizeof(p4) / sizeof(inst));
  vector<register_state> expected_insn3_r0_p4;
  print_test_res(reg_state_is_equal(expected_insn3_r0_p4, pss.static_state[3].reg_state[0]), "4");

  // p5: check pkt_start and pkt_end
  inst p5[] = {inst(LDXW, 2, 1, 4),
               inst(LDXW, 7, 1, 0),
               inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  static_analysis(pss, p5, sizeof(p5) / sizeof(inst));
  // check r2 state before executing insn 1
  vector<register_state> expected_insn1_r2_p5;
  expected_insn1_r2_p5.push_back(register_state{PTR_TO_PACKET_END, 0});
  print_test_res(reg_state_is_equal(expected_insn1_r2_p5, pss.static_state[1].reg_state[2]), "5.1");
  // check r2, r7 state before executing insn 2
  vector<register_state> expected_insn2_r2_p5, expected_insn2_r7_p5;
  expected_insn2_r2_p5.push_back(register_state{PTR_TO_PACKET_END, 0});
  print_test_res(reg_state_is_equal(expected_insn2_r2_p5, pss.static_state[2].reg_state[2]), "5.2");
  expected_insn2_r7_p5.push_back(register_state{PTR_TO_PKT, 0});
  print_test_res(reg_state_is_equal(expected_insn2_r7_p5, pss.static_state[2].reg_state[7]), "5.3");

  cout << "5.2 safety check" << endl;
  test_safety_check(p1, sizeof(p1) / sizeof(inst), true, "1");
  test_safety_check(p2, sizeof(p2) / sizeof(inst), true, "2");

  inst p2_1[] = {inst(LDXW, 2, 1, 0),
                 inst(LDXW, 3, 2, 0),
                 inst(EXIT),
                };
  test_safety_check(p2_1, sizeof(p2_1) / sizeof(inst), false, "3");

  inst p2_2[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 2), // if r3 > r2, exit;
                 inst(STW, 7, 0, 1),   // 5: pkt[0:3] safe
                 inst(STB, 7, 4, 1),   // 6: unsafe
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  test_safety_check(p2_2, sizeof(p2_2) / sizeof(inst), false, "p2.2 1");
  test_safety_check_win(p2_2, sizeof(p2_2) / sizeof(inst), 5, 5, true, "p2.2 2");
  test_safety_check_win(p2_2, sizeof(p2_2) / sizeof(inst), 6, 6, false, "p2.2 3");

  inst p2_3[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 5), // if r3 > r2, exit;
                 inst(STB, 7, 0, 1),   // 5: sz = 4
                 inst(MOV64XY, 3, 7),  // 6: sz = 4
                 inst(ADD64XC, 3, 6),  // r3 = r7 + 6
                 inst(JGTXY, 3, 2, 1), // 8: sz = 4
                 inst(STB, 7, 6, 1),   // 9: sz = 6, unsafe accessing pkt[6]
                 inst(MOV64XC, 0, 0),  // 10: sz = min(0, 4, 6) = 0
                 inst(EXIT),
                };
  test_safety_check(p2_3, sizeof(p2_3) / sizeof(inst), false, "p2.3 1");
  test_safety_check_win(p2_3, sizeof(p2_3) / sizeof(inst), 5, 5, true, "p2.3 2");
  test_safety_check_win(p2_3, sizeof(p2_3) / sizeof(inst), 9, 9, false, "p2.3 3");

  cout << "5.3 liveness analysis" << endl;
  inst p3_1[] = {inst(LDXW, 2, 1, 0),
                 inst(MOV64XY, 0, 2),
                 inst(EXIT),
                };
  static_analysis(pss, p3_1, sizeof(p3_1) / sizeof(inst));
  live_variables liv_expected_insn0_p31;
  // check live variables after executing insn 0
  liv_expected_insn0_p31.regs = {2};
  for (int i = 0; i < 8; i++) { // 8: two 32-bit pointers
    liv_expected_insn0_p31.mem[PTR_TO_CTX].insert(i);
  }
  for (int i = 0; i < mem_t::_layout._pkt_sz; i++) {
    liv_expected_insn0_p31.mem[PTR_TO_PKT].insert(i);
  }
  print_test_res(live_var_is_equal(liv_expected_insn0_p31, pss.static_state[0].live_var), "1");

  cout << "5.4 min_pkt_sz inference" << endl;
  inst p4_1[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 2), // if r3 > r2, exit; else pkt[0:1] = 1;
                 inst(STB, 7, 0, 1),  // 5: sz = 4
                 inst(STB, 7, 1, 1),  // 6: sz = 4
                 inst(MOV64XC, 0, 0), // 7: sz = min(0, 4)
                 inst(EXIT),
                };
  const int len_p4_1 = sizeof(p4_1) / sizeof(inst);
  static_analysis(pss, p4_1, len_p4_1);
  vector<unsigned int> min_pkt_sz_expected(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  min_pkt_sz_expected[5] = 4;
  min_pkt_sz_expected[6] = 4;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "1");

  // check branches
  inst p4_2[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 5), // if r3 > r2, exit;
                 inst(STB, 7, 0, 1),   // 5: sz = 4
                 inst(MOV64XY, 3, 7),  // 6: sz = 4
                 inst(ADD64XC, 3, 6),  // r3 = r7 + 6
                 inst(JGTXY, 3, 2, 1), // 8: sz = 4
                 inst(STB, 7, 5, 1),   // 9: sz = 6
                 inst(MOV64XC, 0, 0),  // 10: sz = min(0, 4, 6) = 0
                 inst(EXIT),
                };
  const int len_p4_2 = sizeof(p4_2) / sizeof(inst);
  static_analysis(pss, p4_2, len_p4_2);
  min_pkt_sz_expected.resize(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  for (int i = 5; i <= 8; i++) min_pkt_sz_expected[i] = 4;
  min_pkt_sz_expected[9] = 6;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "2");

  inst p4_3[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 6), // 4: if r3 > r2, exit;
                 inst(STB, 7, 0, 1),   // 5: sz = 4
                 inst(MOV64XY, 3, 7),  // 6: sz = 4
                 inst(ADD64XC, 3, 6),  // r3 = r7 + 6
                 inst(JGTXY, 3, 2, 1), // 8: sz = 4
                 inst(STB, 7, 5, 1),   // 9: sz = 6
                 inst(STB, 7, 3, 1),   // 10: sz = min(4, 6) = 4
                 inst(MOV64XC, 0, 0),  // 11: sz = min(min(4, 6), 0) = 0
                 inst(EXIT),
                };
  const int len_p4_3 = sizeof(p4_3) / sizeof(inst);
  static_analysis(pss, p4_3, len_p4_3);
  min_pkt_sz_expected.resize(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  for (int i = 5; i <= 8; i++) min_pkt_sz_expected[i] = 4;
  min_pkt_sz_expected[9] = 6;
  min_pkt_sz_expected[10] = 4;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "3");

  // check jmp 0
  inst p4_4[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 0), // if r3 > r2, jmp 0
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  const int len_p4_4 = sizeof(p4_4) / sizeof(inst);
  static_analysis(pss, p4_4, len_p4_4);
  min_pkt_sz_expected.resize(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "4");

  // check other block_e insns that won't update min_pkt_sz
  inst p4_5[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXC, 3, 2, 2), // if r3 > 2, jmp 1
                 inst(MOV64XC, 0, 1),
                 inst(JA, 1),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  const int len_p4_5 = sizeof(p4_5) / sizeof(inst);
  static_analysis(pss, p4_5, len_p4_5);
  min_pkt_sz_expected.resize(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "5");

  // check other block_e insns that won't update min_pkt_sz
  inst p4_6[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(JGTXY, 3, 2, 6), // 4: if r3 > r2, exit;
                 inst(STB, 7, 0, 1),   // 5: sz = 4
                 inst(MOV64XY, 3, 7),  // 6: sz = 4
                 inst(ADD64XC, 3, 6),  // r3 = r7 + 6
                 inst(JGTXC, 3, 0xff, 1), // 8: sz = 4
                 inst(JA, 1),          // 9: sz = 4
                 inst(STB, 7, 3, 1),   // 10: sz = min(4, 4) = 4
                 inst(MOV64XC, 0, 0),  // 11: sz = min(4, 0) = 0
                 inst(EXIT),
                };
  const int len_p4_6 = sizeof(p4_6) / sizeof(inst);
  static_analysis(pss, p4_6, len_p4_6);
  min_pkt_sz_expected.resize(pss.static_state.size());
  for (int i = 0; i < min_pkt_sz_expected.size(); i++) min_pkt_sz_expected[i] = 0;
  for (int i = 5; i <= 10; i++) min_pkt_sz_expected[i] = 4;
  print_test_res(check_min_pkt_sz(min_pkt_sz_expected, pss.static_state), "6");

  mem_t::_layout.clear();
}

void test6() {
  cout << "Test 6: test gen_random_input_for_win" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(16);
  mem_t::add_map(map_attr(16, 32, 16)); // key_sz = 2 bytes, val_sz = 4 bytes
  mem_t::add_map(map_attr(16, 32, 16));

  inst p1[] = {inst(STB, 10, -1, 0x1),
               INSN_LDMAPID(1, 0),
               inst(MOV64XY, 2, 10),
               inst(ADD64XC, 2, -1),
               inst(CALL, BPF_FUNC_map_lookup_elem),
               inst(JEQXC, 0, 0, 2),
               inst(LDXB, 0, 0, 0), // insn 6
               inst(EXIT),
               inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  int win_start = 6, win_end = 6;
  inout_t::start_insn = win_start;
  inout_t::end_insn = win_end;
  smt_var::is_win = true;
  prog_static_state pss;
  static_analysis(pss, p1, sizeof(p1) / sizeof(inst));
  set_up_smt_inout_orig(pss, p1, sizeof(p1) / sizeof(inst), win_start, win_end);
  int n_inputs = 1;
  vector<inout_t> inputs;
  gen_random_input_for_win(inputs, n_inputs, pss.static_state[win_start], p1[win_start], win_start, win_end);
  inout_t output;
  prog_state ps;
  ps.init();
  interpret(output, p1, sizeof(p1) / sizeof(inst), ps, inputs[0]);
  // check r0
  int64_t r0_expected = output.maps_mem[0][0];
  auto it = output.regs.find(0);
  if (it != output.regs.end()) {
    int64_t r0_actual = it->second;
    print_test_res(r0_actual == r0_expected, "1");
  } else {
    print_test_res(false, "1");
  }

}

void test7() {
  cout << "Test 7: test set nops as JA 0 instructions" << endl;
  inst p1[] = {inst(),
               inst(MOV64XC, 0, 1),
               inst(EXIT),
              };
  inst p1_exp[] = {inst(JA, 0),
                   inst(MOV64XC, 0, 1),
                   inst(EXIT),
                  };
  set_nops_as_JA0(p1, 3);
  print_test_res(are_same_progs(p1, 3, p1_exp, 3), "1");

  inst p2[] = {INSN_LDMAPID(1, 0),
               inst(),
               inst(),
               inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  inst p2_exp[] = {INSN_LDMAPID(1, 0),
                   inst(),
                   inst(JA, 0),
                   inst(MOV64XC, 0, 0),
                   inst(EXIT),
                  };
  set_nops_as_JA0(p2, 5);
  print_test_res(are_same_progs(p2, 5, p2_exp, 5), "2");

  inst p3[] = {INSN_MOVDWXC(0, 1),
               inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  set_nops_as_JA0(p3, 4);
  print_test_res(are_same_progs(p3, 4, p3, 4), "3");
}

int main(int argc, char *argv[]) {
  try {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
  } catch (string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}