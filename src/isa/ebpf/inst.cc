#include <iostream>
#include <cassert>
#include "../inst_codegen.h"
#include "inst.h"

using namespace std;

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]
#define UNCOND_OFFVAL(inst_var) (inst_var)._args[0]
#define COND_OFFVAL(inst_var) (inst_var)._args[2]

#define IMM1VAL32(inst_var) (int32_t)(IMM1VAL(inst_var))
#define IMM2VAL32(inst_var) (int32_t)(IMM2VAL(inst_var))
#define UNCOND_OFFVAL16(inst_var) (int16_t)(UNCOND_OFFVAL(inst_var))
#define COND_OFFVAL16(inst_var) (int16_t)(COND_OFFVAL(inst_var))

constexpr int ebpf::num_operands[NUM_INSTR];
constexpr int ebpf::insn_num_regs[NUM_INSTR];
constexpr int ebpf::opcode_type[NUM_INSTR];
constexpr int ebpf::optable[NUM_INSTR];

string ebpf_inst::opcode_to_str(int opcode) const {
  switch (opcode) {
    case ebpf::NOP: return "";
    case ebpf::ADD64XC: return "add";
    case ebpf::ADD64XY: return "add";
    case ebpf::LSH64XC: return "lsh";
    case ebpf::LSH64XY: return "lsh";
    case ebpf::RSH64XC: return "rsh";
    case ebpf::RSH64XY: return "rsh";
    case ebpf::MOV64XC: return "mov";
    case ebpf::MOV64XY: return "mov";
    case ebpf::ARSH64XC: return "arsh";
    case ebpf::ARSH64XY: return "arsh";
    case ebpf::ADD32XC: return "add32";
    case ebpf::ADD32XY: return "add32";
    case ebpf::LSH32XC: return "lsh32";
    case ebpf::LSH32XY: return "lsh32";
    case ebpf::RSH32XC: return "rsh32";
    case ebpf::RSH32XY: return "rsh32";
    case ebpf::MOV32XC: return "mov32";
    case ebpf::MOV32XY: return "mov32";
    case ebpf::ARSH32XC: return "arsh32";
    case ebpf::ARSH32XY: return "arsh32";
    case ebpf::LE16: return "le16";
    case ebpf::LE32: return "le32";
    case ebpf::LE64: return "le64";
    case ebpf::BE16: return "be16";
    case ebpf::BE32: return "be32";
    case ebpf::BE64: return "be64";
    case ebpf::JA: return "ja";
    case ebpf::JEQXC: return "jeq";
    case ebpf::JEQXY: return "jeq";
    case ebpf::JGTXC: return "jgt";
    case ebpf::JGTXY: return "jgt";
    case ebpf::JSGTXC: return "jsgt";
    case ebpf::JSGTXY: return "jsgt";
    case ebpf::EXIT: return "exit";
    default: return "unknown opcode";
  }
}

ebpf_inst& ebpf_inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

// For jmp opcode, it can only jump forward
int ebpf_inst::get_max_operand_val(int op_index, int inst_index) const {
  // max value for each operand type
  int max_val[4] = {
    [ebpf::OP_UNUSED] = 0,
    [ebpf::OP_REG] = ebpf::NUM_REGS,
    [ebpf::OP_IMM] = ebpf::MAX_IMM,
    [ebpf::OP_OFF] = min((int)ebpf::MAX_OFF, ebpf::MAX_PROG_LEN - inst_index - 1),
  };
  return max_val[EBPF_OPTYPE(_opcode, op_index)];
}

void ebpf_inst::make_insts(vector<inst*> &insts, const vector<inst*> &other) const {
  int num_inst = insts.size();
  ebpf_inst* new_insts = new ebpf_inst[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = *other[i];
  }
  for (int i = 0; i < num_inst; i++) {
    insts[i] = &new_insts[i];
  }
}

void ebpf_inst::make_insts(vector<inst*> &insts, const inst* instruction) const {
  int num_inst = insts.size();
  ebpf_inst* new_insts = new ebpf_inst[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = instruction[i];
  }
  for (int i = 0; i < num_inst; i++) {
    insts[i] = &new_insts[i];
  }
}

void ebpf_inst::clear_insts() {
  delete []this;
}

int ebpf_inst::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return UNCOND_OFFVAL16(*this);
    case (OP_COND_JMP): return COND_OFFVAL16(*this);
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

void ebpf_inst::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
  jmp_set.insert(ebpf::JA);
  jmp_set.insert(ebpf::JEQXC);
  jmp_set.insert(ebpf::JEQXY);
  jmp_set.insert(ebpf::JGTXC);
  jmp_set.insert(ebpf::JGTXY);
}

