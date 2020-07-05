#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <cmath>
#include <cassert>
#include <unordered_set>
#include "proposals.h"

using namespace std;

default_random_engine gen;
uniform_real_distribution<double> unidist(0.0, 1.0);

/* Return a uniformly random integer from start to end inclusive */
int sample_int(int start, int end) {
  end++;
  int val;
  do {
    val = start + (int)(unidist(gen) * (double)(end - start));
  } while (val == end && end > start);
  return val;
}

/* Return a uniformly random integer from 0 to limit inclusive */
int sample_int(int limit) {
  return sample_int(0, limit);
}

/* Return a uniformly random integer from 0 to limit inclusive, with the
 * exceptions of  `excepts`. */
int sample_int_with_exceptions(int limit, unordered_set<int> &excepts) {
  int val = sample_int(limit - excepts.size());
  for (auto it = excepts.begin(); it != excepts.end(); it++) {
    if (*it <= val) val++;
  }
  return val;
}

/* Return a uniformly random integer from start to end inclusive, with the
 * exception of  `except`. */
int sample_int_with_exception(int start, int end, int except) {
  end++;
  int val;
  do {
    val = start + (int)(unidist(gen) * (double)(end - start));
  } while ((val == end || val == except) && ((end - start) > 1));
  return val;
}

/* Return a uniformly random integer from 0 to limit inclusive, with the
 * exception of  `except`. */
int sample_int_with_exception(int limit, int except) {
  return sample_int_with_exception(0, limit, except);
}

// sample with exception `old_opvalue`
int get_new_operand(int sel_inst_index, const inst& sel_inst, int op_to_change, int old_opvalue) {
  int max_opvalue = sel_inst.get_max_operand_val(op_to_change, sel_inst_index);
  int min_opvalue = sel_inst.get_min_operand_val(op_to_change, sel_inst_index);
  // TODO: is it wise to sample with exception?
  int new_opvalue = sample_int_with_exception(min_opvalue, max_opvalue, old_opvalue);
  return new_opvalue;
}

// sample without exception
int get_new_operand(int sel_inst_index, const inst& sel_inst, int op_to_change) {
  int max_opvalue = sel_inst.get_max_operand_val(op_to_change, sel_inst_index);
  int min_opvalue = sel_inst.get_min_operand_val(op_to_change, sel_inst_index);
  int new_opvalue = sample_int(min_opvalue, max_opvalue);
  return new_opvalue;
}

void mod_operand(const prog &orig, prog* synth, int sel_inst_index, int op_to_change) {
  assert (op_to_change < MAX_OP_LEN);
  assert(sel_inst_index < inst::max_prog_len);
  // First make a fresh copy of the program.
  inst* sel_inst = &synth->inst_list[sel_inst_index];
  int old_opvalue = sel_inst->get_operand(op_to_change);
  int new_opvalue = get_new_operand(sel_inst_index, *sel_inst, op_to_change, old_opvalue);
  sel_inst->set_operand(op_to_change, new_opvalue);
}

void mod_random_operand(const prog &orig, prog* synth, int inst_index) {
  int num = orig.inst_list[inst_index].get_num_operands();
  if (num == 0) return;
  int op_to_change = sample_int(num - 1);
  mod_operand(orig, synth, inst_index, op_to_change);
}

prog* mod_random_inst_operand(const prog &orig, int win_start, int win_end) {
  assert(win_end < inst::max_prog_len);
  // TODO: remove instructions whithout valid operands, such as NOP, EXIT
  int inst_index = sample_int(win_start, win_end);
  prog* synth = new prog(orig);
  synth->reset_vals();
  mod_random_operand(orig, synth, inst_index);
  if (synth->inst_list[inst_index] == orig.inst_list[inst_index]) {
    synth->set_vals(orig);
  }
  return synth;
}

void mod_select_inst(prog *orig, unsigned int sel_inst_index) {
  assert(sel_inst_index < inst::max_prog_len);
  // TODO: is it wise to sample with exception?
  inst* sel_inst = &orig->inst_list[sel_inst_index];
  int old_opcode = sel_inst->get_opcode();
  // exceptions set is used to avoid jumps in the last line of the program
  unordered_set<int> exceptions;
  if (sel_inst_index == inst::max_prog_len - 1) {
    exceptions = {old_opcode};
    sel_inst->insert_jmp_opcodes(exceptions);
  } else {
    exceptions = {old_opcode};
  }
  int new_opcode_idx = sample_int_with_exceptions(NUM_INSTR - 1, exceptions);
  int new_opcode = sel_inst->get_opcode_by_idx(new_opcode_idx);
  sel_inst->set_as_nop_inst();
  sel_inst->set_opcode(new_opcode);
  for (int i = 0; i < sel_inst->get_num_operands(); i++) {
    int new_opvalue = get_new_operand(sel_inst_index, *sel_inst, i);
    sel_inst->set_operand(i, new_opvalue);
  }
}

prog* mod_random_inst(const prog &orig, int win_start, int win_end) {
  assert(win_end < inst::max_prog_len);
  // First make a copy of the old program
  prog* synth = new prog(orig);
  synth->reset_vals();
  int inst_index = sample_int(win_start, win_end);
  mod_select_inst(synth, inst_index);
  if (synth->inst_list[inst_index] == orig.inst_list[inst_index]) {
    synth->set_vals(orig);
  }
  return synth;
}

prog* mod_random_k_cont_insts(const prog &orig, unsigned int k, int win_start, int win_end) {
  assert(win_end < inst::max_prog_len);
  // If k is too big, modify all instructions of the program window
  if (win_start + k - 1 > win_end) k = win_end - win_start + 1;
  // First make a copy of the old program
  prog* synth = new prog(orig);
  synth->reset_vals();
  // Select a random start instruction
  int start_inst_index = sample_int(win_start, win_end - k + 1);
  for (int i = start_inst_index; i < start_inst_index + k; i++) {
    mod_select_inst(synth, i);
  }
  bool is_same_pgm = true;
  for (int i = start_inst_index; i < start_inst_index + k; i++) {
    if (!(synth->inst_list[i] == orig.inst_list[i])) {
      is_same_pgm = false;
      break;
    }
  }
  if (is_same_pgm) synth->set_vals(orig);
  return synth;
}

prog* mod_random_cont_insts(const prog &orig, int win_start, int win_end) {
  assert(win_end < inst::max_prog_len);
  int start_k_value = 2; // at least change two instructions
  int max_len = win_end - win_start + 1;
  int k = sample_int(start_k_value, max_len);
  return mod_random_k_cont_insts(orig, k);
}
