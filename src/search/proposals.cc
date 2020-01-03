#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include <cassert>
#include <unordered_set>
#include "proposals.h"

using namespace std;

#define SEL_INST(prog) ((prog)->inst_list[sel_inst_index])

default_random_engine gen;
uniform_real_distribution<double> unidist(0.0, 1.0);

/* Return a uniformly random integer from start to end - 1 inclusive */
int sample_int(int start, int end) {
  int val;
  do {
    val = (int)(start + unidist(gen) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

/* Return a uniformly random integer from 0 to limit - 1 inclusive */
int sample_int(int limit) {
  return sample_int(0, limit);
}

/* Return a uniformly random integer from 0 to limit - 1 inclusive, with the
 * exceptions of  `excepts`. */
int sample_int_with_exceptions(int limit, unordered_set<int> &excepts) {
  int val = sample_int(limit - excepts.size());
  while (true) {
    if (excepts.find(val) == excepts.end()) {
      break;
    }
    val++;
  }
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

int get_new_operand(int sel_inst_index, const inst& sel_inst, int op_to_change, int old_opvalue) {
  int max_opvalue = sel_inst.get_max_operand_val(op_to_change, sel_inst_index);
  // TODO: is it wise to sample with exception?
  int new_opvalue = sample_int_with_exception(max_opvalue, old_opvalue);
  return new_opvalue;
}

void mod_operand(const prog &orig, prog* synth, int sel_inst_index, int op_to_change) {
  assert (op_to_change < orig.get_max_op_len());
  assert(sel_inst_index < orig.get_max_prog_len());
  // First make a fresh copy of the program.
  // inst* sel_inst = &synth->inst_list[sel_inst_index];
  int old_opvalue = SEL_INST(synth).get_operand(op_to_change);
  int new_opvalue = get_new_operand(sel_inst_index, SEL_INST(synth), op_to_change, old_opvalue);
  SEL_INST(synth).set_operand(op_to_change, new_opvalue);
}

void mod_random_operand(const prog &orig, prog* synth, int inst_index) {
  int op_to_change = sample_int(orig.inst_list[inst_index].get_num_operands());
  mod_operand(orig, synth, inst_index, op_to_change);
}

prog* mod_random_inst_operand(const prog &orig) {
  int inst_index = sample_int(orig.get_max_prog_len());
  prog* synth = prog::make_prog(orig);
  mod_random_operand(orig, synth, inst_index);
  return synth;
}

void mod_select_inst(prog *orig, unsigned int sel_inst_index) {
  const int MAX_PROG_LEN = orig->get_max_prog_len();
  assert(sel_inst_index < MAX_PROG_LEN);
  // TODO: is it wise to sample with exception?
  int old_opcode = SEL_INST(orig).get_opcode();
  // If sel_inst_index == MAX_PROG_LEN - 1, then new_opcode can not be JMP
  unordered_set<int> exceptions;
  if (sel_inst_index == MAX_PROG_LEN - 1) {
    exceptions = {old_opcode};
    orig->inst_list->insert_jmp_opcodes(exceptions);
  } else {
    exceptions = {old_opcode};
  }
  int new_opcode = sample_int_with_exceptions(orig->get_num_instr(), exceptions);
  SEL_INST(orig).set_opcode(new_opcode);
  for (int i = 0; i < SEL_INST(orig).get_num_operands(); i++) {
    int new_opvalue = get_new_operand(sel_inst_index, SEL_INST(orig), i, -1);
    SEL_INST(orig).set_operand(i, new_opvalue);
  }
  for (int i = SEL_INST(orig).get_num_operands(); i < orig->get_max_op_len(); i++) {
    SEL_INST(orig).set_operand(i, 0);
  }
}

prog* mod_random_inst(const prog &orig) {
  // First make a copy of the old program
  prog* synth = prog::make_prog(orig);
  int inst_index = sample_int(orig.get_max_prog_len());
  mod_select_inst(synth, inst_index);
  return synth;
}

prog* mod_random_k_cont_insts(const prog &orig, unsigned int k) {
  const int MAX_PROG_LEN = orig.get_max_prog_len();
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
  int k = sample_int(start_k_value, orig.get_max_prog_len() + 1);
  return mod_random_k_cont_insts(orig, k);
}