int ebpf_inst::inst_output_opcode_type() const {
  switch (_opcode) {
    case ebpf::EXIT: return RET_X;
    default: cout << "Error: opcode is not EXIT" << endl; return RET_X;
  }
}

int ebpf_inst::inst_output() const {
  switch (_opcode) {
    case ebpf::EXIT: return 0;
    default: cout << "Error: opcode is not EXIT" << endl; return 0;
  }
}

bool ebpf_inst::is_real_inst() const {
  if (_opcode == ebpf::NOP) return false;
  return true;
}

void ebpf_inst::set_as_nop_inst() {
  _opcode = ebpf::NOP;
  _args[0] = 0;
  _args[1] = 0;
  _args[2] = 0;
}

int64_t ebpf_inst::interpret(const vector<inst*> &insts, prog_state &ps, int input) const {
// type: int64_t
#define DST ps.regs[DSTREG(*insts[insn])]
#define SRC ps.regs[SRCREG(*insts[insn])]
#define IMM1 (int64_t)IMM1VAL32(*insts[insn])
#define IMM2 (int64_t)IMM2VAL32(*insts[insn])
// type: int32_t
#define DST32 (int32_t)(DST)
#define SRC32 (int32_t)(SRC)
#define IMM1_32 (int32_t)IMM1
#define IMM2_32 (int32_t)IMM2
// type: uint64_t
#define UDST (uint64_t)DST
#define USRC (uint64_t)SRC
#define UIMM1 (uint64_t)IMM1
#define UIMM2 (uint64_t)IMM2

#define SRC_L6 L6(SRC)
#define SRC32_L5 L5(SRC32)
#define UNCOND_OFF16 UNCOND_OFFVAL16(*insts[insn])
#define COND_OFF16 COND_OFFVAL16(*insts[insn])

#define CONT {                                                     \
      insn++;                                                      \
      if (insn < length) {                                         \
        goto select_insn;                                          \
      } else goto out;                                             \
  }

#define ALU64_UNARY(OPCODE, OP, OPERAND)                           \
  INSN_##OPCODE:                                                   \
    DST = compute_##OP(OPERAND);                                   \
    CONT;

#define ALU64_BINARY(OPCODE, OP, OPERAND1, OPERAND2)               \
  INSN_##OPCODE:                                                   \
    DST = compute_##OP(OPERAND1, OPERAND2);                        \
    CONT;

