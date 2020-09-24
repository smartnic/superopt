#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <random>
#include "validator.h"
#include "z3client.h"

using namespace z3;

default_random_engine gen_vld;
uniform_real_distribution<double> unidist_vld(0.0, 1.0);

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

void validator::gen_counterex(inst* orig, int length, model& m, smt_var& post_sv_synth, int counterex_type) {
  expr input_orig = string_to_expr("input");
  _last_counterex.clear();
  // func counterex_2_input_mem will clear input
  // TODO: update input.reg in counterex_2_input_mem(.)
  if (counterex_type == COUNTEREX_eq_check) {
    counterex_2_input_mem(_last_counterex.input, m, _post_sv_orig, post_sv_synth);
  } else if (counterex_type == COUNTEREX_safety_check) {
    counterex_2_input_mem(_last_counterex.input, m, post_sv_synth);
  } else {
    cout << "ERROR: no counterex_type matches" << endl;
  }
  expr input_orig_val = m.eval(input_orig);
  if (input_orig_val.is_numeral()) {
    _last_counterex.input.reg = (reg_t)input_orig_val.get_numeral_uint64();
  } else {  // mean Z3 does not care about this value
    _last_counterex.input.reg = (double)unidist_vld(gen_vld) * 0xffffffffffffffff;
  }
  // get output from interpreter
  prog_state ps;
  ps.init();
  _last_counterex.output.clear();
  // call interpret to get the output, if get output from mdl, the output needs to be computed
  // for different path conditions, which seems to cost more time than interpret(.)
  // TODO: for BPF, if return value is an address (ex, &map[k]), the output will be different
  // record in https://github.com/smartnic/superopt/issues/83
  try {
    interpret(_last_counterex.output, orig, length, ps, _last_counterex.input);
  } catch (const string err_msg) {
    // interpret throws error because of stack uninitialized read,
    // and for this case, only counterex's input is needed
  }
}

int validator::is_smt_valid(expr& smt, model& mdl) {
  // cout << "is_smt_valid" << endl;
  // Compared to the default tactic, 'bv' tactic is faster
  // for z3 check when processing bit vector
  tactic t = tactic(smt_c, "bv");
  solver s = t.mk_solver();
  s.add(!smt);
  // cout << "About to invoke z3client\n";
  string res = write_problem_to_z3server(s.to_smt2());
  // cout << "Received result from z3server: \n" << res << endl;
  if (res.compare("unsat") == 0)
    return 1;
  else if (res.compare("unknown") == 0)
    return -1;
  else if (res.compare(0, 1, "(") == 0) {
    /* We've received a serialized version of a Z3 model. Extract a
     * model object from this. */
    solver s1(smt_c);
    s1.from_string(res.c_str());
    auto is_sat = s1.check();
    assert (is_sat == z3::sat);
    mdl = s1.get_model();
    return 0;
  } else {
    cout << "z3 solver client received unexpected output: '"
         << res << "'\n";
    return -1;
  }
}

// assgin input r0 "input", other registers 0
// for eBPF, r10=bottom_of_stack
// todo: move pre constrain setting as a call from class inst
void validator::smt_pre(expr& pre, unsigned int prog_id, unsigned int num_regs,
                        unsigned int input_reg) {
  smt_var sv;
  sv.init(prog_id, 0, num_regs);
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
  pst = smt_pgm_eq_chk(_post_sv_orig, post_sv_synth);
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

  // reset _prog_eq_cache, _prog_uneq_cache, since original program is reset
  _prog_eq_cache.clear();
  _prog_uneq_cache.clear();
  _count_is_equal_to = 0;
  _count_throw_err = 0;
  _count_prog_eq_cache = 0;
  _count_solve_safety = 0;
  _count_solve_eq = 0;

  // safety check of the original program
  int sc = safety_check(orig, length, _pre_orig, _pl_orig, ps_orig.p_sc, _post_sv_orig);
  if (sc != 1) {
    string err_msg = "original program is unsafe";
    throw (err_msg);
  }
  // if the original program is safe, insert it in the prog_eq cache
  prog orig_prog(orig);
  canonicalize(orig_prog.inst_list, length);
  insert_into_prog_cache(orig_prog, _prog_eq_cache);
}

// calculate and store pre_orig, ps_orign
void validator::set_orig(expr fx, expr input, expr output) {
  smt_pre(_pre_orig, input);
  _pl_orig = fx && (string_to_expr("output" + to_string(VLD_PROG_ID_ORIG)) == output);
  // no storing store_ps_orig here
}

bool validator::is_in_prog_cache(prog& pgm, unordered_map<int, vector<prog*> >& prog_cache) {
  int ph = progHash()(pgm);
  if (prog_cache.find(ph) != prog_cache.end()) {
    vector<prog*> chain = prog_cache[ph];
    for (auto p : chain) {
      if (*p == pgm) {
        return true;
      }
    }
  }
  return false;
}

