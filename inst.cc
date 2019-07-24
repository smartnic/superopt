#include <iostream>
#include "inst.h"

using namespace std;

void prog_state::print() {
  for (int i = 0; i < NUM_REGS; i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
}

void prog_state::clear() {
  pc = 0;
  for (int i = 0; i < NUM_REGS; i++) {
    regs[i] = 0;
  }
}

string inst::opcode_to_str(int opcode) {
  switch(opcode) {
    case NOP: return "NOP";
    case ADDXY: return "ADDXY";
    case MOVXC: return "MOVXC";
    case RETX: return "RETX";
    case RETC: return "RETC";
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

void inst::print() {
  cout << opcode_to_str(_opcode);
  for (int i=0; i < num_operands[_opcode]; i++) {
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

size_t instHash::operator()(const inst &x) const {
  return hash<int>()(x._opcode) ^
      (hash<int>()(x._args[0]) << 1) ^
      (hash<int>()(x._args[1]) << 2) ^
      (hash<int>()(x._args[2]) << 3);
}

void print_program(inst* program, int length) {
  for (int i = 0; i < length; i++) {
    cout << i << ": ";
    program[i].print();
  }
  cout << endl;
}

int interpret(inst *program, int length, prog_state &ps, int input) {
  /* Input currently is just one integer which will be written into R1. Will
  need to generalize this later. */
  inst *insn = program;
  ps.clear();
  ps.regs[0] = input;

  static void *jumptable[256] = {
    [NOP]   = &&INSN_NOP,
    [ADDXY] = &&INSN_ADDXY,
    [MOVXC] = &&INSN_MOVXC,
    [RETX] = &&INSN_RETX,
    [RETC] = &&INSN_RETC,
    [JMPEQ] = &&INSN_JMPEQ,
    [JMPGT] = &&INSN_JMPGT,
    [JMPGE] = &&INSN_JMPGE,
    [JMPLT] = &&INSN_JMPLT,
    [JMPLE] = &&INSN_JMPLE,
    [MAXC] = &&INSN_MAXC,
    [MAXX] = &&INSN_MAXX,
    [NUM_INSTR ... 255] = &&error_label,
  };

#define CONT { \
      insn++;                                                           \
      if (insn < program + length) {                                    \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }
#define DST ps.regs[insn->_args[0]-1]
#define SRC ps.regs[insn->_args[1]-1]
#define IMM1 insn->_args[0]
#define IMM2 insn->_args[1]

select_insn:
  goto *jumptable[insn->_opcode];

INSN_NOP:
  CONT;

INSN_ADDXY:
  DST = DST + SRC;
  CONT;

INSN_MOVXC:
  DST = IMM2;
  CONT;

INSN_RETX:
  return DST;

INSN_RETC:
  return IMM1;

INSN_MAXC:
  DST = max(DST, IMM2);
  CONT;

INSN_MAXX:
  DST = max(DST, SRC);
  CONT;

#define JMP(SUFFIX, OP)                         \
  INSN_JMP##SUFFIX:                             \
      if (DST OP SRC)                           \
        insn += insn->_args[2];                 \
  CONT;

  JMP(EQ, ==)
  JMP(GT, >)
  JMP(GE, >=)
  JMP(LT, <)
  JMP(LE, <=)

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return -1;

out:
  cout << "Error: program terminated without a return instruction" << endl;
  return -2; /* Terminate without return */
}

