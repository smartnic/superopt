#include <unordered_set>
#include "win_select.h"

using namespace std;

void test_satisfy_constraints(inst* pgm, int len,
                              unordered_set<int>& false_insns_exp,
                              string test_name) {
  prog_static_state pss;
  static_analysis(pss, pgm, len);
  bool test_res;
  vector<int> failed_insns;
  for (int i = 0; i < len; i++) {
    bool res = insn_satisfy_isa_win_constraints(pgm[i], pss.static_state[i]);
    bool res_exp = (false_insns_exp.find(i) == false_insns_exp.end());
    if (res != res_exp) failed_insns.push_back(i);
  }
  if (failed_insns.size() == 0) {
    print_test_res(true, test_name);
  } else {
    print_test_res(false, test_name);
    cout << "failed insn ids: ";
    for (int i = 0; i < failed_insns.size(); i++) {
      cout << i << " ";
    }
    cout << endl;
  }
}

void test1() {
  cout << "Test1: test insn_satisfy_isa_win_constraints" << endl;
  cout << "1. test function call" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  mem_t::add_map(map_attr(16, 32, 16));
  inst p1_1[] = {inst(STB, 10, -1, 0x1),
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
  const int len_p1_1 = sizeof(p1_1) / sizeof(inst);
  unordered_set<int> false_insns_exp = {4};
  test_satisfy_constraints(p1_1, len_p1_1, false_insns_exp, "1");

  cout << "2. test memory access for PGM_INPUT_pkt" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(32);
  inst p2_1[] = {inst(LDXB, 5, 1, 0), // r5 = *r1
                 inst(STXB, 1, 2, 5),
                 inst(ADD64XY, 1, 1),
                 inst(STXB, 1, 1, 5),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  const int len_p2_1 = sizeof(p2_1) / sizeof(inst);
  false_insns_exp = {3};
  test_satisfy_constraints(p2_1, len_p2_1, false_insns_exp, "1");

  cout << "3. test symbolic memory access for PGM_INPUT_pkt_ptrs" << endl;
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(32);
  inst p3_1[] = {inst(LDXW, 2, 1, 4), // r2: PTR_TO_PACKET_END
                 inst(LDXW, 7, 1, 0), // r7: pkt_s
                 inst(MOV64XY, 3, 7), // r3 = r7 + 4
                 inst(ADD64XC, 3, 4),
                 inst(MOV64XY, 0, 7), // r0 = r7
                 inst(JGEXC, 3, 0xff, 1),
                 inst(ADD64XC, 0, 4),  // 6: r0 += 4
                 inst(JGTXY, 3, 2, 2), // if r3 > r2, exit;
                 inst(STB, 0, 0, 1),   // 8:
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                };
  const int len_p3_1 = sizeof(p3_1) / sizeof(inst);
  false_insns_exp = {8};
  test_satisfy_constraints(p3_1, len_p3_1, false_insns_exp, "1");
}

int main() {
  try {
    test1();
  } catch (const string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}
