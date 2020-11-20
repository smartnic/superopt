#include <iostream>
#include "../../../src/utils.h"
#include "canonicalize.h"

void canonicalize_check(inst* prog, int len, inst* expected_prog, string test_name) {
  canonicalize(prog, len);
  bool is_equal = true;
  for (int i = 0; i < len; i++) {
    is_equal = (prog[i] == expected_prog[i]);
  }
  print_test_res(is_equal, test_name);
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
               inst(LDMAPID, 1, 0), // r1 = map_id (0)
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
                           inst(LDMAPID, 1, 0), // r1 = map_id (0)
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
                             inst(ADD64XC, 1, 1),
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
    is_equal = (prog[i] == expected_prog[i]);
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
  inst p2_expected[] = {inst(JA, 0),
                        inst(MOV64XC, 1, 0),
                        inst(EXIT),
                        inst(NOP),
                        inst(JA, 0),
                        inst(EXIT),
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
                        inst(JA, 1),
                        inst(EXIT),
                        inst(JA, -2),
                        inst(),
                        inst(),
                        inst(),
                        inst(),
                       };
  remove_nops_check(p3, sizeof(p3) / sizeof(inst), p3_expected, "3");

  inst p4[] = {inst(LDMAPID, 1, 0),
               inst(),
               inst(EXIT),
              };
  inst p4_expected[] = {inst(LDMAPID, 1, 0),
                        inst(),
                        inst(EXIT),
                       };
  remove_nops_check(p4, sizeof(p4) / sizeof(inst), p4_expected, "4");
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

  cout << "3.2 test live analysis" << endl;
  inst p2_1[] = {inst(),
                 inst(STH, 10, -8, 0xff),
                 inst(LDMAPID, 1, 0),
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

void test4() {
  cout << "Test 4: program static safety check" << endl;
  cout << "4.1 illegal pointer operations" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::add_map(map_attr(8, 8, 32)); // k_sz: 8 bits; v_sz: 8 bits; max_entirs: 32
  inst p1_1[] = {inst(JEQXC, 1, 0, 0),
                 inst(EXIT),
                };
  test_safety_check(p1_1, sizeof(p1_1) / sizeof(inst), false, "1");

  inst p1_2[] = {inst(AND64XC, 1),
                 inst(EXIT),
                };
  test_safety_check(p1_2, sizeof(p1_2) / sizeof(inst), false, "2");

  inst p1_3[] = {inst(STB, 10, -1, 0x1),
                 inst(LDMAPID, 1, 0),
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

  cout << "5.2 safety check" << endl;
  test_safety_check(p1, sizeof(p1) / sizeof(inst), true, "1");
  test_safety_check(p2, sizeof(p2) / sizeof(inst), true, "2");

  inst p2_1[] = {inst(LDXW, 2, 1, 0),
                 inst(LDXW, 3, 2, 0),
                 inst(EXIT),
                };
  test_safety_check(p2_1, sizeof(p2_1) / sizeof(inst), true, "3");

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
  mem_t::_layout.clear();
}

int main(int argc, char *argv[]) {
  try {
    test1();
    test2();
    test3();
    test4();
    test5();
  } catch (string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}