void validator::insert_into_prog_cache(prog& pgm, unordered_map<int, vector<prog*> >& prog_cache) {
  if (is_in_prog_cache(pgm, prog_cache)) return;

  int ph = progHash()(pgm);
  prog_cache[ph] = std::vector<prog*>();
  prog* pgm_copy = new prog(pgm);
  prog_cache[ph].push_back(pgm_copy);
}

int validator::safety_check(inst* orig, int len, expr& pre, expr& pl, expr& p_sc, smt_var& sv) {
  expr smt_safety_chk = implies(pre && pl, p_sc);
  model mdl_sc(smt_c);
  auto t1 = NOW;
  int is_safe = is_smt_valid(smt_safety_chk, mdl_sc);
  auto t2 = NOW;
  cout << "vld solve safety: " << DUR(t1, t2) << " us" << " " << is_safe << endl;
  if (is_safe == 0) {
    gen_counterex(orig, len, mdl_sc, sv, COUNTEREX_safety_check);
    return ILLEGAL_CEX;
  }
  return is_safe;
}

int validator::is_equal_to(inst* orig, int length_orig, inst* synth, int length_syn) {
  _count_is_equal_to++;
  expr pre_synth = string_to_expr("true");
  smt_pre(pre_synth, VLD_PROG_ID_SYNTH, NUM_REGS, synth->get_input_reg());
  smt_prog ps_synth;
  expr pl_synth = string_to_expr("true");
  try {
    pl_synth = ps_synth.gen_smt(VLD_PROG_ID_SYNTH, synth, length_syn);
  } catch (const string err_msg) {
    // TODO error program process; Now just return false
    // cerr << err_msg << endl;
    _count_throw_err++;
    return -1;
  }
  // check whether the synth is in the eq_prog_cache. If so, this synth is equal to the original
  prog synth_prog(synth);
  if (_enable_prog_eq_cache) {
    canonicalize(synth_prog.inst_list, length_syn);
    if (is_in_prog_cache(synth_prog, _prog_eq_cache)) {
      _count_prog_eq_cache++;
      cout << "vld synth eq from prog_eq_cache" << endl;
      return 1;
    }
  }

  if (_enable_prog_uneq_cache) {
    canonicalize(synth_prog.inst_list, length_syn);
    if (is_in_prog_cache(synth_prog, _prog_uneq_cache)) {
      cout << "ERROR: found the same unequal program again" << endl;
      return UNEQ_NOCEX;
    }
  }

  smt_var post_sv_synth = ps_synth.sv;

  int sc = safety_check(orig, length_orig, pre_synth, pl_synth, ps_synth.p_sc, post_sv_synth);
  if (sc != 1) {
    if ((sc == ILLEGAL_CEX) && _enable_prog_uneq_cache) {
      insert_into_prog_cache(synth_prog, _prog_uneq_cache);
    }
    return sc;
  }

  expr pre_mem_same_mem = smt_pgm_set_same_input(_post_sv_orig, post_sv_synth);
  expr post = string_to_expr("true");
  smt_post(post, VLD_PROG_ID_ORIG, VLD_PROG_ID_SYNTH, post_sv_synth);
  expr smt = implies(pre_mem_same_mem && _pre_orig && pre_synth && _pl_orig && pl_synth, post);
  // store
  _store_post = post;
  _store_f = smt;
  model mdl(smt_c);
  _count_solve_eq++;
  auto t1 = NOW;
  int is_equal = is_smt_valid(smt, mdl);
  auto t2 = NOW;
  cout << "validator solve eq: " << DUR(t1, t2) << " us" << " " << is_equal << endl;

  if (is_equal == 0) {
    // cout << is_equal << endl;
    // cout << mdl << endl;
    gen_counterex(orig, length_orig, mdl, post_sv_synth, COUNTEREX_eq_check);
  } else if ((is_equal == 1) && _enable_prog_eq_cache) {
    // insert the synth into eq_prog_cache if new
    insert_into_prog_cache(synth_prog, _prog_eq_cache);
  }

  if ((is_equal != 1) && _enable_prog_uneq_cache) {
    insert_into_prog_cache(synth_prog, _prog_uneq_cache);
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

void validator::print_counters() const {
  std::cout << "validator counters: "
            << "is_equal_to: " << _count_is_equal_to << ", "
            << "throw_err: " << _count_throw_err << ", "
            << "prog_eq_cache: " << _count_prog_eq_cache << ", "
            << "solve_safety: " << _count_solve_safety << ", "
            << "solve_eq: " << _count_solve_eq << endl;
}

/* class validator end */
