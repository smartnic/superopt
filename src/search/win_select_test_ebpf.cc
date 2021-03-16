#include "../../src/isa/ebpf/inst.h"
#include "win_select.h"

bool are_wins_equal(vector<pair<int, int>>& wins,
                    vector<int>& win_s_expected,
                    vector<int>& win_e_expected) {
  bool are_equal = true;
  if (wins.size() == win_s_expected.size()) {
    for (int i = 0; i < wins.size(); i++) {
      bool found = false;
      for (int j = 0; j < win_s_expected.size(); j++) {
        if ((wins[i].first == win_s_expected[j]) &&
            (wins[i].second == win_e_expected[j])) {
          found = true;
          break;
        }
      }
      if (! found) {
        are_equal = false;
        break;
      }
    }
  } else {
    are_equal = false;
  }
  return are_equal;
}

void check_gen_wins(inst* program, int len,
                    vector<int>& win_s_expected,
                    vector<int>& win_e_expected,
                    string test_name) {
  assert(win_s_expected.size() == win_e_expected.size());
  prog_static_state pss;
  static_analysis(pss, program, len);
  vector<pair<int, int>> wins;
  gen_wins(wins, program, len, pss);
  bool eq = are_wins_equal(wins, win_s_expected, win_e_expected);
  print_test_res(eq, test_name);
}

void check_opt_wins(inst* program, int len,
                    vector<int>& win_s_expected,
                    vector<int>& win_e_expected,
                    string test_name) {
  assert(win_s_expected.size() == win_e_expected.size());
  prog_static_state pss;
  static_analysis(pss, program, len);
  vector<pair<int, int>> wins;
  gen_wins(wins, program, len, pss);
  optimize_wins(wins);
  bool eq = are_wins_equal(wins, win_s_expected, win_e_expected);
  print_test_res(eq, test_name);
}

void test1() {
  cout << "test 1" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(16, 32, 16));

  vector<int> win_s_expected, win_e_expected;
  inst p1[] = {inst(MOV64XC, 1, 0),
               inst(MOV64XC, 2, 2),
               inst(MOV64XC, 3, 0),
               inst(EXIT),
              };

  win_s_expected = {0};
  win_e_expected = {2};
  check_gen_wins(p1, sizeof(p1) / sizeof(inst), win_s_expected, win_e_expected, "1");
  check_opt_wins(p1, sizeof(p1) / sizeof(inst), win_s_expected, win_e_expected, "1");

  inst p2[] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
               inst(JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
               inst(EXIT),
               inst(MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
               inst(JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
               inst(EXIT),
               inst(MOV64XC, 0, 0),          /* else r0 = 0 */
               inst(EXIT),                   /* exit, return r0 */
              };
  win_s_expected = {0, 3, 6};
  win_e_expected = {0, 3, 6};
  check_gen_wins(p2, sizeof(p2) / sizeof(inst), win_s_expected, win_e_expected, "2");
  win_s_expected = {};
  win_e_expected = {};
  check_opt_wins(p2, sizeof(p2) / sizeof(inst), win_s_expected, win_e_expected, "2");

  inst p3[] = {inst(JA, 0),
               inst(MOV64XC, 1, 0),
               inst(NOP),
               inst(MOV64XC, 2, 0),
               inst(EXIT),
              };
  win_s_expected = {1};
  win_e_expected = {3};
  check_gen_wins(p3, sizeof(p3) / sizeof(inst), win_s_expected, win_e_expected, "3");
  check_opt_wins(p3, sizeof(p3) / sizeof(inst), win_s_expected, win_e_expected, "3");

  inst p4[] = {inst(MOV64XC, 0, 0),
               inst(JEQXC, 0, 0, 3),
               inst(MOV64XC, 0, 0),
               inst(MOV64XC, 0, 0),
               inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  win_s_expected = {0, 2};
  win_e_expected = {0, 4};
  check_gen_wins(p4, sizeof(p4) / sizeof(inst), win_s_expected, win_e_expected, "4");
  win_s_expected = {2};
  win_e_expected = {4};
  check_opt_wins(p4, sizeof(p4) / sizeof(inst), win_s_expected, win_e_expected, "4");

  inst p5[] = {inst(MOV64XC, 0, 0), // 0:
               inst(),
               inst(),
               inst(), // 3:
               inst(),
               inst(JEQXC, 0, 0, 4),
               inst(), // 6:
               inst(),
               inst(), // 8:
               inst(JA, 10),
               inst(), // 10:
               inst(),
               inst(),
               inst(),
               inst(),
               inst(),
               inst(),
               inst(),
               inst(),
               inst(), // 19
               inst(EXIT),
              };
  win_s_expected = {0, 6, 10};
  win_e_expected = {4, 8, 19};
  check_gen_wins(p5, sizeof(p5) / sizeof(inst), win_s_expected, win_e_expected, "5");
  win_s_expected = {0, 6, 10, 14, 18};
  win_e_expected = {3, 8, 13, 17, 19};
  check_opt_wins(p5, sizeof(p5) / sizeof(inst), win_s_expected, win_e_expected, "5");

  cout << "Test 1.2: test the opcode with has multiple insns" << endl;
  // test ldmapid
  inst p2_1[] = {inst(STH, 10, -2, 0xff),
                 INSN_LDMAPID(1, 0),
                 inst(),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -2),
                 inst(CALL, BPF_FUNC_map_lookup_elem),
                 inst(JEQXC, 0, 0, 3),
                 inst(LDXB, 1, 0, 0), // insn 6
                 inst(MOV64XY, 0, 1),
                 inst(EXIT),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  win_s_expected = {0, 3, 7, 10};
  win_e_expected = {0, 4, 8, 10};
  check_gen_wins(p2_1, sizeof(p2_1) / sizeof(inst), win_s_expected, win_e_expected, "1");
  win_s_expected = {3, 7};
  win_e_expected = {4, 8};
  check_opt_wins(p2_1, sizeof(p2_1) / sizeof(inst), win_s_expected, win_e_expected, "1");

  // test movdwxc
  inst p2_2[] = {INSN_MOVDWXC(0, 0x1234567890),
                 inst(EXIT),
                };
  win_s_expected = {};
  win_e_expected = {};
  check_gen_wins(p2_2, sizeof(p2_2) / sizeof(inst), win_s_expected, win_e_expected, "2");
  check_opt_wins(p2_2, sizeof(p2_2) / sizeof(inst), win_s_expected, win_e_expected, "2");
}

int main() {
  logger.set_least_print_level(LOGGER_ERROR);
  try {
    test1();
  } catch (const string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}
