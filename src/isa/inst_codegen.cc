#include <iostream>
#include "inst_codegen.h"

using namespace std;

/* Inputs x, y must be side-effect-free expressions. */
#define NEG_EXPR(x, y) (y GENMODE ~x)
#define MOV_EXPR(x, y) (y GENMODE x)
/* Inputs x, y, z must be side-effect-free expressions. */
#define ADD_EXPR(x, y, z) (z GENMODE x + y)
#define SUB_EXPR(x, y, z) (z GENMODE x - y)
#define MUL_EXPR(x, y, z) (z GENMODE x * y)
#define DIV_EXPR(x, y, z) (z GENMODE x / y)
#define OR_EXPR(x, y, z)  (z GENMODE x | y)
#define AND_EXPR(x, y, z) (z GENMODE x & y)
#define LSH_EXPR(x, y, z) (z GENMODE x << y)
#define RSH_EXPR(x, y, z) (z GENMODE RSH(x, y))
#define MOD_EXPR(x, y, z) (z GENMODE x % y)
#define XOR_EXPR(x, y, z) (z GENMODE x ^ y)
#define ARSH_EXPR(x, y, z) (z GENMODE ARSH(x, y))
// Expression for 32-bit `compute` and `predicate`
// Inputs x, y are both z3 int of int32_t or both 32-bit bit vectors
// z is z3 int of int64_t
#define BIT32_EXPR_FMT(z, operation) (z == CONCAT_MODE(H32_EXPR, L32_EXPR(operation)))
#define ADD32_EXPR(x, y, z) BIT32_EXPR_FMT(z, x + y)
#define MOV32_EXPR(x, y) BIT32_EXPR_FMT(y, x)
#define RSH32_EXPR(x, y, z) BIT32_EXPR_FMT(z, RSH32(x, y))
#define ARSH32_EXPR(x, y, z) BIT32_EXPR_FMT(z, ARSH32(x, y))

/* Predicate expressions capture instructions like MAX which have different
 * results on a register based on the evaluation of a predicate. */
/* Inputs x, y, pred_if, pred_else must be side-effect-free. */
#define PRED_UNARY_EXPR(x, y, pred_if, ret_if, ret_else) ({      \
    IF_PRED_ACTION(pred_if, ret_if, y)                           \
    CONNECTIFELSE                                                \
    ELSE_PRED_ACTION(pred_if, ret_else, y);                      \
  })
/* Inputs x, y, z, pred_if, pred_else must be side-effect-free. */
#define PRED_BINARY_EXPR(x, y, z, pred_if, ret_if, ret_else) ({  \
    IF_PRED_ACTION(pred_if, ret_if, z)                           \
    CONNECTIFELSE                                                \
    ELSE_PRED_ACTION(pred_if, ret_else, z);                      \
  })

#define LE16_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), a, SWAP_L16(a)))
#define LE32_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), a, SWAP_L32(a)))
#define LE64_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), a, SWAP_L64(a)))
#define BE16_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), SWAP_L16(a), a))
#define BE32_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), SWAP_L32(a), a))
#define BE64_EXPR(a, b) (PRED_UNARY_EXPR(a, b, is_little_endian(), SWAP_L64(a), a))

#define MAX_EXPR(a, b, c) (PRED_BINARY_EXPR(a, b, c, a > b, a, b))

#undef GENMODE
#define GENMODE =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var GENMODE expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var GENMODE expr
#undef RSH
#define RSH(a, b) (a >> b)
#undef ARSH
#define ARSH(a, b) (a >> b)
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

#define COMPUTE_UNARY(func_name, operation, para1_t, para2_t, ret_t)             \
ret_t compute_##func_name(para1_t a, para2_t b) {                                \
  operation(a, b);                                                               \
  return b;                                                                      \
}

#define COMPUTE_BINARY(func_name, operation, para1_t, para2_t, para3_t, ret_t)   \
ret_t compute_##func_name(para1_t a, para2_t b, para3_t c) {                     \
  operation(a, b, c);                                                            \
  return c;                                                                      \
}

#define COMPUTE_RSH(func_name, operation, para1_t, para2_t, para3_t, ret_t)      \
ret_t compute_##func_name(para1_t a, para2_t b, para3_t c) {                     \
  operation((u##para1_t)a, b, c);                                                \
  return c;                                                                      \
}

COMPUTE_UNARY(mov, MOV_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(mov, MOV_EXPR, int32_t, int32_t, int32_t)
COMPUTE_UNARY(le16, LE16_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(le32, LE32_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(le64, LE64_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be16, BE16_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be32, BE32_EXPR, int64_t, int64_t, int64_t)
COMPUTE_UNARY(be64, BE64_EXPR, int64_t, int64_t, int64_t)

COMPUTE_BINARY(add, ADD_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(add, ADD_EXPR, int32_t, int32_t, int32_t, int32_t)
COMPUTE_BINARY(lsh, LSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(lsh, LSH_EXPR, int32_t, int32_t, int32_t, int32_t)
COMPUTE_RSH(rsh, RSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_RSH(rsh, RSH_EXPR, int32_t, int32_t, int32_t, int32_t)
COMPUTE_BINARY(arsh, ARSH_EXPR, int64_t, int64_t, int64_t, int64_t)
COMPUTE_BINARY(arsh, ARSH_EXPR, int32_t, int32_t, int32_t, int32_t)
COMPUTE_BINARY(max, MAX_EXPR, int64_t, int64_t, int64_t, int64_t)

#undef GENMODE
#define GENMODE ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var GENMODE expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var GENMODE expr))
#undef CONCAT_MODE
#define CONCAT_MODE(hi, lo) z3::bv2int(z3::concat(hi, lo), true)
#undef H32_EXPR // 32-bit bv with zero
#define H32_EXPR to_expr(0, 32)
// For arithmetic operators and bitwise operators, experssions of L32_EXPR are different.
// For arithmetic operators, need to convert z3 int expr into z3 32-bit bitvector expr
// For bitwise operators, do not need the conversion
#undef L32_EXPR
#define L32_EXPR(a) z3::int2bv(32, a)
#undef RSH
#define RSH(a, b) z3::bv2int(z3::lshr(a, b), true)
#undef ARSH
#define ARSH(a, b) z3::bv2int(z3::ashr(a, b), true)
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

#define PREDICATE_UNARY(func_name, operation)                           \
z3::expr predicate_##func_name(z3::expr a, z3::expr b) {                \
  return operation(a, b);                                               \
}

#define PREDICATE_BINARY(func_name, operation)                          \
z3::expr predicate_##func_name(z3::expr a, z3::expr b, z3::expr c) {    \
  return operation(a, b, c);                                            \
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
PREDICATE_BINARY(rsh, RSH_EXPR)
PREDICATE_BINARY(arsh, ARSH_EXPR)
PREDICATE_BINARY(max, MAX_EXPR)

/* bitwise operators for 32-bit bitvector start */
#undef L32_EXPR
#define L32_EXPR(a) a

PREDICATE_BINARY(rsh32, RSH32_EXPR);
PREDICATE_BINARY(arsh32, ARSH32_EXPR);
/* bitwise operators for 32-bit bitvector end */
