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

int interpret(inst *program, int length, prog_state &ps) {
  inst *insn = program;

  static void *jumptable[256] = {
    [ADDXY] = &&INSN_ADDXY,
    [MOVXC] = &&INSN_MOVXC,
    [RETX] = &&INSN_RETX,
    [RETC] = &&INSN_RETC,
    [4 ... 255] = &&error_label,
  };

#define CONT ({ ps.print(); insn++; if (insn < program + length) goto select_insn; })
#define DST ps.regs[insn->_arg1]
#define SRC ps.regs[insn->_arg2]
#define IMM1 insn->_arg1
#define IMM2 insn->_arg2
#define JMP_OFF ps.regs[insn->_jmp_off]

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

error_label:
  cout << "Error in processing instruction; unknown opcode" << endl;
  return -1;
}

int main() {
  inst instructions[6] = {inst(MOVXC, 1, 10), /* mov r1, 10 */
                          inst(MOVXC, 2, 4),  /* mov r2, 4  */
                          inst(ADDXY, 1, 2),  /* add r1, r2 */
                          inst(MOVXC, 3, 5),  /* mov r3, 5  */
                          inst(ADDXY, 1, 3),  /* add r1, r3 */
                          inst(RETX, 1),      /* ret r1     */
  };
  prog_state ps;

  cout << "Result of full interpretation: " << interpret(instructions, 6, ps) << endl;
  return 0;
}
