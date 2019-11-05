#include <iostream>
#include <vector>
#include <string>
#include "validator.h"
#include "inst.h"
#include "z3++.h"

using namespace z3;

/* class validator start */
validator::validator() {
}

validator::validator(inst* orig, int len) {
  set_orig(orig, len);
}

validator::validator(expr fx, expr input, expr output) {
  set_orig(fx, input, output);
}

validator::~validator() {}

void validator::gen_counterex(model& m) {
  expr input_orig = string_to_expr("input");
  expr output_orig = string_to_expr("output" + to_string(VLD_PROG_ID_ORIG));
  _last_counterex.set_in_out(m.eval(input_orig).get_numeral_int(), \
                             m.eval(output_orig).get_numeral_int());
}

bool validator::is_smt_valid(expr& smt) {
  solver s(smt_prog_c);
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
void validator::smt_pre(expr& pre, unsigned int prog_id) {
  smt_var sv(prog_id, 0);
  expr p = (sv.get_cur_reg_var(0) == string_to_expr("input"));
  for (size_t i = 1; i < NUM_REGS; i++) {
    p = p and (sv.get_cur_reg_var(i) == 0);
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
void validator::set_orig(inst* orig, int len) {
  smt_pre(_pre_orig, VLD_PROG_ID_ORIG);
  smt_prog ps_orig;
  try {
    _pl_orig = ps_orig.gen_smt(VLD_PROG_ID_ORIG, orig, len);
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

int validator::is_equal_to(inst* synth, int len) {
  expr pre_synth = string_to_expr("true");
  smt_pre(pre_synth, VLD_PROG_ID_SYNTH);
  smt_prog ps_synth;
  expr pl_synth = string_to_expr("true");
  try {
    pl_synth = ps_synth.gen_smt(VLD_PROG_ID_SYNTH, synth, len);
  } catch (const string err_msg) {
    // TODO error program process; Now just return false
    // cerr << err_msg << endl;
    return -1;
  }
  expr post = string_to_expr("true");
  smt_post(post, VLD_PROG_ID_ORIG, VLD_PROG_ID_SYNTH);
  expr smt = implies(_pre_orig && pre_synth && _pl_orig && pl_synth, post);
  // store
  _store_pre_synth = pre_synth;
  _store_ps_synth = ps_synth;
  _store_post = post;
  _store_f = smt;
  return (int)is_smt_valid(smt);
}
/* class validator end */
