#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"

using namespace std;

/* Assume the machine is little-endian.
   APIs exposed to the externals,
   Should ensure all parameters do NOT have side effects.
*/
// type of parameters (in, in1, in2, out) and return value is int64_t
// return (out = op in)
inline int64_t compute_mov(int64_t in, int64_t out = 0);
inline int64_t compute_mov32(int64_t in, int64_t out = 0);
inline int64_t compute_le16(int64_t in, int64_t out = 0);
inline int64_t compute_le32(int64_t in, int64_t out = 0);
inline int64_t compute_le64(int64_t in, int64_t out = 0);
inline int64_t compute_be16(int64_t in, int64_t out = 0);
inline int64_t compute_be32(int64_t in, int64_t out = 0);
inline int64_t compute_be64(int64_t in, int64_t out = 0);

// return (out = in1 op in2)
inline int64_t compute_add(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_add32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_lsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_lsh32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_rsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_rsh32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_arsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_arsh32(int64_t in1, int64_t in2, int64_t out = 0);
// out = (u32)[addr + off]
inline int64_t compute_ld32(uint64_t addr, int64_t off, int64_t out = 0);
// (u32)[addr + off] = in
inline void compute_st32(int64_t in, uint64_t addr, int64_t off);

/* type of parameters (in, in1, in2, out) is z3 64-bit bitvector */
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
// return (out == (u32)mem[addr + off])
// mem type z3 array, bv64 -> bv8
inline z3::expr predicate_ld32(z3::expr addr, z3::expr off, z3::expr mem, z3::expr out);
// return mem_out == new_mem, new_mem: mem_in[addr + off] = (u32)in
inline z3::expr predicate_st32(z3::expr in, z3::expr addr, z3::expr off, z3::expr mem_in, z3::expr mem_out);

// out == (read addr+off, 8, s); type: addr, off, out: bv64;
inline z3::expr predicate_ld8(z3::expr addr, z3::expr off, smt_stack& s, z3::expr out);

/* APIs exposed to the externals end */

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
#undef ADD32_EXPR
#undef MOV32_EXPR
#undef LSH32_EXPR
#undef RSH32_EXPR
#undef ARSH32_EXPR
// 1. XXX32_EXPRs express the logic EBPF 32-bit ALU instructions.
// E.g., instruction [add32 dst, src]: dst = 0x00000000ffffffff & ((int32_t)dst + (int32_t)src)
// 2. SET_HIGHER32_ZERO(a) makes sure that the higher 32-bit of output is 0,
// and the lower 32-bit is the result of input experssion `a`
// 3. make sure in, in1, in2 should convert to int32_t or z3 32-bit bitvector first, i.e., use INT32()
// For some cases, this conversion does not matter. But for some, sucha as ARSH32, it matters,
// since whether higher bits are 1 or 0 depend on the 32th bit but not the 64th bit
// 4. Why RSH(a, b) and RSH32(a, b): for numerical number,
// need to convert a to unsigned according to its type: int32_t -> uint32_t, int64_t -> uint64_t
#define ADD32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(INT32(in1) + INT32(in2)))
#define MOV32_EXPR(in, out) (out EQ SET_HIGHER32_ZERO(INT32(in)))
#define LSH32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(LSH(INT32(in1), INT32(in2))))
#define RSH32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(RSH32(INT32(in1), INT32(in2))))
#define ARSH32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(ARSH(INT32(in1), INT32(in2))))

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
// For LE expressions, if the machine is little-endian, then set the higher bits of the
// value as 0, else swap the lower bits and set the higher bits of the value as 0.
// For BE expressions, if the machine is little-endian, then swap the lower bits and
// set the higher bits of the value as 0, else set the higher bits of the value as 0.
#define LE16_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SET_HIGHER48_ZERO(a), SET_HIGHER48_ZERO(SWAP16(a))))
#define LE32_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SET_HIGHER32_ZERO(a), SET_HIGHER32_ZERO(SWAP32(a))))
#define LE64_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), a, SWAP64(a)))
#define BE16_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SET_HIGHER48_ZERO(SWAP16(a)), SET_HIGHER48_ZERO(a)))
#define BE32_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SET_HIGHER32_ZERO(SWAP32(a)), SET_HIGHER32_ZERO(a)))
#define BE64_EXPR(a, b) (PRED_UNARY_EXPR(b, is_little_endian(), SWAP64(a), a))

#undef EQ
#define EQ =
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) if(pred) var EQ expr
#undef CONNECTIFELSE
#define CONNECTIFELSE  ;
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) else var EQ expr
#undef LSH
#define LSH(a, b) (a << b)
#undef RSH
#define RSH(a, b) ((uint64_t)a >> b)
#undef ARSH
#define ARSH(a, b) (a >> b)
#undef RSH32
#define RSH32(a, b) ((uint32_t)a >> b)
#undef SET_HIGHER32_ZERO
#define SET_HIGHER32_ZERO(a) (L32(a))
#undef SET_HIGHER48_ZERO
#define SET_HIGHER48_ZERO(a) (L16(a))
#undef INT32
#define INT32(a) (int32_t)(a)
#undef SWAP16
#undef SWAP32
#undef SWAP64
// unsigned(v) to make sure it is a logical shift
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

#undef COMPUTE_UNARY
#define COMPUTE_UNARY(func_name, operation)                                      \
inline int64_t compute_##func_name(int64_t in, int64_t out) {                    \
  operation(in, out);                                                            \
  return out;                                                                    \
}