#define ALU32_UNARY(OPCODE, OP, OPERAND)                           \
  INSN_##OPCODE:                                                   \
    DST = L32(compute_##OP(OPERAND));                              \
    CONT;

#define ALU32_BINARY(OPCODE, OP, OPERAND1, OPERAND2)               \
  INSN_##OPCODE:                                                   \
    DST = L32(compute_##OP(OPERAND1, OPERAND2));                   \
    CONT;

#define BYTESWAP(OPCODE, OP)                                       \
  INSN_##OPCODE:                                                   \
    DST = compute_##OP(DST);                                       \
    CONT;

#define COND_JMP(OPCODE, OP, OPERAND1, OPERAND2)                   \
  INSN_##OPCODE:                                                   \
    if (OPERAND1 OP OPERAND2)                                      \
      insn += COND_OFF16;                                          \
  CONT;

  int insn = 0;
  int length = insts.size();
  ps.clear();

  static void *jumptable[ebpf::NUM_INSTR] = {
    [ebpf::NOP]      = && INSN_NOP,
    [ebpf::ADD64XC]  = && INSN_ADD64XC,
    [ebpf::ADD64XY]  = && INSN_ADD64XY,
    [ebpf::LSH64XC]  = && INSN_LSH64XC,
    [ebpf::LSH64XY]  = && INSN_LSH64XY,
    [ebpf::RSH64XC]  = && INSN_RSH64XC,
    [ebpf::RSH64XY]  = && INSN_RSH64XY,
    [ebpf::MOV64XC]  = && INSN_MOV64XC,
    [ebpf::MOV64XY]  = && INSN_MOV64XY,
    [ebpf::ARSH64XC] = && INSN_ARSH64XC,
    [ebpf::ARSH64XY] = && INSN_ARSH64XY,

    [ebpf::ADD32XC]  = && INSN_ADD32XC,
    [ebpf::ADD32XY]  = && INSN_ADD32XY,
    [ebpf::LSH32XC]  = && INSN_LSH32XC,
    [ebpf::LSH32XY]  = && INSN_LSH32XY,
    [ebpf::RSH32XC]  = && INSN_RSH32XC,
    [ebpf::RSH32XY]  = && INSN_RSH32XY,
    [ebpf::MOV32XC]  = && INSN_MOV32XC,
    [ebpf::MOV32XY]  = && INSN_MOV32XY,
    [ebpf::ARSH32XC] = && INSN_ARSH32XC,
    [ebpf::ARSH32XY] = && INSN_ARSH32XY,

    [ebpf::LE16]     = && INSN_LE16,
    [ebpf::LE32]     = && INSN_LE32,
    [ebpf::LE64]     = && INSN_LE64,
    [ebpf::BE16]     = && INSN_BE16,
    [ebpf::BE32]     = && INSN_BE32,
    [ebpf::BE64]     = && INSN_BE64,

    [ebpf::JA]       = && INSN_JA,
    [ebpf::JEQXC]    = && INSN_JEQXC,
    [ebpf::JEQXY]    = && INSN_JEQXY,
    [ebpf::JGTXC]    = && INSN_JGTXC,
    [ebpf::JGTXY]    = && INSN_JGTXY,
    [ebpf::JSGTXC]   = && INSN_JSGTXC,
    [ebpf::JSGTXY]   = && INSN_JSGTXY,
    [ebpf::EXIT]     = && INSN_EXIT,
  };

select_insn:
  goto *jumptable[insts[insn]->_opcode];

INSN_NOP:
  CONT;

  ALU64_UNARY(MOV64XC, mov, IMM2)
  ALU64_UNARY(MOV64XY, mov, SRC)
  ALU64_BINARY(ADD64XC, add, DST, IMM2)
  ALU64_BINARY(ADD64XY, add, DST, SRC)
  ALU64_BINARY(LSH64XC, lsh, DST, IMM2)
  ALU64_BINARY(LSH64XY, lsh, DST, SRC_L6)
  ALU64_BINARY(RSH64XC, rsh, DST, IMM2)
  ALU64_BINARY(RSH64XY, rsh, DST, SRC_L6)
  ALU64_BINARY(ARSH64XC, arsh, DST, IMM2)
  ALU64_BINARY(ARSH64XY, arsh, DST, SRC_L6)

  ALU32_UNARY(MOV32XC, mov, IMM2_32)
  ALU32_UNARY(MOV32XY, mov, SRC32)
  ALU32_BINARY(ADD32XC, add, DST32, IMM2_32)
  ALU32_BINARY(ADD32XY, add, DST32, SRC32)
  ALU32_BINARY(LSH32XC, lsh, DST32, IMM2_32)
  ALU32_BINARY(LSH32XY, lsh, DST32, SRC32_L5)
  ALU32_BINARY(RSH32XC, rsh, DST32, IMM2_32)
  ALU32_BINARY(RSH32XY, rsh, DST32, SRC32_L5)
  ALU32_BINARY(ARSH32XC, arsh, DST32, IMM2_32)
  ALU32_BINARY(ARSH32XY, arsh, DST32, SRC32_L5)

  BYTESWAP(LE16, le16)
  BYTESWAP(LE32, le32)
  BYTESWAP(LE64, le64)
  BYTESWAP(BE16, be16)
  BYTESWAP(BE32, be32)
  BYTESWAP(BE64, be64)

INSN_JA:
  insn += UNCOND_OFF16;
  CONT;

  COND_JMP(JEQXC, ==, DST, IMM2)
  COND_JMP(JEQXY, ==, DST, SRC)
  COND_JMP(JGTXC, >, UDST, UIMM2)
  COND_JMP(JGTXY, >, UDST, USRC)
  COND_JMP(JSGTXC, >, DST, IMM2)
  COND_JMP(JSGTXY, >, DST, SRC)

INSN_EXIT:
  return ps.regs[0];

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return -1;

out:
  //cout << "Error: program terminated without RET; returning R0" << endl;
  return ps.regs[0]; /* return default R0 value */
}

#undef IMM2
#undef IMM2_32
// int64_t
#define NEWDST newDst
#define CURDST curDst
#define CURSRC curSrc
#define IMM2 to_expr(imm2)
// bv of int64_t
#define NEWDST_BV z3::int2bv(64, newDst)
#define CURDST_BV z3::int2bv(64, curDst)
#define CURSRC_BV z3::int2bv(64, curSrc)
#define IMM2_BV to_expr(imm2, 64)
// int32_t
#define NEWDST32 z3::bv2int(NEWDST32_BV, true)
#define CURDST32 z3::bv2int(CURDST32_BV, true)
#define CURSRC32 z3::bv2int(CURSRC32_BV, true)
#define IMM2_32 to_expr((int32_t)imm2)
// bv of int32_t
#define NEWDST32_BV z3::int2bv(32, newDst)
#define CURDST32_BV z3::int2bv(32, curDst)
#define CURSRC32_BV z3::int2bv(32, curSrc)
#define IMM2_32_BV to_expr((int32_t)imm2, 32)

#define CURSRC_L6_BV (CURSRC_BV & to_expr(0x3f, 64))
#define CURSRC32_L5_BV (CURSRC32_BV & to_expr(0x1f, 32))

z3::expr ebpf_inst::smt_inst(smt_var& sv) const {
  // check whether opcode is valid. If invalid, curDst cannot be updated to get newDst
  // If opcode is valid, then define curDst, curSrc, imm2 and newDst
  if (get_opcode_type() != OP_OTHERS) return string_to_expr("false");
  // Should get curDst and curSrc before updating curDst (curSrc may be the same reg as curDst)
  z3::expr curDst = sv.get_cur_reg_var(DSTREG(*this));
  z3::expr curSrc = string_to_expr("false");
  if (SRCREG(*this) < get_num_regs() && SRCREG(*this) >= 0) {
    curSrc = sv.get_cur_reg_var(SRCREG(*this));
  }
  z3::expr newDst = sv.update_reg_var(DSTREG(*this));
  int64_t imm2 = (int64_t)IMM2VAL32(*this);

  switch (_opcode) {
    case ebpf::ADD64XC: return predicate_add(CURDST, IMM2, NEWDST);
    case ebpf::ADD64XY: return predicate_add(CURDST, CURSRC, NEWDST);
    case ebpf::LSH64XC: return predicate_lsh(CURDST_BV, IMM2_BV, NEWDST);
    case ebpf::LSH64XY: return predicate_lsh(CURDST_BV, CURSRC_L6_BV, NEWDST);
    case ebpf::RSH64XC: return predicate_rsh(CURDST_BV, IMM2_BV, NEWDST);
    case ebpf::RSH64XY: return predicate_rsh(CURDST_BV, CURSRC_L6_BV, NEWDST);
    case ebpf::MOV64XC: return predicate_mov(IMM2, NEWDST);
    case ebpf::MOV64XY: return predicate_mov(CURSRC, NEWDST);
    case ebpf::ARSH64XC: return predicate_arsh(CURDST_BV, IMM2_BV, NEWDST);
    case ebpf::ARSH64XY: return predicate_arsh(CURDST_BV, CURSRC_L6_BV, NEWDST);
    case ebpf::ADD32XC: return predicate_add32(CURDST32, IMM2_32, NEWDST);
    case ebpf::ADD32XY: return predicate_add32(CURDST32, CURSRC32, NEWDST);
    case ebpf::LSH32XC: return predicate_lsh32(CURDST32_BV, IMM2_32_BV, NEWDST);
    case ebpf::LSH32XY: return predicate_lsh32(CURDST32_BV, CURSRC32_L5_BV, NEWDST);
    case ebpf::RSH32XC: return predicate_rsh32(CURDST32_BV, IMM2_32_BV, NEWDST);
    case ebpf::RSH32XY: return predicate_rsh32(CURDST32_BV, CURSRC32_L5_BV, NEWDST);
    case ebpf::MOV32XC: return predicate_mov32(IMM2_32, NEWDST);
    case ebpf::MOV32XY: return predicate_mov32(CURSRC32, NEWDST);
    case ebpf::ARSH32XC: return predicate_arsh32(CURDST32_BV, IMM2_32_BV, NEWDST);
    case ebpf::ARSH32XY: return predicate_arsh32(CURDST32_BV, CURSRC32_L5_BV, NEWDST);
    case ebpf::LE16: return predicate_le16(CURDST, NEWDST);
    case ebpf::LE32: return predicate_le32(CURDST, NEWDST);
    case ebpf::LE64: return predicate_le64(CURDST, NEWDST);
    case ebpf::BE16: return predicate_be16(CURDST, NEWDST);
    case ebpf::BE32: return predicate_be32(CURDST, NEWDST);
    case ebpf::BE64: return predicate_be64(CURDST, NEWDST);
    default: return string_to_expr("false");
  }
}

z3::expr ebpf_inst::smt_inst_jmp(smt_var& sv) const {
  // If opcode is valid, then define curDst, curSrc, imm2
  if (get_opcode_type() != OP_COND_JMP) return string_to_expr("false");
  z3::expr curDst = sv.get_cur_reg_var(DSTREG(*this));
  z3::expr curSrc = string_to_expr("false");
  if (SRCREG(*this) < get_num_regs() && SRCREG(*this) >= 0) {
    curSrc = sv.get_cur_reg_var(SRCREG(*this));
  }
  int64_t imm2 = (int64_t)IMM2VAL32(*this);

  switch (_opcode) {
    case ebpf::JEQXC: return (CURDST == IMM2);
    case ebpf::JEQXY: return (CURDST == CURSRC);
    case ebpf::JGTXC: return (ugt(CURDST_BV, IMM2_BV));
    case ebpf::JGTXY: return (ugt(CURDST_BV, CURSRC_BV));
    case ebpf::JSGTXC: return (CURDST > IMM2);
    case ebpf::JSGTXY: return (CURDST > CURSRC);
    default: return string_to_expr("false");
  }
}
