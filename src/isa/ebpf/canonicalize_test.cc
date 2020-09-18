#include <iostream>
#include "canonicalize.h"

void print_live_regs(unordered_set<int> live_regs) {
  for (const int& x : live_regs) cout << x << " ";
  cout << endl;
}

void liveness_analysis_check(inst* prog, int len) {
  unordered_set<int> live_regs;
  unordered_set<int> initial_live_regs = {0}; // 0 is the output register
  liveness_analysis(live_regs, prog, 0, len - 1, initial_live_regs);
  cout << "-----------------" << endl;
  cout << "live regs: ";
  print_live_regs(live_regs);
  for (int i = 0; i < len; i++) prog[i].print();
  cout << "-----------------" << endl;
}

void test1() {
  cout << "Test 1: liveness analysis" << endl;
  inst p1[] = {inst(MOV64XC, 0, 0),
               inst(EXIT),
              };
  liveness_analysis_check(p1, sizeof(p1) / sizeof(inst));
  inst p2[] = {inst(MOV64XC, 0, -1),
               inst(ADD64XY, 0, 0),
               inst(EXIT),
              };
  liveness_analysis_check(p2, sizeof(p2) / sizeof(inst));
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
  liveness_analysis_check(p3, sizeof(p3) / sizeof(inst));
  inst p4[] = {inst(MOV64XC, 2, 10),
               inst(ADD64XC, 2, -8),
               inst(CALL, BPF_FUNC_map_lookup_elem),
              };
  liveness_analysis_check(p4, sizeof(p4) / sizeof(inst));
  inst p5[] = {inst(LDXB, 1, 1, 0),
               inst(ADD64XC, 1, 5),
               inst(MOV64XY, 2, 10),
               inst(EXIT),
              };
  liveness_analysis_check(p5, sizeof(p5) / sizeof(inst));
}

int main(int argc, char *argv[]) {
  try {
    test1();
  } catch (string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}