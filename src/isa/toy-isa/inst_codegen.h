#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"

using namespace std;

/* APIs exposed to the externals start */
// return (out = in)
inline int64_t toy_isa_compute_mov(int64_t in, int64_t out = 0);
// return (out = in1 + in2)
inline int64_t toy_isa_compute_add(int64_t in1, int64_t in2, int64_t out = 0);
// return (out = max(in1, in2))
inline int64_t toy_isa_compute_max(int64_t in1, int64_t in2, int64_t out = 0);
// return (out == in)
inline z3::expr toy_isa_compute_mov(z3::expr in, z3::expr out);
// return (out == in1 + in2)
inline z3::expr toy_isa_compute_add(z3::expr in1, z3::expr in2, z3::expr out);
// return (out == max(in1, in2))
inline z3::expr toy_isa_compute_max(z3::expr in1, z3::expr in2, z3::expr out);
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
inline ret_t toy_isa_compute_##func_name(para1_t in, para2_t out) {                 \
  operation(in, out);                                                               \
  return out;                                                                       \
}

#undef COMPUTE_BINARY
#define COMPUTE_BINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)      \
inline ret_t toy_isa_compute_##func_name(para1_t in1, para2_t in2, para3_t out) {   \
  operation(in1, in2, out);                                                         \
  return out;                                                                       \
}

COMPUTE_UNARY(mov, MOV_EXPR, int64_t, int64_t, int64_t)
COMPUTE_BINARY(add, ADD_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(max, MAX_EXPR, int64_t, int64_t, int64_t, int64_t)
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
inline z3::expr toy_isa_predicate_##func_name(z3::expr in, z3::expr out) {                 \
  return operation(in, out);                                                               \
}
#undef PREDICATE_BINARY
#define PREDICATE_BINARY(func_name, operation)                                             \
inline z3::expr toy_isa_predicate_##func_name(z3::expr in1, z3::expr in2, z3::expr out) {  \
  return operation(in1, in2, out);                                                         \
}

PREDICATE_UNARY(mov, MOV_EXPR)
PREDICATE_BINARY(add, ADD_EXPR)
PREDICATE_BINARY(max, MAX_EXPR)
// Functions for validator end
/* Macros for validator end  */
