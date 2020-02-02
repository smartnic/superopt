#include <iostream>
#include "inst.h"

using namespace std;

#define DSTREG(inst_var) (inst_var)._args[0]
#define SRCREG(inst_var) (inst_var)._args[1]
#define IMM1VAL(inst_var) (inst_var)._args[0]
#define IMM2VAL(inst_var) (inst_var)._args[1]

string inst_t::opcode_to_str(int opcode) const {
  switch (opcode) {
    case NOP: return "NOP";
    case ADDXY: return "ADDXY";
    case MOVXC: return "MOVXC";
    case RETX: return "RETX";
    case RETC: return "RETC";
    case JMP: return "JMP";
    case JMPEQ: return "JMPEQ";
    case JMPGT: return "JMPGT";
    case JMPGE: return "JMPGE";
    case JMPLT: return "JMPLT";
    case JMPLE: return "JMPLE";
    case MAXC: return "MAXC";
    case MAXX: return "MAXX";
    default: return "unknown opcode";
  }
}

inst_t& inst_t::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

// For jmp opcode, it can only jump forward
int inst_t::get_max_operand_val(int op_index, int inst_index) const {
  // max value for each operand type
  int max_val[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = NUM_REGS,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = MAX_PROG_LEN - inst_index - 1,
  };
  return max_val[OPTYPE(_opcode, op_index)];
}

void inst_t::make_insts(vector<inst*> &instptr_list, const vector<inst*> &other) const {
  int num_inst = instptr_list.size();
  inst_t* new_insts = new inst_t[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = *other[i];
  }
  for (int i = 0; i < num_inst; i++) {
    instptr_list[i] = &new_insts[i];
  }
}

void inst_t::make_insts(vector<inst*> &instptr_list, const inst* instruction) const {
  int num_inst = instptr_list.size();
  inst_t* new_insts = new inst_t[num_inst];
  for (int i = 0; i < num_inst; i++) {
    new_insts[i] = instruction[i];
  }
  for (int i = 0; i < num_inst; i++) {
    instptr_list[i] = &new_insts[i];
  }
}

void inst_t::clear_insts() {
  delete []this;
}

int inst_t::get_jmp_dis() const {
  switch (get_opcode_type()) {
    case (OP_UNCOND_JMP): return _args[0];
    case (OP_COND_JMP): return _args[2];
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

void inst_t::insert_jmp_opcodes(unordered_set<int>& jmp_set) const {
  jmp_set.insert(JMP);
  jmp_set.insert(JMPEQ);
  jmp_set.insert(JMPGT);
  jmp_set.insert(JMPGE);
  jmp_set.insert(JMPLT);
  jmp_set.insert(JMPLE);
}

int inst_t::inst_output_opcode_type() const {
  switch (_opcode) {
    case RETX:
      return RET_X;
    case RETC:
      return RET_C;
    default: // no RET, return register 0
      return RET_X;
  }
}

int inst_t::inst_output() const {
  switch (_opcode) {
    case RETX:
      return DSTREG(*this);
    case RETC:
      return IMM1VAL(*this);
    default: // no RET, return register 0
      return 0;
  }
}

bool inst_t::is_real_inst() const {
  if (_opcode == NOP) return false;
  return true;
}

void inst_t::set_as_nop_inst() {
  _opcode = NOP;
  _args[0] = 0;
  _args[1] = 0;
  _args[2] = 0;
}

int inst_t::interpret(const vector<inst*> &instptr_list, prog_state &ps, int input) const {
  /* Input currently is just one integer which will be written into R0. Will
  need to generalize this later. */
  // inst *insn = this;
  int insn = 0;
  int length = instptr_list.size();
  ps.clear();
  ps.regs[0] = input;

  static void *jumptable[NUM_INSTR] = {
    [NOP]   = && INSN_NOP,
    [ADDXY] = && INSN_ADDXY,
    [MOVXC] = && INSN_MOVXC,
    [RETX] = && INSN_RETX,
    [RETC] = && INSN_RETC,
    [JMP] = && INSN_JMP,
    [JMPEQ] = && INSN_JMPEQ,
    [JMPGT] = && INSN_JMPGT,
    [JMPGE] = && INSN_JMPGE,
    [JMPLT] = && INSN_JMPLT,
    [JMPLE] = && INSN_JMPLE,
    [MAXC] = && INSN_MAXC,
    [MAXX] = && INSN_MAXX,
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

z3::expr inst_t::smt_inst(smt_var& sv) const {
  z3::expr curDst = string_to_expr("false");
  z3::expr curSrc = string_to_expr("false");
  z3::expr newDst = string_to_expr("false");
  z3::expr imm = string_to_expr("false");
  switch (_opcode) {
    case MAXX:
    case ADDXY:
      curDst = CURDST;
      curSrc = CURSRC;
      newDst = NEWDST;
      break;
    case MOVXC:
      imm = IMM2;
      newDst = NEWDST;
      break;
    case MAXC:
      curDst = CURDST;
      imm = IMM2;
      newDst = NEWDST;
      break;
  }
  switch (_opcode) {
    case ADDXY: return predicate_add(curDst, curSrc, newDst);
    case MOVXC: return predicate_mov(imm, newDst);
    case MAXC: return predicate_max(curDst, imm, newDst);
    case MAXX: return predicate_max(curDst, curSrc, newDst);
    default: return string_to_expr("false");
  }
}

z3::expr inst_t::smt_inst_jmp(smt_var& sv) const {
  switch (_opcode) {
    case JMPEQ: return (CURDST == CURSRC);
    case JMPGT: return (CURDST > CURSRC);
    case JMPGE: return (CURDST >= CURSRC);
    case JMPLT: return (CURDST < CURSRC);
    case JMPLE: return (CURDST <= CURSRC);
    default: return string_to_expr("false");
  }
}
