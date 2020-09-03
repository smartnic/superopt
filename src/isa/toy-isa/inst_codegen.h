#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"
#include "inst_var.h"

using namespace std;

/* APIs exposed to the externals start */
// return (out = in)
inline int compute_mov(int in, int out = 0);
// return (out = in1 + in2)
inline int compute_add(int in1, int in2, int out = 0);
// return (out = max(in1, in2))
inline int compute_max(int in1, int in2, int out = 0);
// return (out == in)
inline z3::expr compute_mov(z3::expr in, z3::expr out);
// return (out == in1 + in2)
inline z3::expr compute_add(z3::expr in1, z3::expr in2, z3::expr out);
// return (out == max(in1, in2))
inline z3::expr compute_max(z3::expr in1, z3::expr in2, z3::expr out);
/* APIs exposed to the externals end */

/* Inputs in, out must be side-effect-free expressions. */
#undef MOV_EXPR
#define MOV_EXPR(in, out) (out EQ in)
/* Inputs in1, in2, out must be side-effect-free expressions. */
#undef ADD_EXPR
#define ADD_EXPR(in1, in2, out) (out EQ in1 + in2)

/* Predicate expressions capture instructions like MAX which have different
 * results on a register based on the evaluation of a predicate. */
/* Inputs out, pred_if, pred_else must be side-effect-free. */
#undef PRED_BINARY_EXPR
#define PRED_BINARY_EXPR(out, pred_if, ret_if, ret_else) ({      \
    IF_PRED_ACTION(pred_if, ret_if, out)                         \
    CONNECTIFELSE                                                \
    ELSE_PRED_ACTION(pred_if, ret_else, out);                    \
  })

/* Inputs in1, in2, out must be side-effect-free. */
#undef MAX_EXPR
#define MAX_EXPR(in1, in2, out) (PRED_BINARY_EXPR(out, in1 > in2, in1, in2))

/* Macros for interpreter start */
// Operator macros in experssion macros for interpreter start
#undef EQ
#define EQ =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var EQ expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var EQ expr
// Operator macros in experssion macros for interpreter end

// Functions for interpreter start
#undef COMPUTE_UNARY
#define COMPUTE_UNARY(func_name, operation, para1_t, para2_t, ret_t)                \
inline ret_t compute_##func_name(para1_t in, para2_t out) {                         \
  operation(in, out);                                                               \
  return out;                                                                       \
}

#undef COMPUTE_BINARY
#define COMPUTE_BINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)      \
inline ret_t compute_##func_name(para1_t in1, para2_t in2, para3_t out) {           \
  operation(in1, in2, out);                                                         \
  return out;                                                                       \
}

COMPUTE_UNARY(mov, MOV_EXPR, int, int, int)
COMPUTE_BINARY(add, ADD_EXPR, int, int, int, int)
COMPUTE_BINARY(max, MAX_EXPR, int, int, int, int)
// Functions for interpreter end
/* Macros for interpreter end */

/* Macros for validator start */
// Operator macros in experssion macros for validator start
#undef EQ
#define EQ ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var EQ expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var EQ expr))
// Operator macros in experssion macros for validator end

// Functions for validator start
#undef PREDICATE_UNARY
#define PREDICATE_UNARY(func_name, operation)                                              \
inline z3::expr predicate_##func_name(z3::expr in, z3::expr out) {                         \
  return operation(in, out);                                                               \
}
#undef PREDICATE_BINARY
#define PREDICATE_BINARY(func_name, operation)                                             \
inline z3::expr predicate_##func_name(z3::expr in1, z3::expr in2, z3::expr out) {          \
  return operation(in1, in2, out);                                                         \
}

PREDICATE_UNARY(mov, MOV_EXPR)
PREDICATE_BINARY(add, ADD_EXPR)
PREDICATE_BINARY(max, MAX_EXPR)

// Functions for validator en
/* Macros for validator end  */
inline z3::expr smt_pgm_eq_chk(smt_var& sv1, smt_var& sv2) {
    return (sv1.ret_val == sv2.ret_val);
}

inline z3::expr smt_pgm_set_same_input(smt_var& sv1, smt_var& sv2) {
  return Z3_true;
}

inline void counterex_2_input_mem(inout_t& input, z3::model& mdl,
                                  smt_var& sv1, smt_var& sv2) {}