#undef COMPUTE_BINARY
#define COMPUTE_BINARY(func_name, operation)                                     \
inline int64_t compute_##func_name(int64_t in1, int64_t in2, int64_t out) {      \
  operation(in1, in2, out);                                                      \
  return out;                                                                    \
}

COMPUTE_UNARY(mov, MOV_EXPR)
COMPUTE_UNARY(mov32, MOV32_EXPR)
COMPUTE_UNARY(le16, LE16_EXPR)
COMPUTE_UNARY(le32, LE32_EXPR)
COMPUTE_UNARY(le64, LE64_EXPR)
COMPUTE_UNARY(be16, BE16_EXPR)
COMPUTE_UNARY(be32, BE32_EXPR)
COMPUTE_UNARY(be64, BE64_EXPR)

COMPUTE_BINARY(add, ADD_EXPR)
COMPUTE_BINARY(add32, ADD32_EXPR)
COMPUTE_BINARY(lsh, LSH_EXPR)
COMPUTE_BINARY(lsh32, LSH32_EXPR)
COMPUTE_BINARY(rsh, RSH_EXPR)
COMPUTE_BINARY(rsh32, RSH32_EXPR)
COMPUTE_BINARY(arsh, ARSH_EXPR)
COMPUTE_BINARY(arsh32, ARSH32_EXPR)

#undef EQ
#define EQ ==
#undef IF_PRED_ACTION
#define IF_PRED_ACTION(pred, expr, var) ((pred) && (var EQ expr))
#undef CONNECTIFELSE
#define CONNECTIFELSE ||
#undef ELSE_PRED_ACTION
#define ELSE_PRED_ACTION(pred, expr, var) (!(pred) && (var EQ expr))
#undef LSH
#define LSH(a, b) z3::shl(a, b)
#undef RSH
#define RSH(a, b) z3::lshr(a, b)
#undef ARSH
#define ARSH(a, b) z3::ashr(a, b)
#undef RSH32
#define RSH32(a, b) RSH(a, b)
#undef SET_HIGHER32_ZERO
#define SET_HIGHER32_ZERO(a) z3::concat(to_expr((int32_t)0, 32), a.extract(31, 0))
#undef SET_HIGHER48_ZERO
#define SET_HIGHER48_ZERO(a) z3::concat(to_expr((int32_t)0, 48), a.extract(15, 0))
#undef INT32
#define INT32(a) a.extract(31, 0)
#undef SWAP16
#undef SWAP32
#undef SWAP64
#define SWAP16(v) z3::concat(v.extract(7, 0), v.extract(15, 8))
#define SWAP32(v) z3::concat(SWAP16(v), z3::concat(v.extract(23, 16), v.extract(31, 24)))
#define SWAP_BV_H32(v) z3::concat(z3::concat(v.extract(39, 32), v.extract(47, 40)), \
                                  z3::concat(v.extract(55, 48), v.extract(63, 56)))
#define SWAP64(v) z3::concat(SWAP32(v), SWAP_BV_H32(v))

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
PREDICATE_BINARY(lsh32, LSH32_EXPR);
PREDICATE_BINARY(rsh32, RSH32_EXPR);
PREDICATE_BINARY(arsh32, ARSH32_EXPR);

/* Memory access implementation */
// out = (u32)[addr + off]
inline int64_t compute_ld32(uint64_t addr, int64_t off, int64_t out) {
  out = *(uint32_t*)(addr + off);
  return out;
}
// (u32)[addr + off] = in
inline void compute_st32(int64_t in, uint64_t addr, int64_t off) {
  *(uint32_t*)(addr + off) = in;
}
// return (out == (u32)mem[addr + off])
// mem type z3 array, bv64 -> bv8
inline z3::expr predicate_ld32(z3::expr addr, z3::expr off, z3::expr mem, z3::expr out) {
  z3::expr a = z3::concat(to_expr(0, 32), mem[addr + off + 3]);
  a = z3::concat(a, mem[addr + off + 2]);
  a = z3::concat(a, mem[addr + off + 1]);
  a = z3::concat(a, mem[addr + off]);
  return (out == a);
}
// return mem_out == new_mem, new_mem: mem_in[addr + off] = (u32)in
inline z3::expr predicate_st32(z3::expr in, z3::expr addr, z3::expr off, z3::expr mem_in, z3::expr mem_out) {
  z3::expr mem1 = store(mem_in, addr + off, in.extract(7, 0));
  z3::expr mem2 = store(mem1, addr + off + to_expr(1, 64), in.extract(15, 8));
  z3::expr mem3 = store(mem2, addr + off + to_expr(2, 64), in.extract(23, 16));
  z3::expr mem4 = store(mem3, addr + off + to_expr(3, 64), in.extract(31, 24));
  return (mem_out == mem4);
}

inline z3::expr predicate_ld8(z3::expr addr, z3::expr off, smt_stack& s, z3::expr out) {
  int i = s.addr.size() - 1;
  z3::expr a = addr + off;
  z3::expr c = (a == s.addr[i]);
  z3::expr f = z3::implies(c, out == z3::concat(to_expr(0, 56), s.val[i]));
  for (i = i - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s.addr[i]), out == z3::concat(to_expr(0, 56), s.val[i]));
    c = c || (a == s.addr[i]);
  }
  return f;
}
