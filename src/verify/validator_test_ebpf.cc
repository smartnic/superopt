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
  inst_t instructions1[9] = {inst_t(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                                inst_t(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                                inst_t(MOV64XC, 1, 0x0),        /* r1 = 0 */
                                inst_t(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                                inst_t(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                                inst_t(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                                inst_t(EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                                inst_t(MOV64XC, 0, 0),
                                inst_t(EXIT),
                               };

  inst_t instructions2[9] = {inst_t(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                                inst_t(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                                inst_t(MOV64XC, 1, 0x0),        /* r1 = 0 */
                                inst_t(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                                inst_t(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                                inst_t(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                                inst_t(JA, 1),                  /* else ret r0 = 0xffffffffffffffff */
                                inst_t(MOV64XC, 0, 0),
                                inst_t(EXIT),
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
