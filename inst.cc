#include <iostream>
#include "inst.h"

using namespace std;

void prog_state::print() {
  for (int i = 0; i < NUM_REGS; i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
}

std::string inst::opcode_to_str() {
  switch(_opcode) {
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
  cout << opcode_to_str() << " " << _arg1 << " " << _arg2 <<
      " " << _jmp_off << endl;
}

void print_program(inst* program, int length) {
  for (int i = 0; i < length; i++) {
    program[i].print();
  }
  cout << endl;
}

int interpret(inst *program, int length, prog_state &ps) {
  inst *insn = program;

  /* The definitions below assume a minimum 16-bit integer data type */
#define FSTOP(x) (x << 10)
#define SNDOP(x) (x << 5)
#define TRDOP(x) (x)

#define JMP_OPS (FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_OFF))

  static int optable[256] = {
    [ADDXY] = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
    [MOVXC] = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
    [RETX]  = FSTOP(OP_REG) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [RETC]  = FSTOP(OP_IMM) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
    [JMPEQ] = JMP_OPS,
    [JMPGT] = JMP_OPS,
    [JMPGE] = JMP_OPS,
    [JMPLT] = JMP_OPS,
    [JMPLE] = JMP_OPS,
    [MAXC]  = FSTOP(OP_REG) | SNDOP(OP_IMM) | TRDOP(OP_UNUSED),
    [MAXX]  = FSTOP(OP_REG) | SNDOP(OP_REG) | TRDOP(OP_UNUSED),
    [11 ... 255] = FSTOP(OP_UNUSED) | SNDOP(OP_UNUSED) | TRDOP(OP_UNUSED),
  };

  static void *jumptable[256] = {
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
    [11 ... 255] = &&error_label,
  };

#define CONT { \
      insn++; ps.print();                                               \
      if (insn < program + length) {                                    \
        cout << "Executing insn with opcode " << insn->_opcode << endl; \
        goto select_insn;                                               \
      } else goto out;                                                  \
  }
#define DST ps.regs[insn->_arg1]
#define SRC ps.regs[insn->_arg2]
#define IMM1 insn->_arg1
#define IMM2 insn->_arg2

select_insn:
  goto *jumptable[insn->_opcode];

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
        insn += insn->_jmp_off;                 \
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

int main(int argc, char *argv[]) {
#define N 7
  /* Add the notion of program input */
  int input = 10;
  if (argc > 1) {
    input = atoi(argv[1]);
  }
  inst instructions[N] = {inst(MOVXC, 1, input), /* mov r1, input */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 15),  /* mov r3, 15  */
                          inst(JMPGT, 1, 3, 1),  /* if r1 <= r3: */
                          inst(RETX, 3),      /* ret r3 */
                          inst(RETX, 1),      /* else ret r1 */
  };
  prog_state ps;

  inst instructions2[5] = {inst(MOVXC, 1, input), /* mov r1, input */
                           inst(MOVXC, 2, 4),     /* mov r2, 4 */
                           inst(ADDXY, 1, 2),     /* add r1, r2 */
                           inst(MAXC, 1, 15),     /* max r1, 15 */
                           inst(RETX, 1),         /* ret r1 */
  };

  cout << "Result of full interpretation: " << endl;
  cout << interpret(instructions, N, ps) << endl;
  cout << "Program 2" << endl;
  cout << interpret(instructions2, 5, ps) << endl;
  return 0;
}
