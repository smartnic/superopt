#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "cfg.h"

using namespace std;

#define COVNERT(inst_ptrs, insts, len) {  \
  inst_ptrs.resize(len);  \
  insts->convert_to_pointers(inst_ptrs, insts);  \
}

ebpf_inst instructions1[8] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                              ebpf_inst(ebpf::JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                              ebpf_inst(ebpf::EXIT),
                              ebpf_inst(ebpf::MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                              ebpf_inst(ebpf::JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                              ebpf_inst(ebpf::EXIT),
                              ebpf_inst(ebpf::MOV64XC, 0, 0),          /* else r0 = 0 */
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions2[9] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                              ebpf_inst(ebpf::ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                              ebpf_inst(ebpf::MOV64XC, 1, 0x0),        /* r1 = 0 */
                              ebpf_inst(ebpf::JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                              ebpf_inst(ebpf::MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                              ebpf_inst(ebpf::JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                              ebpf_inst(ebpf::JA, 1),                  /* else ret r0 = 0xffffffffffffffff */
                              ebpf_inst(ebpf::MOV64XC, 0, 0),
                              ebpf_inst(ebpf::EXIT),
                             };

void test1() {
  vector<inst*> insts;
  string expected;
  COVNERT(insts, instructions1, 8);
  graph g1(insts);
  expected = "nodes:0,1 2,2 3,4 5,5 6,7 edges: 0:;1,2, 1:0,; 2:0,;3,4, 3:2,; 4:2,;";
  print_test_res(g1.graph_to_str() == expected, "program 1");

  COVNERT(insts, instructions2, 9);
  graph g2(insts);
  expected = "nodes:0,3 4,5 6,6 8,8 7,7 edges: 0:;1,3, 1:0,;2,4, 2:1,;3, 3:2,4,0,; 4:1,;3,";
  print_test_res(g2.graph_to_str() == expected, "program 2");
}

int main() {
  test1();

  return 0;
}