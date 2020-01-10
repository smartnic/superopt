#include <iostream>
#include "inst_codegen.h"

using namespace std;

/* Inputs x, y, z must be side-effect-free expressions. */
#define ADDXY_EXPR(x, y, z) (z GENMODE x + y)
#define MOV_EXPR(x, y) (y GENMODE x)

/* Predicate expressions capture instructions like MAX which have different
 * results on a register based on the evaluation of a predicate. */
/* Inputs x, y, z, pred_if, pred_else must be side-effect-free. */
#define PRED_EXPR(x, y, z, pred_if, ret_if, ret_else) ({  \
    IF_PRED_ACTION(pred_if, ret_if, z)                    \
    CONNECTIFELSE                                         \
    ELSE_PRED_ACTION(pred_if, ret_else, z);           \
  })

#define MAXX_EXPR(a, b, c) (PRED_EXPR(a, b, c, a > b, a, b))

#undef GENMODE
#define GENMODE =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var GENMODE expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var GENMODE expr

int compute_add(int a, int b, int c) {
  ADDXY_EXPR(a, b, c);
  return c;
}

int compute_mov(int a, int b) {
  MOV_EXPR(a, b);
  return b;
}

int compute_max(int a, int b, int c) {
  MAXX_EXPR(a, b, c);
  return c;
}

#undef GENMODE
#define GENMODE ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var GENMODE expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var GENMODE expr))

z3::expr predicate_add(z3::expr a, z3::expr b, z3::expr c) {
  return ADDXY_EXPR(a, b, c);
}

z3::expr predicate_mov(int a, z3::expr b) {
  return MOV_EXPR(a, b);
}

z3::expr predicate_max(z3::expr a, int b, z3::expr c) {
  return MAXX_EXPR(a, b, c);
}

z3::expr predicate_max(z3::expr a, z3::expr b, z3::expr c) {
  return MAXX_EXPR(a, b, c);
}
