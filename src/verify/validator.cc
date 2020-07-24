#include <iostream>
#include <vector>
#include <string>
#include "validator.h"

using namespace z3;

int n_solve;
int n_is_equal_to;

/* class validator start */
validator::validator() {
  _last_counterex.input.init();
  _last_counterex.output.init();
}

validator::validator(inst* orig, int length) {
  set_orig(orig, length);
  _last_counterex.input.init();
  _last_counterex.output.init();
}

validator::validator(expr fx, expr input, expr output) {
  set_orig(fx, input, output);
  _last_counterex.input.init();
  _last_counterex.output.init();
}

validator::~validator() {}

void validator::gen_counterex(inst* orig, int length, model& m, smt_var& post_sv_synth) {
  expr input_orig = string_to_expr("input");
  expr output_orig = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  _last_counterex.clear();
  // func counterex_2_input_mem will clear input
  // TODO: update input.reg in counterex_2_input_mem(.)
  counterex_2_input_mem(_last_counterex.input, m, _post_sv_orig, post_sv_synth);
  _last_counterex.input.reg = (reg_t)m.eval(input_orig).get_numeral_uint64();
  // get output from interpreter
  prog_state ps;
  ps.init();
  _last_counterex.output.clear();
  // call interpret to get the output, if get output from mdl, the output needs to be computed
  // for different path conditions, which seems to cost more time than interpret(.)
  // TODO: for BPF, if return value is an address (ex, &map[k]), the output will be different
  // record in https://github.com/smartnic/superopt/issues/83
  interpret(_last_counterex.output, orig, length, ps, _last_counterex.input);
}

int validator::is_smt_valid(expr& smt, model& mdl) {
  // cout << "is_smt_valid" << endl;
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
                         smt_var& post_sv_synth) {
  pst = (string_to_expr("output" + to_string(prog_id1)) == \
         string_to_expr("output" + to_string(prog_id2))) &&
        smt_pgm_mem_eq_chk(_post_sv_orig, post_sv_synth);
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
  _post_sv_orig = ps_orig.sv;
  _store_ps_orig = ps_orig; // store
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(expr fx, expr input, expr output) {
  smt_pre(_pre_orig, input);
  _pl_orig = fx && (string_to_expr("output" + to_string(VLD_PROG_ID_ORIG)) == output);
  // no storing store_ps_orig here
}

int validator::is_equal_to(inst* orig, int length_orig, inst* synth, int length_syn) {
  n_is_equal_to++;
  expr pre_synth = string_to_expr("true");
  smt_pre(pre_synth, VLD_PROG_ID_SYNTH, NUM_REGS, synth->get_input_reg());
  smt_prog ps_synth;
  expr pl_synth = string_to_expr("true");
  try {
    pl_synth = ps_synth.gen_smt(VLD_PROG_ID_SYNTH, synth, length_syn);
  } catch (const string err_msg) {
    // TODO error program process; Now just return false
    // cerr << err_msg << endl;
    return -1;
  }
  n_solve++;
  smt_var post_sv_synth = ps_synth.sv;
  expr pre_mem_same_mem = smt_pgm_set_same_input(_post_sv_orig, post_sv_synth);
  expr post = string_to_expr("true");
  smt_post(post, VLD_PROG_ID_ORIG, VLD_PROG_ID_SYNTH, post_sv_synth);
  expr smt = implies(pre_mem_same_mem && _pre_orig && pre_synth && _pl_orig && pl_synth, post);
  // store
  _store_post = post;
  _store_f = smt;
  model mdl(smt_c);
  int is_equal = is_smt_valid(smt, mdl);
  if (is_equal == 0) {
    // cout << is_equal << endl;
    gen_counterex(orig, length_orig, mdl, post_sv_synth);
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
