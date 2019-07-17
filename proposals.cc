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

inst* mod_operand(inst* program, inst* sel_inst, int op_to_change,
                  int prog_length) {
  // Number of possibilities for each operand type
  int num_poss[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = NUM_REGS,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = prog_length,
  };
  // TODO: Make a copy of this program for later reference, rather than update
  // in place
  int sel_opcode = sel_inst->_opcode;
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

inst* mod_random_operand(inst* program, int inst_index, int prog_length) {
  inst* sel_inst = &program[inst_index];
  int sel_opcode = sel_inst->_opcode;
  int op_to_change = sample_int(num_operands[sel_opcode]) + 1;
  return mod_operand(program, sel_inst, op_to_change, prog_length);
}

inst* mod_random_inst_operand(inst* program, int prog_length) {
  int inst_index = sample_int(prog_length);
  cout << "Changing instruction " << inst_index << " ";
  return mod_random_operand(program, inst_index, prog_length);
}
