#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"

using namespace std;

// return (out = op in)
inline int64_t compute_mov(int64_t in, int64_t out = 0);
inline int64_t compute_mov32(int32_t in, int64_t out = 0);
inline int64_t compute_le16(int64_t in, int64_t out = 0);
inline int64_t compute_le32(int64_t in, int64_t out = 0);
inline int64_t compute_le64(int64_t in, int64_t out = 0);
inline int64_t compute_be16(int64_t in, int64_t out = 0);
inline int64_t compute_be32(int64_t in, int64_t out = 0);
inline int64_t compute_be64(int64_t in, int64_t out = 0);

// return (out = in1 op in2)
inline int64_t compute_add(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_add32(int32_t in1, int32_t in2, int64_t out = 0);
inline int64_t compute_lsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_lsh32(int32_t in1, int32_t in2, int64_t out = 0);
inline int64_t compute_rsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_rsh32(int32_t in1, int32_t in2, int64_t out = 0);
inline int64_t compute_arsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_arsh32(int32_t in1, int32_t in2, int64_t out = 0);

// return (out == op in)
inline z3::expr predicate_mov(z3::expr in, z3::expr out);
inline z3::expr predicate_mov32(z3::expr in, z3::expr out);
inline z3::expr predicate_le16(z3::expr in, z3::expr out);
inline z3::expr predicate_le32(z3::expr in, z3::expr out);
inline z3::expr predicate_le64(z3::expr in, z3::expr out);
inline z3::expr predicate_be16(z3::expr in, z3::expr out);
inline z3::expr predicate_be32(z3::expr in, z3::expr out);
inline z3::expr predicate_be64(z3::expr in, z3::expr out);
// return (out == in1 op in2)
inline z3::expr predicate_add(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_add32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_lsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_lsh32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_rsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_rsh32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_arsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_arsh32(z3::expr in1, z3::expr in2, z3::expr out);


/* Inputs in, out must be side-effect-free expressions. */
#undef MOV_EXPR
#define MOV_EXPR(in, out) (out EQ in)
/* Inputs in1, in2, out must be side-effect-free expressions. */
#undef ADD_EXPR
#undef LSH_EXPR
#undef RSH_EXPR
#undef ARSH_EXPR
#define ADD_EXPR(in1, in2, out) (out EQ in1 + in2)
#define LSH_EXPR(in1, in2, out) (out EQ LSH(in1, in2))
#define RSH_EXPR(in1, in2, out) (out EQ RSH(in1, in2))
#define ARSH_EXPR(in1, in2, out) (out EQ ARSH(in1, in2))
// Expression for 32-bit `compute` and `predicate`
// Inputs x, y are both z3 int of int32_t or both 32-bit bit vectors
// z is z3 int of int64_t
#undef BIT32_EXPR_FMT
#define BIT32_EXPR_FMT(out, operation) (out EQ CONCAT(H32_EXPR, L32_EXPR(operation)))
#undef ADD32_EXPR
#undef MOV32_EXPR
#undef LSH32_EXPR
#undef RSH32_EXPR
#undef ARSH32_EXPR
#define ADD32_EXPR(in1, in2, out) BIT32_EXPR_FMT(out, in1 + in2)
#define MOV32_EXPR(in, out) BIT32_EXPR_FMT(out, in)
#define LSH32_EXPR(in1, in2, out) BIT32_EXPR_FMT(out, LSH32(in1, in2))
#define RSH32_EXPR(in1, in2, out) BIT32_EXPR_FMT(out, RSH32(in1, in2))
#define ARSH32_EXPR(in1, in2, out) BIT32_EXPR_FMT(out, ARSH32(in1, in2))

/* Predicate expressions capture instructions like MAX which have different
 * results on a register based on the evaluation of a predicate. */
/* Inputs out, pred_if, pred_else must be side-effect-free. */
#undef PRED_UNARY_EXPR
#define PRED_UNARY_EXPR(out, pred_if, ret_if, ret_else) ({              \
    IF_PRED_ACTION(pred_if, ret_if, out)                                \
    CONNECTIFELSE                                                       \
    ELSE_PRED_ACTION(pred_if, ret_else, out);                           \
  })

