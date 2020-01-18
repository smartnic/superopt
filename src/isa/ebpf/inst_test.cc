#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

#define COVNERT(inst_ptrs, insts, len) {  \
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

ebpf_inst instructions15[4] = {ebpf_inst(ebpf::MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                               ebpf_inst(ebpf::JGTXC, 0, -2, 1),        /* if r0 > 0xfffffffffffffffe, ret r0 = 0xffffffff */
                               ebpf_inst(ebpf::MOV64XC, 0, 0),          /* else ret r0 = 0 */
                               ebpf_inst(ebpf::EXIT),
                              };

void test1() {
  ebpf_prog_state ps;
  cout << "Test 1: full interpretation check" << endl;
  vector<inst*> insts;
  COVNERT(insts, instructions1, 3);
  int64_t expected = 0xfffffffffffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 1");

  COVNERT(insts, instructions2, 3);
  expected = 0xfffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 2");

  COVNERT(insts, instructions3, 9);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 3");

  COVNERT(insts, instructions4, 9);
  expected = 0x100000001;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 4");

  COVNERT(insts, instructions5, 3);
  if (is_little_endian()) expected = 0x01234567;
  else expected = 0x23014567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 5");

  COVNERT(insts, instructions6, 3);
  if (is_little_endian()) expected = 0x01234567;
  else expected = 0x67452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 6");

  COVNERT(insts, instructions7, 3);
  if (is_little_endian()) expected = 0x01236745;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 7");

  COVNERT(insts, instructions8, 3);
  if (is_little_endian()) expected = 0x67452301;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 8");

  COVNERT(insts, instructions9, 6);
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 9");

  COVNERT(insts, instructions10, 6);
  if (is_little_endian()) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 10");

  COVNERT(insts, instructions11, 7);
  expected = 0x7fffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret rsh64 & rsh32");

  COVNERT(insts, instructions12, 8);
  expected = 0xffffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret arsh64 & arsh32");

  COVNERT(insts, instructions13, 8);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jgt");

  COVNERT(insts, instructions14, 7);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jsgt");

  COVNERT(insts, instructions15, 4);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jgt");
}

int64_t eval_output(z3::expr smt, z3::expr output) {
  z3::solver s(smt_c);
  s.add(smt);
  if (s.check() == z3::sat) {
    z3::model m = s.get_model();
    return m.eval(output).get_numeral_int64();
  }
  cout << "ERROR: no output, return -1" << endl;
  return -1;
}

void test2() {
  cout << endl << "Test 2: instruction smt check" << endl;

#define CURDST sv.get_cur_reg_var(insn._args[0])
#define CURSRC sv.get_cur_reg_var(insn._args[1])
#define SMT_CHECK_XC(dst_input, dst_expected, test_name)                         \
  smt = (CURDST == to_expr((int64_t)dst_input)) && insn.smt_inst(sv);            \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

#define SMT_CHECK_XY(dst_input, src_input, dst_expected, test_name)              \
  smt = (CURDST == to_expr((int64_t)dst_input)) &&                               \
        (CURSRC == to_expr((int64_t)src_input)) &&                               \
        insn.smt_inst(sv);                                                       \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

  ebpf_inst insn = (ebpf::NOP);
  int prog_id = 0, node_id = 0, num_regs = insn.get_num_regs();
  smt_var sv(prog_id, node_id, num_regs);
  z3::expr smt = string_to_expr("false");
  z3::expr output = string_to_expr("false");

  insn = ebpf_inst(ebpf::ADD64XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffffffffffff, 0xfffffffffffffffe, "smt ADD64XC");

  insn = ebpf_inst(ebpf::ADD64XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0x1fffffffe, "smt ADD64XY");

  insn = ebpf_inst(ebpf::ADD32XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffff, 0xfffffffe, "smt ADD32XC");

  insn = ebpf_inst(ebpf::ADD32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0xfffffffe, "smt ADD32XY");

  insn = ebpf_inst(ebpf::MOV64XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0x0, 0xfffffffffffffffe, "smt MOV64XC");

  insn = ebpf_inst(ebpf::MOV64XY, 0, 1);
  SMT_CHECK_XY(0x0, 0x0123456789abcdef, 0x0123456789abcdef, "smt MOV64XY");

  insn = ebpf_inst(ebpf::MOV32XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0xffffffff00000000, 0xfffffffe, "smt MOV32XC");

  insn = ebpf_inst(ebpf::MOV32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff00000000, 0x0123456789abcdef, 0x89abcdef, "smt MOV32XY");

  insn = ebpf_inst(ebpf::RSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 1, "smt RSH64XC");

  insn = ebpf_inst(ebpf::RSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 63, 1, "smt RSH64XY");

  insn = ebpf_inst(ebpf::RSH32XC, 0, 31);
  SMT_CHECK_XC((int32_t)0xffffffff, 1, "smt RSH32XC");

  insn = ebpf_inst(ebpf::RSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 31, 1, "smt RSH32XY");

  insn = ebpf_inst(ebpf::ARSH64XC, 0, 0x1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffffffffffff, "smt ARSH64XC");

  insn = ebpf_inst(ebpf::ARSH64XY, 0, 1);
  SMT_CHECK_XY(0x8000000000000000, 63, 0xffffffffffffffff, "smt ARSH64XY");

  insn = ebpf_inst(ebpf::ARSH32XC, 0, 1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffff, "smt ARSH32XC");

  insn = ebpf_inst(ebpf::ARSH32XY, 0, 1);
  SMT_CHECK_XY(0xfffffffffffffffe, 31, 0xffffffff, "smt ARSH32XY");

  int64_t expected;
  insn = ebpf_inst(ebpf::LE16, 0);
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0x2301456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE16");

  insn = ebpf_inst(ebpf::LE32, 0);
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0x6745230189abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE32");

  insn = ebpf_inst(ebpf::LE64, 0);
  if (is_little_endian()) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE64");

  insn = ebpf_inst(ebpf::BE16, 0);
  if (is_little_endian()) expected = 0x0123456789abefcd;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE16");

  insn = ebpf_inst(ebpf::BE32, 0);
  if (is_little_endian()) expected = 0x01234567efcdab89;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE32");

  insn = ebpf_inst(ebpf::BE64, 0);
  if (is_little_endian()) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE64");

#undef CURDST
#undef CURSRC
#undef SMT_CHECK_XC
#undef SMT_CHECK_XY
}

int main(int argc, char *argv[]) {
  test1();
  test2();

  return 0;
}
