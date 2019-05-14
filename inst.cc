#include <iostream>

#define NUM_REGS 4

using namespace std;

class prog_state {
  int pc = 0; /* Assume only straight line code execution for now */
 public:
  int regs[NUM_REGS] = {}; /* assume only registers for now */
  void print();
};

void prog_state::print() {
  for (int i = 0; i < NUM_REGS; i++) {
    cout << "Register "  << i << " " << regs[i] << endl;
  }
}

class inst {
 public:
  int _opcode;
  int _arg1;
  int _arg2;
  int _jmp_off;
  inst(int opcode, int arg1, int arg2=0, int jmp_off=0) {
    _opcode  = opcode;
    _arg1    = arg1;
    _arg2    = arg2;
    _jmp_off = jmp_off;
  }
};

// Instruction opcodes
#define ADDXY 0
#define MOVXC 1
#define RETX 2
#define RETC 3
#define JMPEQ 4
#define JMPGT 5
#define JMPGE 6
#define JMPLT 7
#define JMPLE 8

int interpret(inst *program, int length, prog_state &ps) {
  inst *insn = program;

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
    [9 ... 255] = &&error_label,
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

  cout << "Result of full interpretation: " << interpret(instructions, N, ps) << endl;
  return 0;
}
