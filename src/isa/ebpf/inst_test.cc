#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

#define COVERT(inst_ptrs, insts, len) {  \
  inst_ptrs.resize(len);  \
  insts->convert_to_pointers(inst_ptrs, insts);  \
}

#define INTERPRET(inst_ptrs, prog_state) inst_ptrs[0]->interpret(inst_ptrs, prog_state)

ebpf_inst instructions1[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                              ebpf_inst(ebpf::ADD64XY, 0, 0),           /* add64 r0, r0 */
                              ebpf_inst(ebpf::EXIT),                    /* exit, return r0 */
                             };

ebpf_inst instructions2[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                              ebpf_inst(ebpf::ADD32XY, 0, 0),           /* add32 r0, r0 */
                              ebpf_inst(ebpf::EXIT),                    /* exit, return r0 */
                             };
/* test JEQXC */
ebpf_inst instructions3[9] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                              ebpf_inst(ebpf::ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */ 
                              ebpf_inst(ebpf::MOV64XC, 1, 0x0),        /* r1 = 0 */
                              ebpf_inst(ebpf::JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                              ebpf_inst(ebpf::MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */ 
                              ebpf_inst(ebpf::JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                              ebpf_inst(ebpf::EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                              ebpf_inst(ebpf::MOV64XC, 0, 0), 
                              ebpf_inst(ebpf::EXIT),
                             };
/* test JEQXY */
ebpf_inst instructions4[9] = {ebpf_inst(ebpf::MOV32XC, 0, 0xffffffff), /* r0 = 0x00000000ffffffff */
                              ebpf_inst(ebpf::ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */ 
                              ebpf_inst(ebpf::MOV64XC, 1, 0x0),        /* r1 = 0 */
                              ebpf_inst(ebpf::JEQXY, 0, 1, 4),         /* if r0 == r1, ret r0 = 0x100000000 */
                              ebpf_inst(ebpf::MOV64XY, 1, 0),          /* else r1 = r0 */ 
                              ebpf_inst(ebpf::JEQXY, 0, 1, 1),         /* if r0 == r1, ret r0 = 0x100000001 */
                              ebpf_inst(ebpf::EXIT),                   /* else ret r0 = 0x100000000 */
                              ebpf_inst(ebpf::ADD64XC, 0, 0x1), 
                              ebpf_inst(ebpf::EXIT),
                             };

ebpf_inst instructions5[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              ebpf_inst(ebpf::LE16, 0),                /* le16 r0 */ 
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions6[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              ebpf_inst(ebpf::LE32, 0),                /* le32 r0 */ 
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions7[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              ebpf_inst(ebpf::BE16, 0),                /* be16 r0 */ 
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions8[3] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              ebpf_inst(ebpf::BE32, 0),                /* be32 r0 */ 
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions9[6] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              ebpf_inst(ebpf::LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                              ebpf_inst(ebpf::MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                              ebpf_inst(ebpf::ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                              ebpf_inst(ebpf::LE64, 0),                /* le64 r0 */
                              ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                             };

ebpf_inst instructions10[6] = {ebpf_inst(ebpf::MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                               ebpf_inst(ebpf::LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                               ebpf_inst(ebpf::MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                               ebpf_inst(ebpf::ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                               ebpf_inst(ebpf::BE64, 0),                /* be64 r0 */
                               ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                              };

ebpf_inst instructions11[7] = {ebpf_inst(ebpf::MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::RSH64XC, 0, 63),         /* r0 >> 63 */
                               ebpf_inst(ebpf::JEQXC, 0, 1, 1),         /* if r0 != 0x1, exit */
                               ebpf_inst(ebpf::EXIT),                   /* exit */
                               ebpf_inst(ebpf::MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::RSH32XC, 0, 1),          /* r0 >>32 1 */
                               ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                              };

ebpf_inst instructions12[8] = {ebpf_inst(ebpf::MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::ARSH64XC, 0, 63),        /* r0 >> 63 */
                               ebpf_inst(ebpf::MOV64XC, 1, -1),         /* r1 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::JEQXY, 0, 1, 1),         /* if r0 != r1, exit */
                               ebpf_inst(ebpf::EXIT),                   /* exit */
                               ebpf_inst(ebpf::MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::ARSH32XC, 0, 1),         /* r0 >>32 1 */
                               ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                              };

ebpf_inst instructions13[8] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                               ebpf_inst(ebpf::JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                               ebpf_inst(ebpf::EXIT),
                               ebpf_inst(ebpf::MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                               ebpf_inst(ebpf::JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                               ebpf_inst(ebpf::EXIT),
                               ebpf_inst(ebpf::MOV64XC, 0, 0),          /* else r0 = 0 */
                               ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                              };

ebpf_inst instructions14[7] = {ebpf_inst(ebpf::MOV64XC, 0, -1),         /* r0 = -1 */
                               ebpf_inst(ebpf::JSGTXC, 0, 0, 4),        /* if r0 s>= 0, ret r0 = -1 */
                               ebpf_inst(ebpf::JSGTXC, 0, 0xffffffff, 3),/* elif r1 s> 0xffffffff, ret r0 = -1 */
                               ebpf_inst(ebpf::MOV64XC, 1, 0),          /* r1 = 0 */
                               ebpf_inst(ebpf::JSGTXY, 0, 1),           /* if r0 s> r1, ret r0 = -1 */
                               ebpf_inst(ebpf::MOV64XC, 0, 0),          /* else r0 = 0 */
                               ebpf_inst(ebpf::EXIT),                   /* exit, return r0 */
                              };

void test1() {
  ebpf_prog_state ps;
  cout << "Test 1: full interpretation check" << endl;
  vector<inst*> insts;
  COVERT(insts, instructions1, 3);
  int64_t expected = 0xfffffffffffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 1");

  COVERT(insts, instructions2, 3);
  expected = 0xfffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 2");

  COVERT(insts, instructions3, 9);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 3");

  COVERT(insts, instructions4, 9);
  expected = 0x100000001;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 4");

  COVERT(insts, instructions5, 3);
  if (is_little_endian()) expected = 0x01234567;
  else expected = 0x23014567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 5");

  COVERT(insts, instructions6, 3);
  if (is_little_endian()) expected = 0x01234567;
  else expected = 0x67452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 6");

  COVERT(insts, instructions7, 3);
  if (is_little_endian()) expected = 0x01236745;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 7");

  COVERT(insts, instructions8, 3);
  if (is_little_endian()) expected = 0x67452301;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 8");

  COVERT(insts, instructions9, 6);
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 9");

  COVERT(insts, instructions10, 6);
  if (is_little_endian()) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 10");

  COVERT(insts, instructions11, 7);
  expected = 0x7fffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret rsh64 & rsh32");

  COVERT(insts, instructions12, 8);
  expected = 0xffffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret arsh64 & arsh32");

  COVERT(insts, instructions13, 8);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jgt");

  COVERT(insts, instructions14, 7);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jsgt");
}

int main(int argc, char *argv[]) {
  test1();

  return 0;
}
