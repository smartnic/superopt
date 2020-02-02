#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

#define CONVERT(inst_ptrs, insts, len) {  \
  inst_ptrs.resize(len);  \
  insts->convert_to_pointers(inst_ptrs, insts);  \
}

#define INTERPRET(inst_ptrs, prog_state) inst_ptrs[0]->interpret(inst_ptrs, prog_state)

inst_t instructions1[3] = {inst_t(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                              inst_t(ADD64XY, 0, 0),           /* add64 r0, r0 */
                              inst_t(EXIT),                    /* exit, return r0 */
                             };

inst_t instructions2[3] = {inst_t(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                              inst_t(ADD32XY, 0, 0),           /* add32 r0, r0 */
                              inst_t(EXIT),                    /* exit, return r0 */
                             };
/* test JEQXC */
inst_t instructions3[9] = {inst_t(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                              inst_t(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                              inst_t(MOV64XC, 1, 0x0),        /* r1 = 0 */
                              inst_t(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                              inst_t(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                              inst_t(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                              inst_t(EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                              inst_t(MOV64XC, 0, 0),
                              inst_t(EXIT),
                             };
/* test JEQXY */
inst_t instructions4[9] = {inst_t(MOV32XC, 0, 0xffffffff), /* r0 = 0x00000000ffffffff */
                              inst_t(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                              inst_t(MOV64XC, 1, 0x0),        /* r1 = 0 */
                              inst_t(JEQXY, 0, 1, 4),         /* if r0 == r1, ret r0 = 0x100000000 */
                              inst_t(MOV64XY, 1, 0),          /* else r1 = r0 */
                              inst_t(JEQXY, 0, 1, 1),         /* if r0 == r1, ret r0 = 0x100000001 */
                              inst_t(EXIT),                   /* else ret r0 = 0x100000000 */
                              inst_t(ADD64XC, 0, 0x1),
                              inst_t(EXIT),
                             };

inst_t instructions5[3] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              inst_t(LE16, 0),                /* le16 r0 */
                              inst_t(EXIT),                   /* exit, return r0 */
                             };

inst_t instructions6[3] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              inst_t(LE32, 0),                /* le32 r0 */
                              inst_t(EXIT),                   /* exit, return r0 */
                             };

inst_t instructions7[3] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              inst_t(BE16, 0),                /* be16 r0 */
                              inst_t(EXIT),                   /* exit, return r0 */
                             };

inst_t instructions8[3] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              inst_t(BE32, 0),                /* be32 r0 */
                              inst_t(EXIT),                   /* exit, return r0 */
                             };

inst_t instructions9[6] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                              inst_t(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                              inst_t(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                              inst_t(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                              inst_t(LE64, 0),                /* le64 r0 */
                              inst_t(EXIT),                   /* exit, return r0 */
                             };

inst_t instructions10[6] = {inst_t(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                               inst_t(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                               inst_t(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                               inst_t(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                               inst_t(BE64, 0),                /* be64 r0 */
                               inst_t(EXIT),                   /* exit, return r0 */
                              };

inst_t instructions11[7] = {inst_t(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                               inst_t(RSH64XC, 0, 63),         /* r0 >> 63 */
                               inst_t(JEQXC, 0, 1, 1),         /* if r0 != 0x1, exit */
                               inst_t(EXIT),                   /* exit */
                               inst_t(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                               inst_t(RSH32XC, 0, 1),          /* r0 >>32 1 */
                               inst_t(EXIT),                   /* exit, return r0 */
                              };

inst_t instructions12[8] = {inst_t(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                               inst_t(ARSH64XC, 0, 63),        /* r0 >> 63 */
                               inst_t(MOV64XC, 1, -1),         /* r1 = 0xffffffffffffffff */
                               inst_t(JEQXY, 0, 1, 1),         /* if r0 != r1, exit */
                               inst_t(EXIT),                   /* exit */
                               inst_t(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                               inst_t(ARSH32XC, 0, 1),         /* r0 >>32 1 */
                               inst_t(EXIT),                   /* exit, return r0 */
                              };

inst_t instructions13[8] = {inst_t(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                               inst_t(JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                               inst_t(EXIT),
                               inst_t(MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                               inst_t(JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                               inst_t(EXIT),
                               inst_t(MOV64XC, 0, 0),          /* else r0 = 0 */
                               inst_t(EXIT),                   /* exit, return r0 */
                              };

inst_t instructions14[7] = {inst_t(MOV64XC, 0, -1),         /* r0 = -1 */
                               inst_t(JSGTXC, 0, 0, 4),        /* if r0 s>= 0, ret r0 = -1 */
                               inst_t(JSGTXC, 0, 0xffffffff, 3),/* elif r1 s> 0xffffffff, ret r0 = -1 */
                               inst_t(MOV64XC, 1, 0),          /* r1 = 0 */
                               inst_t(JSGTXY, 0, 1),           /* if r0 s> r1, ret r0 = -1 */
                               inst_t(MOV64XC, 0, 0),          /* else r0 = 0 */
                               inst_t(EXIT),                   /* exit, return r0 */
                              };

inst_t instructions15[4] = {inst_t(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                               inst_t(JGTXC, 0, -2, 1),        /* if r0 > 0xfffffffffffffffe, ret r0 = 0xffffffff */
                               inst_t(MOV64XC, 0, 0),          /* else ret r0 = 0 */
                               inst_t(EXIT),
                              };

void test1() {
  prog_state_t ps;
  cout << "Test 1: full interpretation check" << endl;
  vector<inst*> insts;
  CONVERT(insts, instructions1, 3);
  int64_t expected = 0xfffffffffffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 1");

  CONVERT(insts, instructions2, 3);
  expected = 0xfffffffe;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 2");

  CONVERT(insts, instructions3, 9);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 3");

  CONVERT(insts, instructions4, 9);
  expected = 0x100000001;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 4");

  bool is_le = is_little_endian();
  CONVERT(insts, instructions5, 3);
  if (is_le) expected = 0x01234567;
  else expected = 0x23014567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 5");

  CONVERT(insts, instructions6, 3);
  if (is_le) expected = 0x01234567;
  else expected = 0x67452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 6");

  CONVERT(insts, instructions7, 3);
  if (is_le) expected = 0x01236745;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 7");

  CONVERT(insts, instructions8, 3);
  if (is_le) expected = 0x67452301;
  else expected = 0x01234567;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 8");

  CONVERT(insts, instructions9, 6);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 9");

  CONVERT(insts, instructions10, 6);
  if (is_le) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret program 10");

  CONVERT(insts, instructions11, 7);
  expected = 0x7fffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret rsh64 & rsh32");

  CONVERT(insts, instructions12, 8);
  expected = 0xffffffff;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret arsh64 & arsh32");

  CONVERT(insts, instructions13, 8);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jgt");

  CONVERT(insts, instructions14, 7);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jsgt");

  CONVERT(insts, instructions15, 4);
  expected = 0;
  print_test_res(INTERPRET(insts, ps) == expected, "interpret jgt");
}

int64_t eval_output(z3::expr smt, z3::expr output) {
  z3::solver s(smt_c);
  s.add(smt);
  if (s.check() == z3::sat) {
    z3::model m = s.get_model();
    // get_numeral_int64() fails when 64th bit value is 1
    return m.eval(output).get_numeral_uint64();
  }
  cout << "ERROR: no output, return -1" << endl;
  return -1;
}

bool is_valid(z3::expr smt) {
  z3::solver s(smt_c);
  s.add(smt);
  switch (s.check()) {
    case z3::unsat: return true;
    case z3::sat: return false;
    case z3::unknown:
      cout << "ERROR: smt is_valid unknown, return false" << endl;
      return false;
  }
}

void test2() {
  cout << endl << "Test 2: instruction smt check" << endl;

#define CURDST sv.get_cur_reg_var(insn._args[0])
#define CURSRC sv.get_cur_reg_var(insn._args[1])
// Input FOL formulae (A) should set to `smt` first, then add instruction FOL formula (B),
// Since getting instruction FOL formula will update dst register expression.
// Also cannot add A and B together, such as smt = A && B, since some compilers
// may compute B first.
#define SMT_CHECK_XC(dst_input, dst_expected, test_name)                         \
  smt = (CURDST == to_expr((int64_t)dst_input));                                 \
  smt = smt && insn.smt_inst(sv);                                                \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

#define SMT_CHECK_XY(dst_input, src_input, dst_expected, test_name)              \
  smt = (CURDST == to_expr((int64_t)dst_input)) &&                               \
        (CURSRC == to_expr((int64_t)src_input));                                 \
  smt = smt && insn.smt_inst(sv);                                                \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

  inst_t insn = (NOP);
  int prog_id = 0, node_id = 0, num_regs = insn.get_num_regs();
  smt_var sv(prog_id, node_id, num_regs);
  z3::expr smt = string_to_expr("false");
  z3::expr output = string_to_expr("false");

  insn = inst_t(ADD64XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffffffffffff, 0xfffffffffffffffe, "smt ADD64XC");

  insn = inst_t(ADD64XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0x1fffffffe, "smt ADD64XY");

  insn = inst_t(ADD32XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffff, 0xfffffffe, "smt ADD32XC");

  insn = inst_t(ADD32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0xfffffffe, "smt ADD32XY");

  insn = inst_t(MOV64XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0x0, 0xfffffffffffffffe, "smt MOV64XC");

  insn = inst_t(MOV64XY, 0, 1);
  SMT_CHECK_XY(0x0, 0x0123456789abcdef, 0x0123456789abcdef, "smt MOV64XY");

  insn = inst_t(MOV32XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0xffffffff00000000, 0xfffffffe, "smt MOV32XC");

  insn = inst_t(MOV32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff00000000, 0x0123456789abcdef, 0x89abcdef, "smt MOV32XY");

  insn = inst_t(LSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 0x8000000000000000, "smt LSH64XC");

  insn = inst_t(LSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x8000000000000000, "smt LSH64XY");

  insn = inst_t(LSH32XC, 0, 31);
  SMT_CHECK_XC(0xffffffffffffffff, 0x80000000, "smt LSH32XC");

  insn = inst_t(LSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x80000000, "smt LSH32XY");

  insn = inst_t(RSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 1, "smt RSH64XC");

  insn = inst_t(RSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 63, 1, "smt RSH64XY");

  insn = inst_t(RSH32XC, 0, 31);
  SMT_CHECK_XC((int32_t)0xffffffff, 1, "smt RSH32XC");

  insn = inst_t(RSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 31, 1, "smt RSH32XY");

  insn = inst_t(ARSH64XC, 0, 0x1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffffffffffff, "smt ARSH64XC");

  insn = inst_t(ARSH64XY, 0, 1);
  SMT_CHECK_XY(0x8000000000000000, 63, 0xffffffffffffffff, "smt ARSH64XY");

  insn = inst_t(ARSH32XC, 0, 1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffff, "smt ARSH32XC");

  insn = inst_t(ARSH32XY, 0, 1);
  SMT_CHECK_XY(0xfffffffffffffffe, 31, 0xffffffff, "smt ARSH32XY");

  int64_t expected;
  bool is_le = is_little_endian();
  insn = inst_t(LE16, 0);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0x2301456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE16");

  insn = inst_t(LE32, 0);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0x6745230189abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE32");

  insn = inst_t(LE64, 0);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE64");

  insn = inst_t(BE16, 0);
  if (is_le) expected = 0x0123456789abefcd;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE16");

  insn = inst_t(BE32, 0);
  if (is_le) expected = 0x01234567efcdab89;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE32");

  insn = inst_t(BE64, 0);
  if (is_le) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE64");

#define SMT_JMP_CHECK_XC(dst_input, bool_expected, test_name)                        \
  smt = z3::implies(CURDST == to_expr((int64_t)dst_input), insn.smt_inst_jmp(sv));   \
  print_test_res(is_valid(!smt) == (bool)bool_expected, test_name);

#define SMT_JMP_CHECK_XY(dst_input, src_input, bool_expected, test_name)             \
  smt = (CURDST == to_expr((int64_t)dst_input)) &&                                   \
        (CURSRC == to_expr((int64_t)src_input));                                     \
  smt = z3::implies(smt, insn.smt_inst_jmp(sv));                                     \
  print_test_res(is_valid(!smt) == (bool)bool_expected, test_name);

  insn = inst_t(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0xffffffffffffffff, true, "smt JEQXC 1");

  insn = inst_t(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0x00000000ffffffff, false, "smt JEQXC 2");

  insn = inst_t(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0xffffffffffffffff, true, "smt JEQXY 1");

  insn = inst_t(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x00000000ffffffff, 0xffffffffffffffff, false, "smt JEQXY 2");

  insn = inst_t(JGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0xffffffff80000001, true, "smt JGTXC 1");

  insn = inst_t(JGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JGTXC 2");

  insn = inst_t(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x7fffffffffffffff, true, "smt JGTXY 1");

  insn = inst_t(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x2, false, "smt JGTXY 2");

  insn = inst_t(JSGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0x0, true, "smt JSGTXC 1");

  insn = inst_t(JSGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JSGTXC 2");

  insn = inst_t(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x8000000000000000, true, "smt JSGTXY 1");

  insn = inst_t(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x2, false, "smt JSGTXY 2");

#undef CURDST
#undef CURSRC
#undef SMT_CHECK_XC
#undef SMT_CHECK_XY
#undef SMT_JMP_CHECK_XC
#undef SMT_JMP_CHECK_XY
}

int main(int argc, char *argv[]) {
  test1();
  test2();

  return 0;
}
