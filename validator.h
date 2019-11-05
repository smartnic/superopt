#pragma once

#include "inst.h"
#include "inout.h"
#include "smt_prog.h"
#include "z3++.h"

using namespace z3;

/* Validator algorithm document: https://github.com/ngsrinivas/superopt/tree/master/doc */

/* Class validator is used to check whether a synthesis program is equal to the original program/function.
 * Steps to use validator
 * step 1. set the original:
 *   a. this step will compute and store the program-level pre-condition and program logic of the original 
 *      into `_pre_orig` and `_pl_orig`;
 *   b. there are two ways to set the original: set in constructors or funtion `set_orig`;
 *   c. function paramters can be either a program or a function.
 * step 2. call funtion `is_equal_to` to check whether a synthesis program is equal to the original one
 *   a. this step will compute the program-level pre-condition and program logic of the synthesis program and 
 *      the post condition of the synthesis and original program. Then generate the SMT to check equivalence.
 *   b. return value:
 *     1 (equal);
 *     0 (unequal) if unequal, a counter-example will be generated and stored in `_last_counterex`;
 *     -1 (synthesis is illegal, e.g, program with loop or goes to invalid instructions).
 *   c. paramters: a synthesis program
 * step 3. if the return value in step 2 is `0`, a counter-example can be extracted in `_last_counterex`
 */
#define VLD_PROG_ID_ORIG 0
#define VLD_PROG_ID_SYNTH 1

class validator {
 private:
  void gen_counterex(model& m);
  bool is_smt_valid(expr& smt);
  // set register 0 in basic block 0 as input[prog_id]
  void smt_pre(expr& pre, unsigned int prog_id);
  // set the input variable of FOL formula as input[prog_id]
  void smt_pre(expr& pre, expr e);
  // setting outputs of two programs are equal
  void smt_post(expr& pst, unsigned int prog_id1, unsigned int prog_id2);
 public:
  // pre_: input formula of program: setting register 0 in basic block 0 as input[prog_id]
  // or the input variable of FOL formula as input[prog_id]
  expr _pre_orig = string_to_expr("true");
  expr _pl_orig = string_to_expr("true");
  // last counterexample
  inout _last_counterex;
  /* store variables start */
  expr _store_pre_synth = string_to_expr("true");
  // ps_: program logic formula, including basic program logic
  // and the formula of capturing the output of the program in the variable output[prog_id]
  prog_smt _store_ps_orig;
  prog_smt _store_ps_synth;
  // two program's output formula of setting outputs of two programs are equal, 
  // i.e., output[VLD_PROG_ID_ORIG] == output[VLD_PROG_ID_SYNTH]
  expr _store_post = string_to_expr("true");
  // f = pre^pre2^p1^p2 -> post
  expr _store_f = string_to_expr("true");
  /* store variables end */
  validator();
  validator(inst* orig, int len);
  validator(expr fx, expr input, expr output);
  ~validator();
  // calculate and store pre_orig, ps_orign
  void set_orig(inst* orig, int len);
  // fx is the original FOL formula, input/output is the input/output variable of fx
  void set_orig(expr fx, expr input, expr output);
  // check whether synth is equal to orig
  // return 0: not equal; return 1: equal; return -1: synth is illegal
  int is_equal_to(inst* synth, int len);
};
