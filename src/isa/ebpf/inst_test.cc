#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

inst instructions1[3] = {inst(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                         inst(ADD64XY, 0, 0),           /* add64 r0, r0 */
                         inst(EXIT),                    /* exit, return r0 */
                        };

inst instructions2[3] = {inst(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                         inst(ADD32XY, 0, 0),           /* add32 r0, r0 */
                         inst(EXIT),                    /* exit, return r0 */
                        };
/* test JEQXC */
inst instructions3[9] = {inst(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                         inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                         inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                         inst(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                         inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                         inst(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                         inst(EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                         inst(MOV64XC, 0, 0),
                         inst(EXIT),
                        };
/* test JEQXY */
inst instructions4[9] = {inst(MOV32XC, 0, 0xffffffff), /* r0 = 0x00000000ffffffff */
                         inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                         inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                         inst(JEQXY, 0, 1, 4),         /* if r0 == r1, ret r0 = 0x100000000 */
                         inst(MOV64XY, 1, 0),          /* else r1 = r0 */
                         inst(JEQXY, 0, 1, 1),         /* if r0 == r1, ret r0 = 0x100000001 */
                         inst(EXIT),                   /* else ret r0 = 0x100000000 */
                         inst(ADD64XC, 0, 0x1),
                         inst(EXIT),
                        };

inst instructions5[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LE, 0, 16),                /* le16 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions6[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LE, 0, 32),                /* le32 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions7[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(BE, 0, 16),                /* be16 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions8[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(BE, 0, 32),                /* be32 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions9[6] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                         inst(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                         inst(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                         inst(LE, 0, 64),                /* le64 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions10[6] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                          inst(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                          inst(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                          inst(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                          inst(BE, 0, 64),                /* be64 r0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions11[7] = {inst(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                          inst(RSH64XC, 0, 63),         /* r0 >> 63 */
                          inst(JEQXC, 0, 1, 1),         /* if r0 != 0x1, exit */
                          inst(EXIT),                   /* exit */
                          inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                          inst(RSH32XC, 0, 1),          /* r0 >>32 1 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions12[8] = {inst(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                          inst(ARSH64XC, 0, 63),        /* r0 >> 63 */
                          inst(MOV64XC, 1, -1),         /* r1 = 0xffffffffffffffff */
                          inst(JEQXY, 0, 1, 1),         /* if r0 != r1, exit */
                          inst(EXIT),                   /* exit */
                          inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                          inst(ARSH32XC, 0, 1),         /* r0 >>32 1 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions13[8] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                          inst(JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                          inst(EXIT),
                          inst(MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                          inst(JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                          inst(EXIT),
                          inst(MOV64XC, 0, 0),          /* else r0 = 0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions14[7] = {inst(MOV64XC, 0, -1),         /* r0 = -1 */
                          inst(JSGTXC, 0, 0, 4),        /* if r0 s>= 0, ret r0 = -1 */
                          inst(JSGTXC, 0, 0xffffffff, 3),/* elif r1 s> 0xffffffff, ret r0 = -1 */
                          inst(MOV64XC, 1, 0),          /* r1 = 0 */
                          inst(JSGTXY, 0, 1),           /* if r0 s> r1, ret r0 = -1 */
                          inst(MOV64XC, 0, 0),          /* else r0 = 0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions15[4] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                          inst(JGTXC, 0, -2, 1),        /* if r0 > 0xfffffffffffffffe, ret r0 = 0xffffffff */
                          inst(MOV64XC, 0, 0),          /* else ret r0 = 0 */
                          inst(EXIT),
                         };

void test1() {
  prog_state ps;
  cout << "Test 1: full interpretation check" << endl;

  int64_t expected = 0xfffffffffffffffe;
  print_test_res(interpret(instructions1, 3, ps) == expected, "interpret program 1");

  expected = 0xfffffffe;
  print_test_res(interpret(instructions2, 3, ps) == expected, "interpret program 2");

  expected = 0;
  print_test_res(interpret(instructions3, 9, ps) == expected, "interpret program 3");

  expected = 0x100000001;
  print_test_res(interpret(instructions4, 9, ps) == expected, "interpret program 4");

  bool is_le = is_little_endian();
  if (is_le) expected = 0x4567;
  else expected = 0x6745;
  print_test_res(interpret(instructions5, 3, ps) == expected, "interpret program 5");

  if (is_le) expected = 0x01234567;
  else expected = 0x67452301;
  print_test_res(interpret(instructions6, 3, ps) == expected, "interpret program 6");

  if (is_le) expected = 0x6745;
  else expected = 0x4567;
  print_test_res(interpret(instructions7, 3, ps) == expected, "interpret program 7");

  if (is_le) expected = 0x67452301;
  else expected = 0x01234567;
  print_test_res(interpret(instructions8, 3, ps) == expected, "interpret program 8");

  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  print_test_res(interpret(instructions9, 6, ps) == expected, "interpret program 9");

  if (is_le) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  print_test_res(interpret(instructions10, 6, ps) == expected, "interpret program 10");

  expected = 0x7fffffff;
  print_test_res(interpret(instructions11, 7, ps) == expected, "interpret rsh64 & rsh32");

  expected = 0xffffffff;
  print_test_res(interpret(instructions12, 8, ps) == expected, "interpret arsh64 & arsh32");

  expected = 0;
  print_test_res(interpret(instructions13, 8, ps) == expected, "interpret jgt");

  expected = 0;
  print_test_res(interpret(instructions14, 7, ps) == expected, "interpret jsgt");

  expected = 0;
  print_test_res(interpret(instructions15, 4, ps) == expected, "interpret jgt");
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

#define CURDST sv.get_cur_reg_var(insn._dst_reg)
#define CURSRC sv.get_cur_reg_var(insn._src_reg)
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

  inst insn = (NOP);
  int prog_id = 0, node_id = 0;
  smt_var sv(prog_id, node_id, NUM_REGS);
  z3::expr smt = string_to_expr("false");
  z3::expr output = string_to_expr("false");

  insn = inst(ADD64XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffffffffffff, 0xfffffffffffffffe, "smt ADD64XC");

  insn = inst(ADD64XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0x1fffffffe, "smt ADD64XY");

  insn = inst(ADD32XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffff, 0xfffffffe, "smt ADD32XC");

  insn = inst(ADD32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0xfffffffe, "smt ADD32XY");

  insn = inst(MOV64XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0x0, 0xfffffffffffffffe, "smt MOV64XC");

  insn = inst(MOV64XY, 0, 1);
  SMT_CHECK_XY(0x0, 0x0123456789abcdef, 0x0123456789abcdef, "smt MOV64XY");

  insn = inst(MOV32XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0xffffffff00000000, 0xfffffffe, "smt MOV32XC");

  insn = inst(MOV32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff00000000, 0x0123456789abcdef, 0x89abcdef, "smt MOV32XY");

  insn = inst(LSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 0x8000000000000000, "smt LSH64XC");

  insn = inst(LSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x8000000000000000, "smt LSH64XY");

  insn = inst(LSH32XC, 0, 31);
  SMT_CHECK_XC(0xffffffffffffffff, 0x80000000, "smt LSH32XC");

  insn = inst(LSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x80000000, "smt LSH32XY");

  insn = inst(RSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 1, "smt RSH64XC");

  insn = inst(RSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 63, 1, "smt RSH64XY");

  insn = inst(RSH32XC, 0, 31);
  SMT_CHECK_XC((int32_t)0xffffffff, 1, "smt RSH32XC");

  insn = inst(RSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 31, 1, "smt RSH32XY");

  insn = inst(ARSH64XC, 0, 0x1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffffffffffff, "smt ARSH64XC");

  insn = inst(ARSH64XY, 0, 1);
  SMT_CHECK_XY(0x8000000000000000, 63, 0xffffffffffffffff, "smt ARSH64XY");

  insn = inst(ARSH32XC, 0, 1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffff, "smt ARSH32XC");

  insn = inst(ARSH32XY, 0, 1);
  SMT_CHECK_XY(0xfffffffffffffffe, 31, 0xffffffff, "smt ARSH32XY");

  int64_t expected;
  bool is_le = is_little_endian();
  insn = inst(LE, 0, 16);
  if (is_le) expected = 0xcdef;
  else expected = 0xefcd;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE16");

  insn = inst(LE, 0, 32);
  if (is_le) expected = 0x89abcdef;
  else expected = 0xefcdab89;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE32");

  insn = inst(LE, 0, 64);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE64");

  insn = inst(BE, 0, 16);
  if (is_le) expected = 0xefcd;
  else expected = 0xcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE16");

  insn = inst(BE, 0, 32);
  if (is_le) expected = 0xefcdab89;
  else expected = 0x89abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE32");

  insn = inst(BE, 0, 64);
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

  insn = inst(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0xffffffffffffffff, true, "smt JEQXC 1");

  insn = inst(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0x00000000ffffffff, false, "smt JEQXC 2");

  insn = inst(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0xffffffffffffffff, true, "smt JEQXY 1");

  insn = inst(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x00000000ffffffff, 0xffffffffffffffff, false, "smt JEQXY 2");

  insn = inst(JGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0xffffffff80000001, true, "smt JGTXC 1");

  insn = inst(JGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JGTXC 2");

  insn = inst(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x7fffffffffffffff, true, "smt JGTXY 1");

  insn = inst(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x2, false, "smt JGTXY 2");

  insn = inst(JSGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0x0, true, "smt JSGTXC 1");

  insn = inst(JSGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JSGTXC 2");

  insn = inst(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x8000000000000000, true, "smt JSGTXY 1");

  insn = inst(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x2, false, "smt JSGTXY 2");

#undef CURDST
#undef CURSRC
#undef SMT_CHECK_XC
#undef SMT_CHECK_XY
#undef SMT_JMP_CHECK_XC
#undef SMT_JMP_CHECK_XY
}

// test3 is to check whether the ebpf inst is the same as linux bpf
void test3() {
  cout << endl << "Test 3: ebpf bytecode check" << endl;
  // this has been checked by bpf-step
  inst prog[] = {inst(MOV64XY, 6, 1),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -4),
                 inst(MOV64XC, 1, 0x10),
                 inst(JGTXC, 1, -1, 1),
                 inst(MOV64XC, 1, 1),
                 inst(JEQXC, 0, 0, 1),
                 inst(MOV64XC, 0, 1),
                 inst(EXIT),
                };
  string expected = "{191, 6, 1, 0, 0},"\
                    "{191, 2, 10, 0, 0},"\
                    "{7, 2, 0, 0, -4},"\
                    "{183, 1, 0, 0, 16},"\
                    "{37, 1, 0, 1, -1},"\
                    "{183, 1, 0, 0, 1},"\
                    "{21, 0, 0, 1, 0},"\
                    "{183, 0, 0, 0, 1},"\
                    "{149, 0, 0, 0, 0},";
  string prog_bytecode = "";
  for (int i = 0; i < 9; i++) {
    prog_bytecode += prog[i].get_bytecode_str() + ",";
  }
  print_test_res(prog_bytecode == expected, "ebpf bytecode 1");

  // test all opcodes: has included all opcodes except NOP, 
  // since there is no NOP in linux bpf
  inst prog2[30] = {inst(ADD64XC, 3, 1),
                    inst(ADD64XY, 3, 1),
                    inst(LSH64XC, 3, 1),
                    inst(LSH64XY, 3, 1),
                    inst(RSH64XC, 3, 1),
                    inst(RSH64XY, 3, 1),
                    inst(MOV64XC, 3, 1),
                    inst(MOV64XY, 3, 1),
                    inst(ARSH64XC, 3, 1),
                    inst(ARSH64XY, 3, 1),
                    inst(ADD32XC, 3, 1),
                    inst(ADD32XY, 3, 1),
                    inst(LSH32XC, 3, 1),
                    inst(LSH32XY, 3, 1),
                    inst(RSH32XC, 3, 1),
                    inst(RSH32XY, 3, 1),
                    inst(MOV32XC, 3, 1),
                    inst(MOV32XY, 3, 1),
                    inst(ARSH32XC, 3, 1),
                    inst(ARSH32XY, 3, 1),
                    inst(LE, 3, 16),
                    inst(BE, 3, 16),
                    inst(JA, 1),
                    inst(JEQXC, 3, 1, 2),
                    inst(JEQXY, 3, 1, 2),
                    inst(JGTXC, 3, 1, 2),
                    inst(JGTXY, 3, 1, 2),
                    inst(JSGTXC, 3, 1, 2),
                    inst(JSGTXY, 3, 1, 2),
                    inst(EXIT),
                   };
  expected = "{7, 3, 0, 0, 1},"\
             "{15, 3, 1, 0, 0},"\
             "{103, 3, 0, 0, 1},"\
             "{111, 3, 1, 0, 0},"\
             "{119, 3, 0, 0, 1},"\
             "{127, 3, 1, 0, 0},"\
             "{183, 3, 0, 0, 1},"\
             "{191, 3, 1, 0, 0},"\
             "{199, 3, 0, 0, 1},"\
             "{207, 3, 1, 0, 0},"\
             "{4, 3, 0, 0, 1},"\
             "{12, 3, 1, 0, 0},"\
             "{100, 3, 0, 0, 1},"\
             "{108, 3, 1, 0, 0},"\
             "{116, 3, 0, 0, 1},"\
             "{124, 3, 1, 0, 0},"\
             "{180, 3, 0, 0, 1},"\
             "{188, 3, 1, 0, 0},"\
             "{196, 3, 0, 0, 1},"\
             "{204, 3, 1, 0, 0},"\
             "{212, 3, 0, 0, 16},"\
             "{220, 3, 0, 0, 16},"\
             "{5, 0, 0, 1, 0},"\
             "{21, 3, 0, 2, 1},"\
             "{29, 3, 1, 2, 0},"\
             "{37, 3, 0, 2, 1},"\
             "{45, 3, 1, 2, 0},"\
             "{101, 3, 0, 2, 1},"\
             "{109, 3, 1, 2, 0},"\
             "{149, 0, 0, 0, 0},";
  prog_bytecode = "";
  for (int i = 0; i < 30; i++) {
    prog_bytecode += prog2[i].get_bytecode_str() + ",";
  }
  print_test_res(prog_bytecode == expected, "ebpf bytecode 2");
}

void test4() {
  cout << endl << "Test 3: ebpf opcode and idx conversion check" << endl;
  bool check_res = true;
  for (int idx = 0; idx < NUM_INSTR; idx++) {
    if (opcode_2_idx(idx_2_opcode[idx]) != idx) {
      print_test_res(false, "idx:" + to_string(idx) + " opcode conversion");
      check_res = false;
    }
  }
  if (check_res) {
    print_test_res(true, "opcode and idx conversion");
  }
}

int main(int argc, char *argv[]) {
  test1();
  test2();
  test3();
  test4();

  return 0;
}
