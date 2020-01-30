#include <iostream>
#include "inst_codegen.h"

using namespace std;

/* Inputs x, y must be side-effect-free expressions. */
#define MOV_EXPR(x, y) (y EQ x)
/* Inputs x, y, z must be side-effect-free expressions. */
#define ADD_EXPR(x, y, z) (z EQ x + y)

/* Predicate expressions capture instructions like MAX which have different
 * results on a register based on the evaluation of a predicate. */
/* Inputs x, y, z, pred_if, pred_else must be side-effect-free. */
#define PRED_BINARY_EXPR(x, y, z, pred_if, ret_if, ret_else) ({  \
    IF_PRED_ACTION(pred_if, ret_if, z)                           \
    CONNECTIFELSE                                                \
    ELSE_PRED_ACTION(pred_if, ret_else, z);                      \
  })

#define MAX_EXPR(a, b, c) (PRED_BINARY_EXPR(a, b, c, a > b, a, b))

#undef EQ
#define EQ =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var EQ expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var EQ expr

#define COMPUTE_UNARY(func_name, operation, para1_t, para2_t, ret_t)             \
ret_t toy_isa_compute_##func_name(para1_t a, para2_t b) {                        \
  operation(a, b);                                                               \
  return b;                                                                      \
}

#define COMPUTE_BINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)   \
ret_t toy_isa_compute_##func_name(para1_t a, para2_t b, para3_t c) {             \
  operation(a, b, c);                                                            \
  return c;                                                                      \
}

COMPUTE_UNARY(mov, MOV_EXPR, int32_t, int64_t, int64_t)
COMPUTE_BINARY(add, ADD_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(max, MAX_EXPR, int64_t, int64_t, int64_t, int64_t)

#undef EQ
#define EQ ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var EQ expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var EQ expr))

#define PREDICATE_UNARY(func_name, operation)                           \
z3::expr toy_isa_predicate_##func_name(z3::expr a, z3::expr b) {        \
  return operation(a, b);                                               \
}

#define PREDICATE_BINARY(func_name, operation)                               \
z3::expr toy_isa_predicate_##func_name(z3::expr a, z3::expr b, z3::expr c) { \
  return operation(a, b, c);                                                 \
}

PREDICATE_UNARY(mov, MOV_EXPR)
PREDICATE_BINARY(add, ADD_EXPR)
PREDICATE_BINARY(max, MAX_EXPR)
