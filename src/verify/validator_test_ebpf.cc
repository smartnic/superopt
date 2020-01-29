#include <iostream>
#include "../../src/utils.h"
#include "../../src/isa/ebpf/inst.h"
#include "validator.h"

using namespace z3;

#define CONVERT(inst_ptrs, insts, len) {  \
  inst_ptrs.resize(len);  \
  insts->convert_to_pointers(inst_ptrs, insts);  \
}

void test1() {
  std::cout << "test 1:" << endl;
  ebpf_inst instructions1[9] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                                ebpf_inst(ebpf::ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                                ebpf_inst(ebpf::MOV64XC, 1, 0x0),        /* r1 = 0 */
                                ebpf_inst(ebpf::JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                                ebpf_inst(ebpf::MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                                ebpf_inst(ebpf::JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                                ebpf_inst(ebpf::EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                                ebpf_inst(ebpf::MOV64XC, 0, 0),
                                ebpf_inst(ebpf::EXIT),
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
  vector<inst*> insts;
  CONVERT(insts, instructions1, 9);
  validator vld(insts);

  CONVERT(insts, instructions1, 9);
  print_test_res(vld.is_equal_to(insts), "instructions1 == instructions1");

  CONVERT(insts, instructions2, 9);
  print_test_res(vld.is_equal_to(insts), "instructions1 == instructions2");
}

int main() {
  test1();

  return 0;
}