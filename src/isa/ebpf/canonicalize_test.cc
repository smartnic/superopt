#include <iostream>
#include "../../../src/utils.h"
#include "canonicalize.h"

void print_live_regs(unordered_set<int> live_regs) {
  for (const int& x : live_regs) cout << x << " ";
  cout << endl;
}

void canonicalize_check(inst* prog, int len, inst* expected_prog, string test_name) {
  cout << "-------- before ---------" << endl;
  for (int i = 0; i < len; i++) {
    prog[i].print();
  }
  canonicalize(prog, len);
  cout << "-------- after ---------" << endl;
  for (int i = 0; i < len; i++) {
    prog[i].print();
  }
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
  inst expected_prog5[] = {inst(),
                           inst(),
                           inst(),
                           inst(EXIT),
                          };
  canonicalize_check(p5, sizeof(p5) / sizeof(inst), expected_prog5, "5");

  cout << "Program with branches" << endl;
  inst p2_1[] = {inst(JEQXY, 1, 2, 2),
                 inst(ADD64XC, 3, 1),
                 inst(EXIT),
                 inst(ADD64XC, 4, 1),
                 inst(EXIT),
                };
  inst expected_prog2_1[] = {inst(JEQXY, 1, 2, 2),
                             inst(),
                             inst(EXIT),
                             inst(),
                             inst(EXIT),
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
                             inst(),
                             inst(EXIT),
                            };
  canonicalize_check(p2_2, sizeof(p2_2) / sizeof(inst), expected_prog2_2, "2.2");
}

int main(int argc, char *argv[]) {
  try {
    test1();
  } catch (string err_msg) {
    cout << "NOT SUCCESS: " << err_msg << endl;
  }

  return 0;
}