#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "cfg.h"

using namespace std;

inst instructions1[8] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                         inst(JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                         inst(EXIT),
                         inst(MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                         inst(JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                         inst(EXIT),
                         inst(MOV64XC, 0, 0),          /* else r0 = 0 */
                         inst(EXIT),                   /* exit, return r0 */
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

// test jmp and st/ld
inst instructions3[6] = {inst(STXB, 10, -1, 1),
                         inst(JEQXC, 1, 0x12, 2),
                         inst(MOV64XC, 1, 0x12),
                         inst(STXW, 10, -1, 1),
                         inst(LDXB, 0, 10, -1),
                         inst(EXIT),
                        };

// test tail call
inst instructions4[6] = {inst(MOV64XY, 1, 1),
                         inst(LDMAPID, 2, 0),
                         inst(MOV64XC, 3, 1),
                         inst(CALL, BPF_FUNC_tail_call),
                         inst(MOV64XC, 0, 0xff),
                         inst(EXIT),
                        };

void test1() {
  string expected;
  graph g1(instructions1, 8);
  expected = "nodes:0,1 2,2 3,4 5,5 6,7 edges: 0:;1,2, 1:0,; 2:0,;3,4, 3:2,; 4:2,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");

  graph g2(instructions2, 9);
  expected = "nodes:0,3 4,5 6,6 8,8 7,7 edges: 0:;1,3, 1:0,;2,4, 2:1,;3, 3:2,4,0,; 4:1,;3,";
  print_test_res(g2.graph_to_str() == expected, "program 2");

  graph g3(instructions3, 6);
  expected = "nodes:0,1 2,3 4,5 edges: 0:;1,2, 1:0,;2, 2:1,0,;";
  print_test_res(g3.graph_to_str() == expected, "program 3");

  graph g4(instructions4, 6);
  expected = "nodes:0,3 edges: 0:;";
  print_test_res(g4.graph_to_str() == expected, "program 4");
}

int main() {
  test1();

  return 0;
}
