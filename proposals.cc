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

/* Return a uniformly random integer from 0 to limit - 1 inclusive, with the
 * exception of  `except`. */
int sample_int_with_exception(int limit, int except) {
  int val;
  do {
    val = (int)(unidist(gen) * (double)limit);
  } while ((val == limit || val == except) && limit > 1);
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
  assert (op_to_change < 3);
  int sel_opcode = sel_inst->_opcode;
  int optype = OPTYPE(sel_opcode, op_to_change);
  int old_opvalue = sel_inst->_args[op_to_change];
  // TODO: is it wise to sample with exception?
  int new_opvalue = sample_int_with_exception(num_poss[optype], old_opvalue);
  cout << "operand " << op_to_change << " of type " <<
      optype << " to new value " << new_opvalue << endl;
  sel_inst->_args[op_to_change] = new_opvalue;
  return program;
}

inst* mod_random_operand(inst* program, int inst_index, int prog_length) {
  inst* sel_inst = &program[inst_index];
  int sel_opcode = sel_inst->_opcode;
  int op_to_change = sample_int(num_operands[sel_opcode]);
  return mod_operand(program, sel_inst, op_to_change, prog_length);
}

inst* mod_random_inst_operand(inst* program, int prog_length) {
  int inst_index = sample_int(prog_length);
  cout << "Changing instruction " << inst_index << " ";
  return mod_random_operand(program, inst_index, prog_length);
}

inst* mod_random_inst(inst* program, int prog_length) {
  int inst_index = sample_int(prog_length);
  inst* sel_inst = &program[inst_index];
  int old_opcode = sel_inst->_opcode;
  // TODO: is it wise to sample with exception?
  int new_opcode = sample_int_with_exception(NUM_INSTR, old_opcode);
  sel_inst->_opcode = new_opcode;
  inst* new_prog;
  cout << "Changing instruction " << inst_index << " to new opcode " <<
      new_opcode << " " << sel_inst->opcode_to_str(new_opcode) << " " << endl;
  for (int i = 0; i < num_operands[new_opcode]; i++) {
    new_prog = mod_operand(program, sel_inst, i, prog_length);
    program = new_prog;
    print_program(program, prog_length);
  }
  return program;
}
