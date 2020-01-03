#include <iostream>
#include "../inst_codegen.h"
#include "inst.h"

using namespace std;

constexpr int toy_isa::num_operands[NUM_INSTR];
constexpr int toy_isa::insn_num_regs[NUM_INSTR];
constexpr int toy_isa::opcode_type[NUM_INSTR];
constexpr int toy_isa::optable[NUM_INSTR];

string toy_isa_inst::opcode_to_str(int opcode) const {
  switch (opcode) {
    case toy_isa::NOP: return "NOP";
    case toy_isa::ADDXY: return "ADDXY";
    case toy_isa::MOVXC: return "MOVXC";
    case toy_isa::RETX: return "RETX";
    case toy_isa::RETC: return "RETC";
    case toy_isa::JMP: return "JMP";
    case toy_isa::JMPEQ: return "JMPEQ";
    case toy_isa::JMPGT: return "JMPGT";
    case toy_isa::JMPGE: return "JMPGE";
    case toy_isa::JMPLT: return "JMPLT";
    case toy_isa::JMPLE: return "JMPLE";
    case toy_isa::MAXC: return "MAXC";
    case toy_isa::MAXX: return "MAXX";
    default: return "unknown opcode";
  }
}

abs_bv_inst toy_isa_inst::inst_to_abs_bv() const {
  int v = (_opcode << (OP_ABS_BIT_LEN * 3)) +
          (_args[0] << (OP_ABS_BIT_LEN * 2)) +
          (_args[1] << (OP_ABS_BIT_LEN * 1)) +
          (_args[2]);
  abs_bv_inst bv(v);
  return bv;
}

toy_isa_inst& toy_isa_inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

int toy_isa_inst::get_max_operand_val(int op_index, int inst_index) const {
  // max value for each operand type
  int max_val[4] = {
    [toy_isa::OP_UNUSED] = 0,
    [toy_isa::OP_REG] = toy_isa::NUM_REGS,
    [toy_isa::OP_IMM] = toy_isa::MAX_CONST,
    [toy_isa::OP_OFF] = toy_isa::MAX_PROG_LEN - inst_index - 1,
  };
  return max_val[TOY_ISA_OPTYPE(_opcode, op_index)];
}

int toy_isa_inst::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return _args[0];
    case (OP_COND_JMP): return _args[2];
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

void toy_isa_inst::insert_jmp_opcodes(unordered_set<int>& jmp_sets) const {
  for (enum toy_isa::OPCODES opcode = toy_isa::JMP; opcode <= toy_isa::JMPLE;
       opcode = toy_isa::OPCODES(opcode + 1)) {
    jmp_sets.insert(opcode);
  }
}

int toy_isa_inst::inst_output_opcode_type() const {
  switch (_opcode) {
    case toy_isa::RETX:
      return RET_X;
    case toy_isa::RETC:
      return RET_C;
    default: // no RET, return register 0
      return RET_X;
  }
}

int toy_isa_inst::inst_output() const {
  switch (_opcode) {
    case toy_isa::RETX:
      return TOY_ISA_DSTREG(*this);
    case toy_isa::RETC:
      return TOY_ISA_IMM1VAL(*this);
    default: // no RET, return register 0
      return 0;
  }
}

bool toy_isa_inst::is_real_inst() const {
  if (_opcode == toy_isa::NOP) return false;
  return true;
}

int toy_isa_inst::interpret(int length, toy_isa_prog_state &ps, int input) {
  /* Input currently is just one integer which will be written into R0. Will
  need to generalize this later. */
  inst *start = this;
  inst *insn = this;
  ps.clear();
  ps.regs[0] = input;

  static void *jumptable[toy_isa::NUM_INSTR] = {
    [toy_isa::NOP]   = && INSN_NOP,
    [toy_isa::ADDXY] = && INSN_ADDXY,
    [toy_isa::MOVXC] = && INSN_MOVXC,
    [toy_isa::RETX] = && INSN_RETX,
    [toy_isa::RETC] = && INSN_RETC,
    [toy_isa::JMP] = && INSN_JMP,
    [toy_isa::JMPEQ] = && INSN_JMPEQ,
    [toy_isa::JMPGT] = && INSN_JMPGT,
    [toy_isa::JMPGE] = && INSN_JMPGE,
    [toy_isa::JMPLT] = && INSN_JMPLT,
    [toy_isa::JMPLE] = && INSN_JMPLE,
    [toy_isa::MAXC] = && INSN_MAXC,
    [toy_isa::MAXX] = && INSN_MAXX,
  };

#define CONT { \
      insn++;                                                           \
      if (insn < start + length) {                                    \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }
#define DST ps.regs[TOY_ISA_DSTREG(*insn)]
#define SRC ps.regs[TOY_ISA_SRCREG(*insn)]
#define IMM1 TOY_ISA_IMM1VAL(*insn)
#define IMM2 TOY_ISA_IMM2VAL(*insn)

select_insn:
  goto *jumptable[insn->_opcode];

INSN_NOP:
  CONT;

INSN_ADDXY:
  DST = compute_add(DST, SRC, DST);
  CONT;

INSN_MOVXC:
  DST = compute_mov(IMM2, DST);
  CONT;

INSN_RETX:
  return DST;

INSN_RETC:
  return IMM1;

INSN_MAXC:
  DST = compute_max(DST, IMM2, DST);
  CONT;

INSN_MAXX:
  DST = compute_max(DST, SRC, DST);
  CONT;

INSN_JMP:
  insn += IMM1;
  CONT;

#define COND_JMP(SUFFIX, OP)                    \
  INSN_JMP##SUFFIX:                             \
      if (DST OP SRC)                           \
        insn += insn->_args[2];                 \
  CONT;

  COND_JMP(EQ, == )
  COND_JMP(GT, > )
  COND_JMP(GE, >= )
  COND_JMP(LT, < )
  COND_JMP(LE, <= )

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return -1;

out:
  //cout << "Error: program terminated without RET; returning R0" << endl;
  return ps.regs[0]; /* return default R0 value */
}

ostream& operator<<(ostream& out, abs_bv_inst& bv) {
  for (size_t i = 0; i < bv.size(); i++)
    out << bv[i];
  return out;
}
