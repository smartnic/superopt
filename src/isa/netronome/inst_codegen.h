#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"
#include "inst_var.h"

using namespace std;

/* APIs exposed to the externals start */
// return (out = in)
inline uint32_t compute_mov(uint32_t in, uint32_t out = 0);
// return (out = max(in1, in2))
// inline uint32_t compute_max(uint32_t in1, uint32_t in2, uint32_t out = 0);

inline z3::expr predicate_mov(z3::expr in, z3::expr out);
// return (out == max(in1, in2))
// inline z3::expr predicate_max(z3::expr in1, z3::expr in2, z3::expr out);

// ALU ops
inline uint32_t compute_add(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_subtract(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_add16(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_add8(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_inv(uint32_t in, uint32_t out = 0);
inline uint32_t compute_and(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_inv_and(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_or(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_xor(uint32_t in1, uint32_t in2, uint32_t out = 0);
inline uint32_t compute_add_ternary(uint32_t in1, uint32_t in2, uint32_t in3, uint32_t out = 0);

inline z3::expr predicate_add(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_subtract(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_add16(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_add8(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_inv(z3::expr in, z3::expr out);
inline z3::expr predicate_and(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_inv_and(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_or(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_xor(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_add_ternary(z3::expr in1, z3::expr in2, z3::expr in3, z3::expr out);

// Operations for carry bit
inline uint32_t compute_carry(uint32_t in1, uint32_t in2, uint32_t out = 0) {
  return (UINT32_MAX - in1 < in2) ? 1 : 0;
}

// out == length 32 bitvector with numeric value 1 if there is a carry out, 0 if not
inline z3::expr predicate_carry(z3::expr in1, z3::expr in2, z3::expr out) {
  // 1. zero extend in1 and in2 by 1 bit (so they're both 33 bits long)
  // 2. add them
  // 3. extract the highest order bit from the sum
  // 4. zero extend the one bit by 31 zeros, so it's 32 bits long
  return out == zext((zext(in1, 1) + zext(in2, 1)).extract(32, 32), 31);
}

/* APIs exposed to the externals end */

/* Inputs in, out must be side-effect-free expressions. */
#undef MOV_EXPR
#define MOV_EXPR(in, out) (out EQ in)
#undef INV_EXPR
#define INV_EXPR(in, out) (out EQ ~(in))
/* Inputs in1, in2, out must be side-effect-free expressions. */

// ALU expressions
#undef ADD_EXPR
#define ADD_EXPR(in1, in2, out) (out EQ in1 + in2)
#undef SUBTRACT_EXPR
#define SUBTRACT_EXPR(in1, in2, out) (out EQ in1 - in2)
#undef ADD16_EXPR
#define ADD16_EXPR(in1, in2, out) (out EQ in1 + L16(in2))
#undef ADD8_EXPR
#define ADD8_EXPR(in1, in2, out) (out EQ in1 + L8(in2))
#undef AND_EXPR
#define AND_EXPR(in1, in2, out) (out EQ (in1 & in2))
#undef INV_AND_EXPR
#define INV_AND_EXPR(in1, in2, out) (out EQ (~(in1) & in2))
#undef OR_EXPR
#define OR_EXPR(in1, in2, out) (out EQ (in1 | in2))
#undef XOR_EXPR
#define XOR_EXPR(in1, in2, out) (out EQ (in1 ^ in2))

#define ADD_TERNARY_EXPR(in1, in2, in3, out) (out EQ in1 + in2 + in3)

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
#define COMPUTE_UNARY(func_name, operation)                                      \
inline uint32_t compute_##func_name(uint32_t in, uint32_t out) {                    \
  operation(in, out);                                                            \
  return out;                                                                    \
}

#undef COMPUTE_BINARY
#define COMPUTE_BINARY(func_name, operation)                                     \
inline uint32_t compute_##func_name(uint32_t in1, uint32_t in2, uint32_t out) {      \
  operation(in1, in2, out);                                                      \
  return out;                                                                    \
}

#undef COMPUTE_TERNARY
#define COMPUTE_TERNARY(func_name, operation)                                     \
inline uint32_t compute_##func_name(uint32_t in1, uint32_t in2, uint32_t in3, uint32_t out) {      \
  operation(in1, in2, in3, out);                                                      \
  return out;                                                                    \
}

COMPUTE_UNARY(mov, MOV_EXPR)
// COMPUTE_BINARY(max, MAX_EXPR)

// ALU operations
COMPUTE_BINARY(add, ADD_EXPR)
COMPUTE_BINARY(subtract, SUBTRACT_EXPR)
COMPUTE_BINARY(add16, ADD16_EXPR)
COMPUTE_BINARY(add8, ADD8_EXPR)
COMPUTE_UNARY(inv, INV_EXPR)
COMPUTE_BINARY(and, AND_EXPR)
COMPUTE_BINARY(inv_and, INV_AND_EXPR)
COMPUTE_BINARY(or, OR_EXPR)
COMPUTE_BINARY(xor, XOR_EXPR)
COMPUTE_TERNARY(add_ternary, ADD_TERNARY_EXPR)

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
#undef PREDICATE_TERNARY
#define PREDICATE_TERNARY(func_name, operation)                                             \
inline z3::expr predicate_##func_name(z3::expr in1, z3::expr in2, z3::expr in3, z3::expr out) {          \
  return operation(in1, in2, in3, out);                                                         \
}


PREDICATE_UNARY(mov, MOV_EXPR)
// PREDICATE_BINARY(max, MAX_EXPR)

// ALU predicates
PREDICATE_BINARY(add, ADD_EXPR)
PREDICATE_BINARY(subtract, SUBTRACT_EXPR)
PREDICATE_BINARY(add16, ADD16_EXPR)
PREDICATE_BINARY(add8, ADD8_EXPR)
PREDICATE_UNARY(inv, INV_EXPR)
PREDICATE_BINARY(and, AND_EXPR)
PREDICATE_BINARY(inv_and, INV_AND_EXPR)
PREDICATE_BINARY(or, OR_EXPR)
PREDICATE_BINARY(xor, XOR_EXPR)
PREDICATE_TERNARY(add_ternary, ADD_TERNARY_EXPR)

// Functions for validator en
/* Macros for validator end  */

inline z3::expr smt_pgm_set_same_input(vector<z3::expr>& pc1, vector<smt_var>& sv1,
                                       vector<z3::expr>& pc2, vector<smt_var>& sv2) {
  return Z3_true;
}

inline z3::expr smt_pgm_mem_eq_chk(vector<z3::expr>& pc1, vector<smt_var>& sv1,
                                   vector<z3::expr>& pc2, vector<smt_var>& sv2) {
  return Z3_true;
}

inline void counterex_2_input_mem(inout_t& input, z3::model& mdl,
                                  vector<z3::expr>& pc1, vector<smt_var>& sv1,
                                  vector<z3::expr>& pc2, vector<smt_var>& sv2) {}
