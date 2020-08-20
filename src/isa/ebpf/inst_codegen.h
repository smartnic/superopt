#pragma once

#include <sstream>
#include <iostream>
#include <iomanip>
#include <utility>
#include <random>
#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/isa/inst_var.h"
#include "inst_var.h"

using namespace std;

/* Assume the machine is little-endian.
   APIs exposed to the externals,
   Should ensure all parameters do NOT have side effects.
*/
/* Function ID, BPF function id starts from 1 */
enum BPF_FUNC_IDS {
  BPF_FUNC_map_lookup = 1,
  BPF_FUNC_map_update,
  BPF_FUNC_map_delete,
  BPF_MAX_FUNC_ID,
};

#define MAP_UPD_RET 0
#define MAP_DEL_RET_IF_KEY_INEXIST (int64_t)0xfffffffe
#define MAP_DEL_RET_IF_KEY_EXIST 0

#define MAP_UPD_RET_EXPR to_expr(MAP_UPD_RET)
#define MAP_DEL_RET_IF_KEY_INEXIST_EXPR to_expr(MAP_DEL_RET_IF_KEY_INEXIST)
#define MAP_DEL_RET_IF_KEY_EXIST_EXPR to_expr(MAP_DEL_RET_IF_KEY_EXIST)
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
inline int64_t compute_ldmapid(int64_t in, int64_t out = 0);
// return (out = in1 op in2)
inline int64_t compute_add(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_add32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_or(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_or32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_and(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_and32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_lsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_lsh32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_rsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_rsh32(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_arsh(int64_t in1, int64_t in2, int64_t out = 0);
inline int64_t compute_arsh32(int64_t in1, int64_t in2, int64_t out = 0);
// out = (u_sz)[addr + off]
inline int64_t compute_ld8(uint64_t addr, int64_t off, int64_t out = 0);
inline int64_t compute_ld16(uint64_t addr, int64_t off, int64_t out = 0);
inline int64_t compute_ld32(uint64_t addr, int64_t off, int64_t out = 0);
inline int64_t compute_ld64(uint64_t addr, int64_t off, int64_t out = 0);
// (u_sz)[addr + off] = in
inline void compute_st8(int64_t in, uint64_t addr, int64_t off);
inline void compute_st16(int64_t in, uint64_t addr, int64_t off);
inline void compute_st32(int64_t in, uint64_t addr, int64_t off);
inline void compute_st64(int64_t in, uint64_t addr, int64_t off);
// (u_sz)[addr+off] += in
void compute_xadd64(int64_t in, uint64_t addr, int64_t off);
void compute_xadd32(int64_t in, uint64_t addr, int64_t off);
// map helper functions
uint64_t compute_helper_function(int func_id, uint64_t r1, uint64_t r2, uint64_t r3,
                                 uint64_t r4, uint64_t r5, mem_t& m, simu_real& sr, prog_state& ps);
uint64_t compute_map_lookup_helper(int addr_map, uint64_t addr_k, mem_t& m, simu_real& sr);
uint64_t compute_map_update_helper(int addr_map, uint64_t addr_k, uint64_t addr_v, mem_t& m,
                                   simu_real& sr);
uint64_t compute_map_delete_helper(int addr_map, uint64_t addr_k, mem_t& m,
                                   simu_real& sr);
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
inline z3::expr predicate_or(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_or32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_and(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_and32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_lsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_lsh32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_rsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_rsh32(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_arsh(z3::expr in1, z3::expr in2, z3::expr out);
inline z3::expr predicate_arsh32(z3::expr in1, z3::expr in2, z3::expr out);
// (write addr+off, sz, in, m); type: in, addr, off: bv64;
inline z3::expr predicate_st8(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
inline z3::expr predicate_st16(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
inline z3::expr predicate_st32(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
inline z3::expr predicate_st64(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
// out == (read addr+off, sz, m); type: addr, off, out: bv64;
inline z3::expr predicate_ld8(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0);
inline z3::expr predicate_ld16(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0);
z3::expr predicate_ld32(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0);
inline z3::expr predicate_ld64(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0);
// *(u64*)(addr+off) += in
z3::expr predicate_xadd64(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
// *(u32*)(addr+off) += in
z3::expr predicate_xadd32(z3::expr in, z3::expr addr, z3::expr off, smt_var& sv, unsigned int block = 0);
// out == map_id, and track the out
z3::expr predicate_ldmapid(z3::expr map_id, z3::expr out, smt_var& sv, unsigned int block = 0);
// map helper functions
// return map lookup FOL formula, addr_v = lookup k map, where k is key,
// addr_v is the address of key's value, map is the map address
// parameter: z3::expr cond = Z3_true
z3::expr predicate_helper_function(int func_id, z3::expr r1, z3::expr r2, z3::expr r3,
                                   z3::expr r4, z3::expr r5, z3::expr r0, smt_var& sv,
                                   unsigned int block = 0);
z3::expr predicate_map_lookup_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_map_v,
                                     smt_var& sv, unsigned int block = 0);
z3::expr predicate_map_update_helper(z3::expr addr_map, z3::expr addr_k, z3::expr addr_v, z3::expr out,
                                     smt_var& sv, unsigned int block = 0);
z3::expr predicate_map_delete_helper(z3::expr addr_map, z3::expr addr_k, z3::expr out, smt_var& sv,
                                     unsigned int block = 0);
// return the FOL formula that set two programs the same inputs (support: same input maps now)
z3::expr smt_map_set_same_input(smt_var& sv1, smt_var& sv2);

z3::expr smt_pgm_set_same_input(smt_var& sv1, smt_var& sv2);
// return the FOL formula that check whether two programs have the same output memories
z3::expr smt_pgm_mem_eq_chk(smt_var& sv1, smt_var& sv2);
void counterex_2_input_mem(inout_t& input, z3::model& mdl, smt_var& sv1, smt_var& sv2);
/* APIs exposed to the externals end */

/* APIS for unit tests start */
z3::expr smt_stack_eq_chk(smt_wt& x, smt_wt& y,
                          z3::expr stack_start_addr, z3::expr stack_end_addr);
z3::expr smt_one_map_set_same_input(int map_id, smt_var& sv1, smt_var& sv2);
z3::expr smt_one_map_eq_chk(int map_id, smt_var& sv1,
                            smt_var& sv2);
z3::expr smt_map_eq_chk(smt_var& sv1, smt_var& sv2);
z3::expr smt_pkt_set_same_input(smt_var& sv1, smt_var& sv2);
z3::expr smt_pkt_eq_chk(smt_var& sv1, smt_var& sv2);
// For the conversion from counterexample (z3 model from validator)
// to input memory (mem_t) for interpreter
string z3_bv_2_hex_str(z3::expr z3_bv);
void counterex_urt_2_input_mem_for_one_sv(inout_t& input, z3::model& mdl, smt_var& sv);

/* APIS for unit tests end */

/* Inputs in, out must be side-effect-free expressions. */
#undef MOV_EXPR
#define MOV_EXPR(in, out) (out EQ in)
/* Inputs in1, in2, out must be side-effect-free expressions. */
#undef ADD_EXPR
#undef OR_EXPR
#undef AND_EXPR
#undef LSH_EXPR
#undef RSH_EXPR
#undef ARSH_EXPR
#define ADD_EXPR(in1, in2, out) (out EQ in1 + in2)
#define OR_EXPR(in1, in2, out) (out EQ (in1 | in2))
#define AND_EXPR(in1, in2, out) (out EQ (in1 & in2))
#define LSH_EXPR(in1, in2, out) (out EQ LSH(in1, in2))
#define RSH_EXPR(in1, in2, out) (out EQ RSH(in1, in2))
#define ARSH_EXPR(in1, in2, out) (out EQ ARSH(in1, in2))
#undef ADD32_EXPR
#undef OR32_EXPR
#undef AND32_EXPR
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
#define OR32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(INT32(in1) | INT32(in2)))
#define AND32_EXPR(in1, in2, out) (out EQ SET_HIGHER32_ZERO(INT32(in1) & INT32(in2)))
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
COMPUTE_BINARY( or , OR_EXPR)
COMPUTE_BINARY(or32, OR32_EXPR)
COMPUTE_BINARY( and , AND_EXPR)
COMPUTE_BINARY(and32, AND32_EXPR)
COMPUTE_BINARY(lsh, LSH_EXPR)
COMPUTE_BINARY(lsh32, LSH32_EXPR)
COMPUTE_BINARY(rsh, RSH_EXPR)
COMPUTE_BINARY(rsh32, RSH32_EXPR)
COMPUTE_BINARY(arsh, ARSH_EXPR)
COMPUTE_BINARY(arsh32, ARSH32_EXPR)

inline int64_t compute_ldmapid(int64_t in, int64_t out) {
  return in;
}

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
PREDICATE_BINARY( or , OR_EXPR)
PREDICATE_BINARY(or32, OR32_EXPR)
PREDICATE_BINARY( and , AND_EXPR)
PREDICATE_BINARY(and32, AND32_EXPR)
PREDICATE_BINARY(lsh, LSH_EXPR)
PREDICATE_BINARY(rsh, RSH_EXPR)
PREDICATE_BINARY(arsh, ARSH_EXPR)
PREDICATE_BINARY(lsh32, LSH32_EXPR);
PREDICATE_BINARY(rsh32, RSH32_EXPR);
PREDICATE_BINARY(arsh32, ARSH32_EXPR);

/* Memory access implementation */
#define COMPUTE_LDST(SIZE, SIZE_TYPE) \
inline int64_t compute_ld##SIZE(uint64_t addr, int64_t off, int64_t out) { \
  out = *(SIZE_TYPE*)(addr + off); \
  return out; \
} \
inline void compute_st##SIZE(int64_t in, uint64_t addr, int64_t off) { \
  *(SIZE_TYPE*)(addr + off) = in; \
}

COMPUTE_LDST(8, uint8_t)
COMPUTE_LDST(16, uint16_t)
COMPUTE_LDST(32, uint32_t)
COMPUTE_LDST(64, uint64_t)

// for the current compiler version, compute_xadd does not lock memory
#define COMPUTE_XADD(SIZE, SIZE_TYPE) \
inline void compute_xadd##SIZE(int64_t in, uint64_t addr, int64_t off) { \
  SIZE_TYPE a = in + *(SIZE_TYPE*)(addr + off); \
  *(SIZE_TYPE*)(addr + off) = a; \
}

COMPUTE_XADD(32, uint32_t)
COMPUTE_XADD(64, uint64_t)

// implemented in inst_codegen.cc, where addr is a reg experssion
z3::expr predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_var &sv, unsigned int block = 0, z3::expr cond = Z3_true);

inline z3::expr predicate_st8(z3::expr in, z3::expr addr, z3::expr off, smt_var &sv, unsigned int block) {
  return predicate_st_byte(in.extract(7, 0), addr, off, sv, block);
}

inline z3::expr predicate_st16(z3::expr in, z3::expr addr, z3::expr off, smt_var &sv, unsigned int block) {
  return (predicate_st8(in.extract(7, 0), addr, off, sv, block) &&
          predicate_st8(in.extract(15, 8), addr, off + to_expr(1, 64), sv, block));
}

inline z3::expr predicate_st32(z3::expr in, z3::expr addr, z3::expr off, smt_var &sv, unsigned int block) {
  return (predicate_st16(in.extract(15, 0), addr, off, sv, block) &&
          predicate_st16(in.extract(31, 16), addr, off + to_expr(2, 64), sv, block));
}

inline z3::expr predicate_st64(z3::expr in, z3::expr addr, z3::expr off, smt_var &sv, unsigned int block) {
  return (predicate_st32(in.extract(31, 0), addr, off, sv, block) &&
          predicate_st32(in.extract(63, 32), addr, off + to_expr(4, 64), sv, block));
}

// implemented in inst_codegen.cc
z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block = 0, z3::expr cond = Z3_true);

inline z3::expr predicate_ld8(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block) {
  return ((out.extract(63, 8) == to_expr(0, 56)) &&
          predicate_ld_byte(addr, off, sv, out.extract(7, 0), block));
}

inline z3::expr predicate_ld16(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block) {
  return ((out.extract(63, 16) == to_expr(0, 48)) &&
          predicate_ld_byte(addr, off, sv, out.extract(7, 0), block) &&
          predicate_ld_byte(addr, off + 1, sv, out.extract(15, 8), block));
}

inline z3::expr predicate_ld64(z3::expr addr, z3::expr off, smt_var& sv, z3::expr out, unsigned int block) {
  z3::expr f = predicate_ld_byte(addr, off, sv, out.extract(7, 0), block);
  for (int i = 1; i < 8; i++) {
    f = f && predicate_ld_byte(addr, off + i, sv, out.extract(8 * i + 7, 8 * i), block);
  }
  return f;
}

string ld_n_bytes_from_addr(const uint8_t *v, const size_t s);

void get_v_from_addr_v(vector<uint8_t>& v, uint64_t addr_v,
                       vector<pair<uint64_t, uint8_t>>& mem_addr_val);
