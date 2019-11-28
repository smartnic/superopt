#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include "proposals.h"

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

/* Return a uniformly random integer from start to end - 1 inclusive */
int sample_int(int start, int end) {
  int val;
  do {
    val = (int)(start + unidist(gen) * (double)(end - start));
  } while (val == end && end > start);
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

int get_new_operand(int opcode, int op_to_change, int old_opvalue) {
  // Number of possibilities for each operand type
  int num_poss[4] = {
    [OP_UNUSED] = 0,
    [OP_REG] = NUM_REGS,
    [OP_IMM] = MAX_CONST,
    [OP_OFF] = MAX_PROG_LEN,
  };
  int optype = OPTYPE(opcode, op_to_change);
  // TODO: is it wise to sample with exception?
  int new_opvalue = sample_int_with_exception(num_poss[optype], old_opvalue);
  return new_opvalue;
}

void mod_operand(const prog &orig, prog* synth, int sel_inst_index, int op_to_change) {
  assert (op_to_change < 3);
  assert(sel_inst_index < MAX_PROG_LEN);
  // First make a fresh copy of the program.
  inst* sel_inst = &synth->inst_list[sel_inst_index];
  int sel_opcode = sel_inst->_opcode;
  int old_opvalue = sel_inst->_args[op_to_change];
  int new_opvalue = get_new_operand(sel_opcode, op_to_change, old_opvalue);
  sel_inst->_args[op_to_change] = new_opvalue;
}

void mod_random_operand(const prog &orig, prog* synth, int inst_index) {
  inst sel_inst = orig.inst_list[inst_index];
  int sel_opcode = sel_inst._opcode;
  int op_to_change = sample_int(num_operands[sel_opcode]);
  mod_operand(orig, synth, inst_index, op_to_change);
}

prog* mod_random_inst_operand(const prog &orig) {
  int inst_index = sample_int(MAX_PROG_LEN);
  prog* synth = prog::make_prog(orig);
  mod_random_operand(orig, synth, inst_index);
  return synth;
}

void mod_select_inst(prog *orig, unsigned int sel_inst_index) {
  assert(sel_inst_index < MAX_PROG_LEN);
  // TODO: is it wise to sample with exception?
  inst* sel_inst = &orig->inst_list[sel_inst_index];
  int old_opcode = sel_inst->_opcode;
  int new_opcode = sample_int_with_exception(NUM_INSTR, old_opcode);
  sel_inst->_opcode = new_opcode;
  for (int i = 0; i < num_operands[new_opcode]; i++) {
    int new_opvalue = get_new_operand(new_opcode, i, -1);
    sel_inst->_args[i] = new_opvalue;
  }
  for (int i = num_operands[new_opcode]; i < MAX_OP_LEN; i++) {
    sel_inst->_args[i] = 0;
  }
}

prog* mod_random_inst(const prog &orig) {
  // First make a copy of the old program
  prog* synth = prog::make_prog(orig);
  int inst_index = sample_int(MAX_PROG_LEN);
  mod_select_inst(synth, inst_index);
  return synth;
}

prog* mod_random_k_cont_insts(const prog &orig, unsigned int k) {
  // If k is too big, modify all instructions of the original program
  if (k > MAX_PROG_LEN) k = MAX_PROG_LEN;
  // First make a copy of the old program
  prog* synth = prog::make_prog(orig);
  // Select a random start instruction
  int start_inst_index = sample_int(MAX_PROG_LEN - k + 1);
  for (int i = start_inst_index; i < start_inst_index + k; i++) {
    mod_select_inst(synth, i);
  }
  return synth;
}

prog* mod_random_cont_insts(const prog &orig) {
  int start_k_value = 2; // at least change two instructions
  int k = sample_int(start_k_value, MAX_PROG_LEN + 1);
  return mod_random_k_cont_insts(orig, k);
}
