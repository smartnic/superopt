#include <iostream>
#include <vector>
#include <string>
#include "validator.h"

using namespace z3;

/* class validator start */
validator::validator() {
}

validator::validator(vector<inst*>& orig) {
  set_orig(orig);
}

validator::validator(expr fx, expr input, expr output) {
  set_orig(fx, input, output);
}

validator::~validator() {}

void validator::gen_counterex(model& m) {
  expr input_orig = string_to_expr("input");
  expr output_orig = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  _last_counterex.set_in_out((int64_t)m.eval(input_orig).get_numeral_uint64(), \
                             (int64_t)m.eval(output_orig).get_numeral_uint64());
}

bool validator::is_smt_valid(expr& smt) {
  tactic t = tactic(smt_c, "bv");
  solver s = t.mk_solver();
  s.add(!smt);
  switch (s.check()) {
    case unsat: return true;
    case sat: {
      model m = s.get_model();
      gen_counterex(m);
      return false;
    }
    case unknown: return false;
  }
  return false;
}

// assgin input r0 "input", other registers 0
void validator::smt_pre(expr& pre, unsigned int prog_id, unsigned int num_regs, unsigned int input_reg) {
  smt_var sv(prog_id, 0, num_regs);
  expr p = (sv.get_cur_reg_var(input_reg) == string_to_expr("input"));
  for (size_t i = 0; i < num_regs; i++) {
    if (i != input_reg) p = p and (sv.get_cur_reg_var(i) == 0);
  }
  pre = p;
}

void validator::smt_pre(expr& pre, expr e) {
  pre = (e == string_to_expr("input"));
}

void validator::smt_post(expr& pst, unsigned int prog_id1, unsigned int prog_id2) {
  pst = (string_to_expr("output" + to_string(prog_id1)) == \
         string_to_expr("output" + to_string(prog_id2)));
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(vector<inst*>& orig) {
  smt_pre(_pre_orig, VLD_PROG_ID_ORIG, orig[0]->get_num_regs(), orig[0]->get_input_reg());
  smt_prog ps_orig;
  try {
    _pl_orig = ps_orig.gen_smt(VLD_PROG_ID_ORIG, orig);
  } catch (const string err_msg) {
    throw (err_msg);
    return;
  }
  _store_ps_orig = ps_orig; // store
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(expr fx, expr input, expr output) {
  smt_pre(_pre_orig, input);
  _pl_orig = fx && (string_to_expr("output" + to_string(VLD_PROG_ID_ORIG)) == output);
  // no storing store_ps_orig here
}

int validator::is_equal_to(vector<inst*>& synth) {
  expr pre_synth = string_to_expr("true");
  smt_pre(pre_synth, VLD_PROG_ID_SYNTH, synth[0]->get_num_regs(), synth[0]->get_input_reg());
  smt_prog ps_synth;
  expr pl_synth = string_to_expr("true");
  try {
    pl_synth = ps_synth.gen_smt(VLD_PROG_ID_SYNTH, synth);
  } catch (const string err_msg) {
    // TODO error program process; Now just return false
    // cerr << err_msg << endl;
    return -1;
  }
  expr post = string_to_expr("true");
  smt_post(post, VLD_PROG_ID_ORIG, VLD_PROG_ID_SYNTH);
  expr smt = implies(_pre_orig && pre_synth && _pl_orig && pl_synth, post);
  // store
  _store_post = post;
  _store_f = smt;
  return (int)is_smt_valid(smt);
}

int validator::get_orig_output(int input, unsigned int num_regs, unsigned int input_reg) {
  smt_var sv(VLD_PROG_ID_ORIG, 0, num_regs);
  expr input_logic = (sv.get_init_reg_var(input_reg) == input);
  solver s(smt_c);
  s.add(_pl_orig && input_logic);
  s.check();
  model m = s.get_model();
  expr output_expr = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  int output = m.eval(output_expr).get_numeral_int();
  return output;
}
/* class validator end */