#undef LE16_EXPR
#undef LE32_EXPR
#undef LE64_EXPR
#undef BE16_EXPR
#undef BE32_EXPR
#undef BE64_EXPR
#define LE16_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), a, SWAP_L16(a)))
#define LE32_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), a, SWAP_L32(a)))
#define LE64_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), a, SWAP_L64(a)))
#define BE16_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SWAP_L16(a), a))
#define BE32_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SWAP_L32(a), a))
#define BE64_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SWAP_L64(a), a))

#undef EQ
#define EQ =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var EQ expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var EQ expr
#undef CONCAT
#define CONCAT(hi, lo) (hi | lo)
#undef H32_EXPR // 32 bits with zero
#define H32_EXPR 0
#undef L32_EXPR
#define L32_EXPR(a) L32(a)
#undef LSH
#define LSH(a, b) (a << b)
#undef RSH
#define RSH(a, b) (a >> b)
#undef ARSH
#define ARSH(a, b) (a >> b)
#undef LSH32
#define LSH32(a, b) (a << b)
#undef RSH32
#define RSH32(a, b) (a >> b)
#undef ARSH32
#define ARSH32(a, b) (a >> b)
#define SWAP16(v) ((((uint16_t)(v) & 0xff00) >> 8) | \
                   (((uint16_t)(v) & 0x00ff) << 8) )
#define SWAP32(v) ((((uint32_t)(v) & 0xff000000) >> 24) | \
                   (((uint32_t)(v) & 0x00ff0000) >> 8 ) | \
                   (((uint32_t)(v) & 0x0000ff00) << 8 ) | \
                   (((uint32_t)(v) & 0x000000ff) << 24) )
#define SWAP64(v) ((((uint64_t)(v) & 0xff00000000000000) >> 56) | \
                   (((uint64_t)(v) & 0x00ff000000000000) >> 40) | \
                   (((uint64_t)(v) & 0x0000ff0000000000) >> 24) | \
                   (((uint64_t)(v) & 0x000000ff00000000) >> 8 ) | \
                   (((uint64_t)(v) & 0x00000000ff000000) << 8 ) | \
                   (((uint64_t)(v) & 0x0000000000ff0000) << 24) | \
                   (((uint64_t)(v) & 0x000000000000ff00) << 40) | \
                   (((uint64_t)(v) & 0x00000000000000ff) << 56) )

#define SWAP_L16(v) (H48(v) | SWAP16(v))
#define SWAP_L32(v) (H32(v) | SWAP32(v))
#define SWAP_L64(v) SWAP64(v)

#undef COMPUTE_UNARY
#define COMPUTE_UNARY(func_name, operation, para1_t, para2_t, ret_t)             \
inline ret_t compute_##func_name(para1_t in, para2_t out) {                      \
  operation(in, out);                                                            \
  return out;                                                                    \
}

#undef COMPUTE_BINARY
#define COMPUTE_BINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)   \
inline ret_t compute_##func_name(para1_t in1, para2_t in2, para3_t out) {        \
  operation(in1, in2, out);                                                      \
  return out;                                                                    \
}

