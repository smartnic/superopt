#include <iostream>
#include <vector>
#include <string>
#include "validator.h"

using namespace z3;

/* class validator start */
validator::validator() {
  _last_counterex_mem.init_mem_by_layout();
}

validator::validator(inst* orig, int length) {
  set_orig(orig, length);
  _last_counterex_mem.init_mem_by_layout();
}

validator::validator(expr fx, expr input, expr output) {
  set_orig(fx, input, output);
}

validator::~validator() {}

void validator::gen_counterex(model& m, vector<expr>& op_pc_synth,
                              vector<smt_var>& op_mem_synth) {
  expr input_orig = string_to_expr("input");
  expr output_orig = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  _last_counterex.set_in_out((reg_t)m.eval(input_orig).get_numeral_uint64(), \
                             (reg_t)m.eval(output_orig).get_numeral_uint64());
  counterex_2_input_mem(_last_counterex_mem, m, _op_pc_orig, _op_mem_orig,
                        op_pc_synth, op_mem_synth);
}

int validator::is_smt_valid(expr& smt, model& mdl) {
  // Compared to the default tactic, 'bv' tactic is faster
  // for z3 check when processing bit vector
  tactic t = tactic(smt_c, "bv");
  solver s = t.mk_solver();
  s.add(!smt);
  switch (s.check()) {
    case unsat: return 1;
    case sat: {
      mdl = s.get_model();
      return 0;
    }
    case unknown: return -1;
  }
}

// assgin input r0 "input", other registers 0
// for eBPF, r10=bottom_of_stack
// todo: move pre constrain setting as a call from class inst
void validator::smt_pre(expr& pre, unsigned int prog_id, unsigned int num_regs,
                        unsigned int input_reg) {
  smt_var sv(prog_id, 0, num_regs);
  expr input = string_to_expr("input");
  // TODO: set input limit, need to be generalized
  expr p = (input >= -1024) && (input <= 1024);
  p = p && inst::smt_set_pre(input, sv);
  pre = p;
}

void validator::smt_pre(expr& pre, expr e) {
  pre = (e == string_to_expr("input"));
}

void validator::smt_post(expr& pst, unsigned int prog_id1, unsigned int prog_id2,
                         vector<expr>& op_pc_synth, vector<smt_var>& op_mem_synth) {
  pst = (string_to_expr("output" + to_string(prog_id1)) == \
         string_to_expr("output" + to_string(prog_id2))) &&
        smt_pgm_mem_eq_chk(_op_pc_orig, _op_mem_orig, op_pc_synth, op_mem_synth);
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(inst* orig, int length) {
  smt_pre(_pre_orig, VLD_PROG_ID_ORIG, NUM_REGS, orig->get_input_reg());
  smt_prog ps_orig;
  try {
    _pl_orig = ps_orig.gen_smt(VLD_PROG_ID_ORIG, orig, length);
  } catch (const string err_msg) {
    throw (err_msg);
    return;
  }
  ps_orig.get_output_pc_mem(_op_pc_orig, _op_mem_orig);
  _store_ps_orig = ps_orig; // store
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(expr fx, expr input, expr output) {
  smt_pre(_pre_orig, input);
  _pl_orig = fx && (string_to_expr("output" + to_string(VLD_PROG_ID_ORIG)) == output);
  // no storing store_ps_orig here
}

int validator::is_equal_to(inst* synth, int length) {
  expr pre_synth = string_to_expr("true");
  smt_pre(pre_synth, VLD_PROG_ID_SYNTH, NUM_REGS, synth->get_input_reg());
  smt_prog ps_synth;
  expr pl_synth = string_to_expr("true");
  try {
    pl_synth = ps_synth.gen_smt(VLD_PROG_ID_SYNTH, synth, length);
  } catch (const string err_msg) {
    // TODO error program process; Now just return false
    // cerr << err_msg << endl;
    return -1;
  }
  vector<expr> op_pc_synth;
  vector<smt_var> op_mem_synth;
  ps_synth.get_output_pc_mem(op_pc_synth, op_mem_synth);
  expr pre_same_mem = smt_pgm_set_same_input(_op_pc_orig, _op_mem_orig,
                      op_pc_synth, op_mem_synth);
  expr post = string_to_expr("true");
  smt_post(post, VLD_PROG_ID_ORIG, VLD_PROG_ID_SYNTH, op_pc_synth, op_mem_synth);
  expr smt = implies(pre_same_mem && _pre_orig && pre_synth && _pl_orig && pl_synth, post);
  // store
  _store_post = post;
  _store_f = smt;
  model mdl(smt_c);
  int is_equal = is_smt_valid(smt, mdl);
  if (is_equal == 0) {
    gen_counterex(mdl, op_pc_synth, op_mem_synth);
  }
  return is_equal;
}

reg_t validator::get_orig_output(reg_t input, unsigned int num_regs, unsigned int input_reg) {
  expr input_logic = string_to_expr("false");
  smt_pre(input_logic, VLD_PROG_ID_ORIG, num_regs, input_reg);
  input_logic = (string_to_expr("input") == to_expr(input)) && input_logic;
  tactic t = tactic(smt_c, "bv");
  solver s = t.mk_solver();
  s.add(_pl_orig && input_logic);
  s.check();
  model m = s.get_model();
  expr output_expr = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  reg_t output = m.eval(output_expr).get_numeral_uint64();
  return output;
}
/* class validator end */
