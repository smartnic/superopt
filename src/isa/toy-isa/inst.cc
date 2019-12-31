#include <iostream>
#include <cassert>
#include "../inst_codegen.h"
#include "inst.h"

using namespace std;

void prog_state::print() {
  for (int i = 0; i < toy_isa::NUM_REGS; i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
}

void prog_state::clear() {
  pc = 0;
  for (int i = 0; i < toy_isa::NUM_REGS; i++) {
    regs[i] = 0;
  }
}

string inst::opcode_to_str(int opcode) const {
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

abs_bv_inst inst::inst_to_abs_bv() const {
  int v = (_opcode << (OP_ABS_BIT_LEN * 3)) +
          (_args[0] << (OP_ABS_BIT_LEN * 2)) +
          (_args[1] << (OP_ABS_BIT_LEN * 1)) +
          (_args[2]);
  abs_bv_inst bv(v);
  return bv;
}

vector<int> inst::get_reg_list() const {
  vector<int> reg_list;
  for (int i = 0 ; i < num_regs[_opcode]; i++)
    reg_list.push_back(_args[i]);
  return reg_list;
}

void inst::print() const {
  cout << opcode_to_str(_opcode);
  for (int i = 0; i < num_operands[_opcode]; i++) {
    cout << " " << _args[i];
  }
  cout << endl;
}

bool inst::operator==(const inst &x) const {
  return ((_opcode  == x._opcode) &&
          (_args[0] == x._args[0]) &&
          (_args[1] == x._args[1]) &&
          (_args[2] == x._args[2]));
}

inst& inst::operator=(const inst &rhs) {
  _opcode = rhs._opcode;
  _args[0] = rhs._args[0];
  _args[1] = rhs._args[1];
  _args[2] = rhs._args[2];
  return *this;
}

int inst::get_max_operand_val(int op_index, int inst_index) const {
  // max value for each operand type
  int max_val[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = toy_isa::NUM_REGS,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = toy_isa::MAX_PROG_LEN - inst_index - 1,
  };
  return max_val[OPTYPE(_opcode, op_index)];
}

int inst::get_operand(int op_index) const {
  assert(op_index < MAX_OP_LEN);
  return _args[op_index];
}

void inst::set_operand(int op_index, int op_value) {
  assert(op_index < MAX_OP_LEN);
  _args[op_index] = op_value;
}

int inst::get_opcode() const {
  return _opcode;
}

void inst::set_opcode(int op_value) {
  assert(op_value < NUM_INSTR);
  _opcode = op_value;
}

int inst::get_jmp_dis() const {
  switch (opcode_type[_opcode]) {
    case (OP_UNCOND_JMP): return _args[0];
    case (OP_COND_JMP): return _args[2];
    default: cout << "Error: opcode is not jmp" << endl; return 0;
  }
}

ostream& operator<<(ostream& out, abs_bv_inst& bv) {
  for (size_t i = 0; i < bv.size(); i++)
    out << bv[i];
  return out;
}

size_t instHash::operator()(const inst &x) const {
  return hash<int>()(x._opcode) ^
         (hash<int>()(x._args[0]) << 1) ^
         (hash<int>()(x._args[1]) << 2) ^
         (hash<int>()(x._args[2]) << 3);
}

void print_program(const inst* program, int length) {
  for (int i = 0; i < length; i++) {
    cout << i << ": ";
    program[i].print();
  }
  cout << endl;
}

void print_program(const vector<inst>& program) {
  for (int i = 0; i < program.size(); i++) {
    cout << i << ": ";
    program[i].print();
  }
  cout << endl;
}

int interpret(inst *program, int length, prog_state &ps, int input) {
  /* Input currently is just one integer which will be written into R0. Will
  need to generalize this later. */
  inst *insn = program;
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

#define CONT { \
      insn++;                                                           \
      if (insn < program + length) {                                    \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }
#define DST ps.regs[DSTREG(*insn)]
#define SRC ps.regs[SRCREG(*insn)]
#define IMM1 IMM1VAL(*insn)
#define IMM2 IMM2VAL(*insn)

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

int num_real_instructions(inst* program, int len) {
  int count = 0;
  for (int i = 0; i < len; i++) {
    if (program[i]._opcode != NOP) count++;
  }
  return count;
}

int inst_output_opcode_type(const inst& inst_end) {
  switch (inst_end._opcode) {
    case RETX:
      return RET_X;
    case RETC:
      return RET_C;
    default: // no RET, return register 0
      return RET_X;
  }
}

int inst_output(const inst& inst_end) {
  switch (inst_end._opcode) {
    case RETX:
      return DSTREG(inst_end);
    case RETC:
      return IMM1VAL(inst_end);
    default: // no RET, return register 0
      return 0;
  }
}
