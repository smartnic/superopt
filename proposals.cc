#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include "inst.h"

#define PDF_SUPPORT 40

using namespace std;

default_random_engine gen;
uniform_real_distribution<double> unidist(0.0, 1.0);

/* Return a uniformly random integer from 0 to limit - 1 inclusive */
int sample_int(int limit) {
  int val;
  do {
    val = (int)(unidist(gen) * (double)limit);
  } while (val == limit && limit > 0);
  return val;
}

inst* mod_random_operand(inst* program, int inst_index, int prog_length) {
  // Number of possibilities for each operand type
  int num_poss[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = NUM_REGS,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = prog_length,
  };
  // TODO: Make a copy of this program for later reference, rather than update
  // in place
  inst* sel_inst = &program[inst_index];
  int sel_opcode = sel_inst->_opcode;
  int op_to_change = sample_int(num_operands[sel_opcode]) + 1;
  int optype = OPTYPE(sel_opcode, op_to_change);
  int new_opvalue = sample_int(num_poss[optype]);
  cout << "operand " << op_to_change << " of type " <<
      optype << " to new value " << new_opvalue << endl;
  switch(op_to_change) {
    case 1: sel_inst->_arg1 = new_opvalue; break;
    case 2: sel_inst->_arg2 = new_opvalue; break;
    case 3: sel_inst->_jmp_off = new_opvalue; break;
    default: cout << "Error setting new operand" << endl; return NULL;
  }
  return program;
}

inst* mod_random_inst_operand(inst* program, int prog_length) {
  int inst_index = sample_int(prog_length);
  cout << "Changing instruction " << inst_index << " ";
  return mod_random_operand(program, inst_index, prog_length);
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
  print_program(instructions, N);
  inst* prog = instructions;
  inst* new_prog;
  for (int i = 0; i < 5; i++) {
    new_prog = mod_random_inst_operand(prog, N);
    print_program(new_prog, N);
    prog = new_prog;
  }
  return 0;
}
