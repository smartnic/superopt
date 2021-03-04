#include "../../src/isa/ebpf/inst.h"
#include "win_select.h"

void check_gen_wins(inst* program, int len,
                    vector<int>& win_s_expected,
                    vector<int>& win_e_expected,
                    string test_name) {
  assert(win_s_expected.size() == win_e_expected.size());
  prog_static_state pss;
  static_analysis(pss, program, len);
  vector<pair<int, int>> wins;
  gen_wins(wins, program, len, pss);
  bool is_equal = true;
  if (wins.size() == win_s_expected.size()) {
    for (int i = 0; i < wins.size(); i++) {
      if ((wins[i].first != win_s_expected[i]) ||
          (wins[i].second != win_e_expected[i])) {
        is_equal = false;
        break;
      }
    }
  } else {
    is_equal = false;
  }
  print_test_res(is_equal, test_name);
}

void test1() {
  cout << "test 1" << endl;
  vector<int> win_s_expected, win_e_expected;
  inst p1[] = {inst(MOV64XC, 1, 0),
               inst(MOV64XC, 2, 2),
               inst(MOV64XC, 3, 0),
               inst(EXIT),
              };

  win_s_expected = {0};
  win_e_expected = {2};
  check_gen_wins(p1, sizeof(p1) / sizeof(inst), win_s_expected, win_e_expected, "1");

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

  inst p3[] = {inst(JA, 0),
               inst(MOV64XC, 1, 0),
               inst(NOP),
               inst(MOV64XC, 2, 0),
               inst(EXIT),
              };
  win_s_expected = {1};
  win_e_expected = {3};
  check_gen_wins(p3, sizeof(p3) / sizeof(inst), win_s_expected, win_e_expected, "3");
}

int main() {
  try {
    test1();
  } catch (const string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}