#undef COMPUTE_UBINARY
#define COMPUTE_UBINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)  \
inline ret_t compute_##func_name(para1_t in1, para2_t in2, para3_t out) {        \
  operation((u##para1_t)in1, in2, out);                                          \
  return out;                                                                    \
}

COMPUTE_UNARY(mov, MOV_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(mov32, MOV32_EXPR, int32_t, int64_t, int64_t)
COMPUTE_UNARY(le16, LE16_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(le32, LE32_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(le64, LE64_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be16, BE16_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be32, BE32_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be64, BE64_EXPR, int64_t, int64_t, int64_t)

COMPUTE_BINARY(add, ADD_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(add32, ADD32_EXPR, int32_t, int32_t, int64_t, int64_t)
COMPUTE_BINARY(lsh, LSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(lsh32, LSH32_EXPR, int32_t, int32_t, int64_t, int64_t)
COMPUTE_UBINARY(rsh, RSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_UBINARY(rsh32, RSH32_EXPR, int32_t, int32_t, int64_t, int64_t)
COMPUTE_BINARY(arsh, ARSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(arsh32, ARSH32_EXPR, int32_t, int32_t, int64_t, int64_t)

#undef EQ
#define EQ ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var EQ expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var EQ expr))
#undef CONCAT
#define CONCAT(hi, lo) z3::bv2int(z3::concat(hi, lo), true)
#undef H32_EXPR // 32-bit bv with zero
#define H32_EXPR to_expr(0, 32)
// For arithmetic operators and bitwise operators, experssions of L32_EXPR are different.
// For arithmetic operators, need to convert z3 int expr into z3 32-bit bitvector expr
// For bitwise operators, do not need the conversion
#undef L32_EXPR
#define L32_EXPR(a) z3::int2bv(32, a)
#undef LSH
#define LSH(a, b) z3::bv2int(z3::shl(a, b), true)
#undef RSH
#define RSH(a, b) z3::bv2int(z3::lshr(a, b), true)
#undef ARSH
#define ARSH(a, b) z3::bv2int(z3::ashr(a, b), true)
#undef LSH32
#define LSH32(a, b) z3::shl(a, b)
#undef RSH32
#define RSH32(a, b) z3::lshr(a, b)
#undef ARSH32
#define ARSH32(a, b) z3::ashr(a, b)
#undef SWAP16
#undef SWAP32
#undef SWAP64
#define SWAP16(v) z3::concat(v.extract(7, 0), v.extract(15, 8))
#define SWAP32(v) z3::concat(SWAP16(v), z3::concat(v.extract(23, 16), v.extract(31, 24)))
#define SWAP_BV_H32(v) z3::concat(z3::concat(v.extract(39, 32), v.extract(47, 40)), \
                                  z3::concat(v.extract(55, 48), v.extract(63, 56)))
#define SWAP64(v) z3::concat(SWAP32(v), SWAP_BV_H32(v))
#undef SWAP_L16
#undef SWAP_L32
#undef SWAP_L64
#define SWAP_L16(v) z3::bv2int(z3::concat(z3::int2bv(64, v).extract(63, 16), SWAP16(z3::int2bv(64, v))), true)
#define SWAP_L32(v) z3::bv2int(z3::concat(z3::int2bv(64, v).extract(63, 32), SWAP32(z3::int2bv(64, v))), true)
#define SWAP_L64(v) z3::bv2int(SWAP64(z3::int2bv(64, v)), true)

#undef PREDICATE_UNARY
#define PREDICATE_UNARY(func_name, operation)                                      \
inline z3::expr predicate_##func_name(z3::expr in, z3::expr out) {                 \
  return operation(in, out);                                                       \
}

#undef PREDICATE_BINARY
#define PREDICATE_BINARY(func_name, operation)                                     \
inline z3::expr predicate_##func_name(z3::expr in1, z3::expr in2, z3::expr out) {  \
  return operation(in1, in2, out);                                                 \
}

PREDICATE_UNARY(mov, MOV_EXPR)
PREDICATE_UNARY(mov32, MOV32_EXPR)
PREDICATE_UNARY(le16, LE16_EXPR)
PREDICATE_UNARY(le32, LE32_EXPR)
PREDICATE_UNARY(le64, LE64_EXPR)
PREDICATE_UNARY(be16, BE16_EXPR)
PREDICATE_UNARY(be32, BE32_EXPR)
PREDICATE_UNARY(be64, BE64_EXPR)
PREDICATE_BINARY(add, ADD_EXPR)
PREDICATE_BINARY(add32, ADD32_EXPR)
PREDICATE_BINARY(lsh, LSH_EXPR)
PREDICATE_BINARY(rsh, RSH_EXPR)
PREDICATE_BINARY(arsh, ARSH_EXPR)

/* bitwise operators for 32-bit bitvector start */
#undef L32_EXPR
#define L32_EXPR(a) a

PREDICATE_BINARY(lsh32, LSH32_EXPR);
PREDICATE_BINARY(rsh32, RSH32_EXPR);
PREDICATE_BINARY(arsh32, ARSH32_EXPR);
/* bitwise operators for 32-bit bitvector end */
