#include <iostream>
#include "inst.h"

using namespace std;

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]

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

toy_isa_inst& toy_isa_inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

// For jmp opcode, it can only jump forward
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

void toy_isa_inst::make_insts(vector<inst*> &instptr_list, const vector<inst*> &other) const {
  int num_inst = instptr_list.size();
  toy_isa_inst* new_insts = new toy_isa_inst[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = *other[i];
  }
  for (int i = 0; i < num_inst; i++) {
    instptr_list[i] = &new_insts[i];
  }
}

void toy_isa_inst::make_insts(vector<inst*> &instptr_list, const inst* instruction) const {
  int num_inst = instptr_list.size();
  toy_isa_inst* new_insts = new toy_isa_inst[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = instruction[i];
  }
  for (int i = 0; i < num_inst; i++) {
    instptr_list[i] = &new_insts[i];
  }
}

void toy_isa_inst::clear_insts() {
  delete []this;
}

int toy_isa_inst::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return _args[0];
    case (OP_COND_JMP): return _args[2];
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

void toy_isa_inst::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
  jmp_set.insert(toy_isa::JMP);
  jmp_set.insert(toy_isa::JMPEQ);
  jmp_set.insert(toy_isa::JMPGT);
  jmp_set.insert(toy_isa::JMPGE);
  jmp_set.insert(toy_isa::JMPLT);
  jmp_set.insert(toy_isa::JMPLE);
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
      return DSTREG(*this);
    case toy_isa::RETC:
      return IMM1VAL(*this);
    default: // no RET, return register 0
      return 0;
  }
}

bool toy_isa_inst::is_real_inst() const {
  if (_opcode == toy_isa::NOP) return false;
  return true;
}

void toy_isa_inst::set_as_nop_inst() {
  _opcode = toy_isa::NOP;
  _args[0] = 0;
  _args[1] = 0;
  _args[2] = 0;
}

int64_t toy_isa_inst::interpret(const vector<inst*> &instptr_list, prog_state &ps, int64_t input) const {
  /* Input currently is just one integer which will be written into R0. Will
  need to generalize this later. */
  // inst *insn = this;
  int insn = 0;
  int length = instptr_list.size();
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

#undef CONT
#define CONT { \
      insn++;                                                           \
      if (insn < length) {                                    \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }

#undef DST
#undef SRC
#undef IMM1
#undef IMM2
#define DST ps.regs[DSTREG(*instptr_list[insn])]
#define SRC ps.regs[SRCREG(*instptr_list[insn])]
#define IMM1 IMM1VAL(*instptr_list[insn])
#define IMM2 IMM2VAL(*instptr_list[insn])

select_insn:
  goto *jumptable[instptr_list[insn]->_opcode];

INSN_NOP:
  CONT;

INSN_ADDXY:
  DST = toy_isa_compute_add(DST, SRC, DST);
  CONT;

INSN_MOVXC:
  DST = toy_isa_compute_mov(IMM2, DST);
  CONT;

INSN_RETX:
  return DST;

INSN_RETC:
  return IMM1;

INSN_MAXC:
  DST = toy_isa_compute_max(DST, IMM2, DST);
  CONT;

INSN_MAXX:
  DST = toy_isa_compute_max(DST, SRC, DST);
  CONT;

INSN_JMP:
  insn += IMM1;
  CONT;

#undef COND_JMP
#define COND_JMP(SUFFIX, OP)                    \
  INSN_JMP##SUFFIX:                             \
      if (DST OP SRC)                           \
        insn += instptr_list[insn]->_args[2];                 \
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


#undef IMM2
#define CURSRC sv.get_cur_reg_var(SRCREG(*this))
#define CURDST sv.get_cur_reg_var(DSTREG(*this))
#define NEWDST sv.update_reg_var(DSTREG(*this))
#define IMM2 to_expr(IMM2VAL(*this))

z3::expr toy_isa_inst::smt_inst(smt_var& sv) const {
  z3::expr curDst = string_to_expr("false");
  z3::expr curSrc = string_to_expr("false");
  z3::expr newDst = string_to_expr("false");
  z3::expr imm = string_to_expr("false");
  switch (_opcode) {
    case toy_isa::MAXX:
    case toy_isa::ADDXY:
      curDst = CURDST;
      curSrc = CURSRC;
      newDst = NEWDST;
      break;
    case toy_isa::MOVXC:
      imm = IMM2;
      newDst = NEWDST;
      break;
    case toy_isa::MAXC:
      curDst = CURDST;
      imm = IMM2;
      newDst = NEWDST;
      break;
  }
  switch (_opcode) {
    case toy_isa::ADDXY: return toy_isa_predicate_add(curDst, curSrc, newDst);
    case toy_isa::MOVXC: return toy_isa_predicate_mov(imm, newDst);
    case toy_isa::MAXC: return toy_isa_predicate_max(curDst, imm, newDst);
    case toy_isa::MAXX: return toy_isa_predicate_max(curDst, curSrc, newDst);
    default: return string_to_expr("false");
  }
}

z3::expr toy_isa_inst::smt_inst_jmp(smt_var& sv) const {
  switch (_opcode) {
    case toy_isa::JMPEQ: return (CURDST == CURSRC);
    case toy_isa::JMPGT: return (CURDST > CURSRC);
    case toy_isa::JMPGE: return (CURDST >= CURSRC);
    case toy_isa::JMPLT: return (CURDST < CURSRC);
    case toy_isa::JMPLE: return (CURDST <= CURSRC);
    default: return string_to_expr("false");
  }
}
