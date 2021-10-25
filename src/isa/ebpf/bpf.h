/* copy from linux/bpf.h, linux/bpf_common.h
 * commit(tag): v5.4
 */

#pragma once

#include <stdint.h>
/* Instruction classes */
#define BPF_CLASS(code) ((code) & 0x07)
#define   BPF_LD    0x00
#define   BPF_LDX   0x01
#define   BPF_ST    0x02
#define   BPF_STX   0x03
#define   BPF_ALU   0x04
#define   BPF_JMP   0x05
#define   BPF_RET   0x06
#define   BPF_MISC  0x07

/* ld/ldx fields */
#define BPF_SIZE(code)  ((code) & 0x18)
#define   BPF_W   0x00 /* 32-bit */
#define   BPF_H   0x08 /* 16-bit */
#define   BPF_B   0x10 /*  8-bit */
/* eBPF   BPF_DW    0x18  64-bit */
#define BPF_MODE(code)  ((code) & 0xe0)
#define   BPF_IMM   0x00
#define   BPF_ABS   0x20
#define   BPF_IND   0x40
#define   BPF_MEM   0x60
#define   BPF_LEN   0x80
#define   BPF_MSH   0xa0

/* alu/jmp fields */
#define BPF_OP(code)    ((code) & 0xf0)
#define   BPF_ADD   0x00
#define   BPF_SUB   0x10
#define   BPF_MUL   0x20
#define   BPF_DIV   0x30
#define   BPF_OR    0x40
#define   BPF_AND   0x50
#define   BPF_LSH   0x60
#define   BPF_RSH   0x70
#define   BPF_NEG   0x80
#define   BPF_MOD   0x90
#define   BPF_XOR   0xa0

#define   BPF_JA    0x00
#define   BPF_JEQ   0x10
#define   BPF_JGT   0x20
#define   BPF_JGE   0x30
#define   BPF_JSET  0x40
#define BPF_SRC(code)   ((code) & 0x08)
#define   BPF_K     0x00
#define   BPF_X     0x08

/* instruction classes */
#define BPF_JMP32 0x06  /* jmp mode in word width */
#define BPF_ALU64 0x07  /* alu mode in double word width */

/* ld/ldx fields */
#define BPF_DW    0x18  /* double word (64-bit) */
#define BPF_XADD  0xc0  /* exclusive add */

/* alu/jmp fields */
#define BPF_MOV   0xb0  /* mov reg to reg */
#define BPF_ARSH  0xc0  /* sign extending arithmetic shift right */

/* change endianness of a register */
#define BPF_END   0xd0  /* flags for endianness conversion: */
#define BPF_TO_LE 0x00  /* convert to little-endian */
#define BPF_TO_BE 0x08  /* convert to big-endian */
#define BPF_FROM_LE BPF_TO_LE
#define BPF_FROM_BE BPF_TO_BE

/* jmp encodings */
#define BPF_JNE   0x50  /* jump != */
#define BPF_JLT   0xa0  /* LT is unsigned, '<' */
#define BPF_JLE   0xb0  /* LE is unsigned, '<=' */
#define BPF_JSGT  0x60  /* SGT is signed '>', GT in x86 */
#define BPF_JSGE  0x70  /* SGE is signed '>=', GE in x86 */
#define BPF_JSLT  0xc0  /* SLT is signed, '<' */
#define BPF_JSLE  0xd0  /* SLE is signed, '<=' */
#define BPF_CALL  0x80  /* function call */
#define BPF_EXIT  0x90  /* function return */

/* Register numbers */
enum {
  BPF_REG_0 = 0,
  BPF_REG_1,
  BPF_REG_2,
  BPF_REG_3,
  BPF_REG_4,
  BPF_REG_5,
  BPF_REG_6,
  BPF_REG_7,
  BPF_REG_8,
  BPF_REG_9,
  BPF_REG_10,
  __MAX_BPF_REG,
};

#define __BPF_FUNC_MAPPER(FN)   \
  FN(unspec),     \
  FN(map_lookup_elem),    \
  FN(map_update_elem),    \
  FN(map_delete_elem),    \
  FN(probe_read),     \
  FN(ktime_get_ns),   \
  FN(trace_printk),   \
  FN(get_prandom_u32),    \
  FN(get_smp_processor_id), \
  FN(skb_store_bytes),    \
  FN(l3_csum_replace),    \
  FN(l4_csum_replace),    \
  FN(tail_call),      \
  FN(clone_redirect),   \
  FN(get_current_pid_tgid), \
  FN(get_current_uid_gid),  \
  FN(get_current_comm),   \
  FN(get_cgroup_classid),   \
  FN(skb_vlan_push),    \
  FN(skb_vlan_pop),   \
  FN(skb_get_tunnel_key),   \
  FN(skb_set_tunnel_key),   \
  FN(perf_event_read),    \
  FN(redirect),     \
  FN(get_route_realm),    \
  FN(perf_event_output),    \
  FN(skb_load_bytes),   \
  FN(get_stackid),    \
  FN(csum_diff),      \
  FN(skb_get_tunnel_opt),   \
  FN(skb_set_tunnel_opt),   \
  FN(skb_change_proto),   \
  FN(skb_change_type),    \
  FN(skb_under_cgroup),   \
  FN(get_hash_recalc),    \
  FN(get_current_task),   \
  FN(probe_write_user),   \
  FN(current_task_under_cgroup),  \
  FN(skb_change_tail),    \
  FN(skb_pull_data),    \
  FN(csum_update),    \
  FN(set_hash_invalid),   \
  FN(get_numa_node_id),   \
  FN(skb_change_head),    \
  FN(xdp_adjust_head),    \
  FN(probe_read_str),   \
  FN(get_socket_cookie),    \
  FN(get_socket_uid),   \
  FN(set_hash),     \
  FN(setsockopt),     \
  FN(skb_adjust_room),    \
  FN(redirect_map),   \
  FN(sk_redirect_map),    \
  FN(sock_map_update),    \
  FN(xdp_adjust_meta),    \
  FN(perf_event_read_value),  \
  FN(perf_prog_read_value), \
  FN(getsockopt),     \
  FN(override_return),    \
  FN(sock_ops_cb_flags_set),  \
  FN(msg_redirect_map),   \
  FN(msg_apply_bytes),    \
  FN(msg_cork_bytes),   \
  FN(msg_pull_data),    \
  FN(bind),     \
  FN(xdp_adjust_tail),    \
  FN(skb_get_xfrm_state),   \
  FN(get_stack),      \
  FN(skb_load_bytes_relative),  \
  FN(fib_lookup),     \
  FN(sock_hash_update),   \
  FN(msg_redirect_hash),    \
  FN(sk_redirect_hash),   \
  FN(lwt_push_encap),   \
  FN(lwt_seg6_store_bytes), \
  FN(lwt_seg6_adjust_srh),  \
  FN(lwt_seg6_action),    \
  FN(rc_repeat),      \
  FN(rc_keydown),     \
  FN(skb_cgroup_id),    \
  FN(get_current_cgroup_id),  \
  FN(get_local_storage),    \
  FN(sk_select_reuseport),  \
  FN(skb_ancestor_cgroup_id), \
  FN(sk_lookup_tcp),    \
  FN(sk_lookup_udp),    \
  FN(sk_release),     \
  FN(map_push_elem),    \
  FN(map_pop_elem),   \
  FN(map_peek_elem),    \
  FN(msg_push_data),    \
  FN(msg_pop_data),   \
  FN(rc_pointer_rel),   \
  FN(spin_lock),      \
  FN(spin_unlock),    \
  FN(sk_fullsock),    \
  FN(tcp_sock),     \
  FN(skb_ecn_set_ce),   \
  FN(get_listener_sock),    \
  FN(skc_lookup_tcp),   \
  FN(tcp_check_syncookie),  \
  FN(sysctl_get_name),    \
  FN(sysctl_get_current_value), \
  FN(sysctl_get_new_value), \
  FN(sysctl_set_new_value), \
  FN(strtol),     \
  FN(strtoul),      \
  FN(sk_storage_get),   \
  FN(sk_storage_delete),    \
  FN(send_signal),    \
  FN(tcp_gen_syncookie),

/* integer value in 'imm' field of BPF_CALL instruction selects which helper
 * function eBPF program intends to call
 */
#define __BPF_ENUM_FN(x) BPF_FUNC_ ## x
enum bpf_func_id {
  __BPF_FUNC_MAPPER(__BPF_ENUM_FN)
  __BPF_FUNC_MAX_ID,
};
#undef __BPF_ENUM_FN

enum bpf_map_type {
  BPF_MAP_TYPE_UNSPEC,
  BPF_MAP_TYPE_HASH,
  BPF_MAP_TYPE_ARRAY,
  BPF_MAP_TYPE_PROG_ARRAY,
  BPF_MAP_TYPE_PERF_EVENT_ARRAY,
  BPF_MAP_TYPE_PERCPU_HASH,
  BPF_MAP_TYPE_PERCPU_ARRAY,
  BPF_MAP_TYPE_STACK_TRACE,
  BPF_MAP_TYPE_CGROUP_ARRAY,
  BPF_MAP_TYPE_LRU_HASH,
  BPF_MAP_TYPE_LRU_PERCPU_HASH,
  BPF_MAP_TYPE_LPM_TRIE,
  BPF_MAP_TYPE_ARRAY_OF_MAPS,
  BPF_MAP_TYPE_HASH_OF_MAPS,
  BPF_MAP_TYPE_DEVMAP,
  BPF_MAP_TYPE_SOCKMAP,
  BPF_MAP_TYPE_CPUMAP,
  BPF_MAP_TYPE_XSKMAP,
  BPF_MAP_TYPE_SOCKHASH,
  BPF_MAP_TYPE_CGROUP_STORAGE,
  BPF_MAP_TYPE_REUSEPORT_SOCKARRAY,
  BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE,
  BPF_MAP_TYPE_QUEUE,
  BPF_MAP_TYPE_STACK,
  BPF_MAP_TYPE_SK_STORAGE,
  BPF_MAP_TYPE_DEVMAP_HASH,
};

// not copy from linux/bpf.h or linux/bpf_common.h
struct bpf_insn {
  uint8_t opcode;
  uint8_t dst_reg: 4;
  uint8_t src_reg: 4;
  short off;
  int imm;
};

/* helper related: from linux/v5.4/source/include/linux/bpf.h
   modifications: search `K2 modification`
*/
/* function argument constraints */
enum bpf_arg_type {
  ARG_DONTCARE = 0, /* unused argument in helper function */

  /* the following constraints used to prototype
   * bpf_map_lookup/update/delete_elem() functions
   */
  ARG_CONST_MAP_PTR,  /* const argument used as pointer to bpf_map */
  ARG_PTR_TO_MAP_KEY, /* pointer to stack used as map key */
  ARG_PTR_TO_MAP_VALUE, /* pointer to stack used as map value */
  ARG_PTR_TO_UNINIT_MAP_VALUE,  /* pointer to valid memory used to store a map value */
  ARG_PTR_TO_MAP_VALUE_OR_NULL, /* pointer to stack used as map value or NULL */

  /* the following constraints used to prototype bpf_memcmp() and other
   * functions that access data on eBPF program stack
   */
  ARG_PTR_TO_MEM,   /* pointer to valid memory (stack, packet, map value) */
  ARG_PTR_TO_MEM_OR_NULL, /* pointer to valid memory or NULL */
  ARG_PTR_TO_UNINIT_MEM,  /* pointer to memory does not need to be initialized,
         * helper function must fill all bytes or clear
         * them in error case.
         */

  ARG_CONST_SIZE,   /* number of bytes accessed from memory */
  ARG_CONST_SIZE_OR_ZERO, /* number of bytes accessed from memory or 0 */

  ARG_PTR_TO_CTX,   /* pointer to context */
  ARG_ANYTHING,   /* any (initialized) argument is ok */
  ARG_PTR_TO_SPIN_LOCK, /* pointer to bpf_spin_lock */
  ARG_PTR_TO_SOCK_COMMON, /* pointer to sock_common */
  ARG_PTR_TO_INT,   /* pointer to int */
  ARG_PTR_TO_LONG,  /* pointer to long */
  ARG_PTR_TO_SOCKET,  /* pointer to bpf_sock (fullsock) */
};

/* type of values returned from helper functions */
enum bpf_return_type {
  RET_INTEGER,      /* function returns integer */
  RET_VOID,     /* function doesn't return anything */
  RET_PTR_TO_MAP_VALUE,   /* returns a pointer to map elem value */
  RET_PTR_TO_MAP_VALUE_OR_NULL, /* returns a pointer to map elem value or NULL */
  RET_PTR_TO_SOCKET_OR_NULL,  /* returns a pointer to a socket or NULL */
  RET_PTR_TO_TCP_SOCK_OR_NULL,  /* returns a pointer to a tcp_sock or NULL */
  RET_PTR_TO_SOCK_COMMON_OR_NULL, /* returns a pointer to a sock_common or NULL */
};

/* eBPF function prototype used by verifier to allow BPF_CALLs from eBPF programs
 * to in-kernel helper functions and for adjusting imm32 field in BPF_CALL
 * instructions after verifying
 */
struct bpf_func_proto {
  // u64 (*func)(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5); // K2 modification: comment
  bool gpl_only;
  bool pkt_access;
  enum bpf_return_type ret_type;
  enum bpf_arg_type arg1_type;
  enum bpf_arg_type arg2_type;
  enum bpf_arg_type arg3_type;
  enum bpf_arg_type arg4_type;
  enum bpf_arg_type arg5_type;
};

const struct bpf_func_proto bpf_map_lookup_elem_proto = {
  // .func   = bpf_map_lookup_elem,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_PTR_TO_MAP_VALUE_OR_NULL,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_MAP_KEY,
};

const struct bpf_func_proto bpf_map_update_elem_proto = {
  // .func   = bpf_map_update_elem,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_MAP_KEY,
  .arg3_type  = ARG_PTR_TO_MAP_VALUE,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_map_delete_elem_proto = {
  // .func   = bpf_map_delete_elem,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_MAP_KEY,
};

const struct bpf_func_proto bpf_map_push_elem_proto = {
  // .func   = bpf_map_push_elem,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_MAP_VALUE,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_map_pop_elem_proto = {
  // .func   = bpf_map_pop_elem,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_UNINIT_MAP_VALUE,
};

const struct bpf_func_proto bpf_map_peek_elem_proto = {
  // .func   = bpf_map_pop_elem,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_UNINIT_MAP_VALUE,
};

const struct bpf_func_proto bpf_get_prandom_u32_proto = {
  // .func   = bpf_user_rnd_u32,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_smp_processor_id_proto = {
  // .func   = bpf_get_smp_processor_id,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_numa_node_id_proto = {
  // .func   = bpf_get_numa_node_id,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_ktime_get_ns_proto = {
  // .func   = bpf_ktime_get_ns,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_current_pid_tgid_proto = {
  // .func   = bpf_get_current_pid_tgid,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_current_uid_gid_proto = {
  // .func   = bpf_get_current_uid_gid,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_current_comm_proto = {
  // .func   = bpf_get_current_comm,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg2_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_spin_lock_proto = {
  // .func   = bpf_spin_lock,
  .gpl_only = false,
  .ret_type = RET_VOID,
  .arg1_type  = ARG_PTR_TO_SPIN_LOCK,
};

const struct bpf_func_proto bpf_spin_unlock_proto = {
  // .func   = bpf_spin_unlock,
  .gpl_only = false,
  .ret_type = RET_VOID,
  .arg1_type  = ARG_PTR_TO_SPIN_LOCK,
};

const struct bpf_func_proto bpf_get_current_cgroup_id_proto = {
  // .func   = bpf_get_current_cgroup_id,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_get_local_storage_proto = {
  // .func   = bpf_get_local_storage,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_MAP_VALUE,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_strtol_proto = {
  // .func   = bpf_strtol,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_MEM,
  .arg2_type  = ARG_CONST_SIZE,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_LONG,
};

const struct bpf_func_proto bpf_strtoul_proto = {
  // .func   = bpf_strtoul,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_MEM,
  .arg2_type  = ARG_CONST_SIZE,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_LONG,
};

const struct bpf_func_proto bpf_override_return_proto = {
  // .func   = bpf_override_return,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_probe_write_user_proto = {
  // .func   = bpf_probe_write_user,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_ANYTHING,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_trace_printk_proto = {
  // .func   = bpf_trace_printk,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_MEM,
  .arg2_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_perf_event_read_value_proto = {
  // .func   = bpf_perf_event_read_value,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_perf_event_output_proto = {
  // .func   = bpf_perf_event_output,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_get_current_task_proto = {
  // .func   = bpf_get_current_task,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_current_task_under_cgroup_proto = {
  // .func           = bpf_current_task_under_cgroup,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_CONST_MAP_PTR,
  .arg2_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_get_stackid_proto_tp = {
  // .func   = bpf_get_stackid_tp,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_get_stack_proto_tp = {
  // .func   = bpf_get_stack_tp,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_perf_prog_read_value_proto = {
  // .func           = bpf_perf_prog_read_value,
  .gpl_only       = true,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type      = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_perf_event_output_proto_raw_tp = {
  // .func   = bpf_perf_event_output_raw_tp,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_get_stackid_proto_raw_tp = {
  // .func   = bpf_get_stackid_raw_tp,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_get_stack_proto_raw_tp = {
  // .func   = bpf_get_stack_raw_tp,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_probe_read_proto = {
  // .func   = bpf_probe_read,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg2_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_perf_event_read_proto = {
  // .func   = bpf_perf_event_read,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_probe_read_str_proto = {
  // .func   = bpf_probe_read_str,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg2_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_send_signal_proto = {
  // .func   = bpf_send_signal,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_ANYTHING,
};

// bpf_rc_repeat_proto, bpf_rc_keydown_proto, bpf_rc_pointer_rel_proto
// are from rc_repeat_proto, rc_keydown_proto, rc_pointer_rel_proto
static const struct bpf_func_proto bpf_rc_repeat_proto = {
  // .func    = bpf_rc_repeat,
  .gpl_only  = true, /* rc_repeat is EXPORT_SYMBOL_GPL */
  .ret_type  = RET_INTEGER,
  .arg1_type = ARG_PTR_TO_CTX,
};

static const struct bpf_func_proto bpf_rc_keydown_proto = {
  // .func    = bpf_rc_keydown,
  .gpl_only  = true, /* rc_keydown is EXPORT_SYMBOL_GPL */
  .ret_type  = RET_INTEGER,
  .arg1_type = ARG_PTR_TO_CTX,
  .arg2_type = ARG_ANYTHING,
  .arg3_type = ARG_ANYTHING,
  .arg4_type = ARG_ANYTHING,
};

static const struct bpf_func_proto bpf_rc_pointer_rel_proto = {
  // .func    = bpf_rc_pointer_rel,
  .gpl_only  = true,
  .ret_type  = RET_INTEGER,
  .arg1_type = ARG_PTR_TO_CTX,
  .arg2_type = ARG_ANYTHING,
  .arg3_type = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sysctl_get_name_proto = {
  // .func   = bpf_sysctl_get_name,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sysctl_get_current_value_proto = {
  // .func   = bpf_sysctl_get_current_value,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_sysctl_get_new_value_proto = {
  // .func   = bpf_sysctl_get_new_value,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_sysctl_set_new_value_proto = {
  // .func   = bpf_sysctl_set_new_value,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_get_stackid_proto = {
  // .func   = bpf_get_stackid,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_get_stack_proto = {
  // .func   = bpf_get_stack,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_storage_get_proto = {
  // .func   = bpf_sk_storage_get,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_MAP_VALUE_OR_NULL,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_SOCKET,
  .arg3_type  = ARG_PTR_TO_MAP_VALUE_OR_NULL,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_storage_delete_proto = {
  // .func   = bpf_sk_storage_delete,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_CONST_MAP_PTR,
  .arg2_type  = ARG_PTR_TO_SOCKET,
};

const struct bpf_func_proto bpf_get_raw_smp_processor_id_proto = {
  // .func   = bpf_get_raw_cpu_id,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
};

const struct bpf_func_proto bpf_skb_store_bytes_proto = {
  // .func   = bpf_skb_store_bytes,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_MEM,
  .arg4_type  = ARG_CONST_SIZE,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_load_bytes_proto = {
  // .func   = bpf_skb_load_bytes,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_flow_dissector_load_bytes_proto = {
  // .func   = bpf_flow_dissector_load_bytes,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_skb_load_bytes_relative_proto = {
  // .func   = bpf_skb_load_bytes_relative,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_pull_data_proto = {
  // .func   = bpf_skb_pull_data,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_fullsock_proto = {
  // .func   = bpf_sk_fullsock,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
};

const struct bpf_func_proto bpf_l3_csum_replace_proto = {
  // .func   = bpf_l3_csum_replace,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_l4_csum_replace_proto = {
  // .func   = bpf_l4_csum_replace,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_csum_diff_proto = {
  // .func   = bpf_csum_diff,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_MEM_OR_NULL,
  .arg2_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg3_type  = ARG_PTR_TO_MEM_OR_NULL,
  .arg4_type  = ARG_CONST_SIZE_OR_ZERO,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_csum_update_proto = {
  // .func   = bpf_csum_update,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_clone_redirect_proto = {
  // .func           = bpf_clone_redirect,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_ANYTHING,
  .arg3_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_redirect_proto = {
  // .func           = bpf_redirect,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_ANYTHING,
  .arg2_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_apply_bytes_proto = {
  // .func           = bpf_msg_apply_bytes,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_cork_bytes_proto = {
  // .func           = bpf_msg_cork_bytes,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_pull_data_proto = {
  // .func   = bpf_msg_pull_data,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_push_data_proto = {
  // .func   = bpf_msg_push_data,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_pop_data_proto = {
  // .func   = bpf_msg_pop_data,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_get_cgroup_classid_proto = {
  // .func           = bpf_get_cgroup_classid,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_get_route_realm_proto = {
  // .func           = bpf_get_route_realm,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_get_hash_recalc_proto = {
  // .func   = bpf_get_hash_recalc,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_set_hash_invalid_proto = {
  // .func   = bpf_set_hash_invalid,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_set_hash_proto = {
  // .func   = bpf_set_hash,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_vlan_push_proto = {
  // .func           = bpf_skb_vlan_push,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_ANYTHING,
  .arg3_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_vlan_pop_proto = {
  // .func           = bpf_skb_vlan_pop,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_skb_change_proto_proto = {
  // .func   = bpf_skb_change_proto,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_change_type_proto = {
  // .func   = bpf_skb_change_type,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_adjust_room_proto = {
  // .func   = bpf_skb_adjust_room,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_change_tail_proto = {
  // .func   = bpf_skb_change_tail,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_change_head_proto = {
  // .func   = bpf_skb_change_head,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_adjust_head_proto = {
  // .func   = bpf_xdp_adjust_head,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_adjust_tail_proto = {
  // .func   = bpf_xdp_adjust_tail,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_adjust_meta_proto = {
  // .func   = bpf_xdp_adjust_meta,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_redirect_proto = {
  // .func           = bpf_xdp_redirect,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_ANYTHING,
  .arg2_type      = ARG_ANYTHING,
};

// bpf_redirect_map_proto is from bpf_xdp_redirect_map_proto
// https://www.spinics.net/lists/netdev/msg451173.html
const struct bpf_func_proto bpf_redirect_map_proto = {
  // .func           = bpf_xdp_redirect_map,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_CONST_MAP_PTR,
  .arg2_type      = ARG_ANYTHING,
  .arg3_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_event_output_proto = {
  // .func   = bpf_skb_event_output,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_skb_get_tunnel_key_proto = {
  // .func   = bpf_skb_get_tunnel_key,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_get_tunnel_opt_proto = {
  // .func   = bpf_skb_get_tunnel_opt,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_skb_set_tunnel_key_proto = {
  // .func   = bpf_skb_set_tunnel_key,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_set_tunnel_opt_proto = {
  // .func   = bpf_skb_set_tunnel_opt,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_skb_under_cgroup_proto = {
  // .func   = bpf_skb_under_cgroup,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skb_cgroup_id_proto = {
  // .func           = bpf_skb_cgroup_id,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_skb_ancestor_cgroup_id_proto = {
  // .func           = bpf_skb_ancestor_cgroup_id,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_event_output_proto = {
  // .func   = bpf_xdp_event_output,
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_get_socket_cookie_proto = {
  // .func           = bpf_get_socket_cookie,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_get_socket_cookie_sock_addr_proto = {
  // .func   = bpf_get_socket_cookie_sock_addr,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_get_socket_cookie_sock_ops_proto = {
  // .func   = bpf_get_socket_cookie_sock_ops,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_get_socket_uid_proto = {
  // .func           = bpf_get_socket_uid,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_sockopt_event_output_proto =  {
  // .func   = bpf_sockopt_event_output,
  .gpl_only       = true,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_ANYTHING,
  .arg4_type      = ARG_PTR_TO_MEM,
  .arg5_type      = ARG_CONST_SIZE_OR_ZERO,
};

const struct bpf_func_proto bpf_setsockopt_proto = {
  // .func   = bpf_setsockopt,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_getsockopt_proto = {
  // .func   = bpf_getsockopt,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
  .arg4_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg5_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_sock_ops_cb_flags_set_proto = {
  // .func   = bpf_sock_ops_cb_flags_set,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_bind_proto = {
  // .func   = bpf_bind,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_skb_get_xfrm_state_proto = {
  // .func   = bpf_skb_get_xfrm_state,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
  .arg5_type  = ARG_ANYTHING,
};

// fib_lookup_proto is from bpf_xdp_fib_lookup_proto and bpf_skb_fib_lookup_proto
const struct bpf_func_proto bpf_fib_lookup_proto = {
  .gpl_only = true,
  .ret_type = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_PTR_TO_MEM,
  .arg3_type      = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
};

// bpf_lwt_push_encap_proto is from bpf_lwt_in_push_encap_proto and bpf_lwt_xmit_push_encap_proto
const struct bpf_func_proto bpf_lwt_push_encap_proto = {
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_MEM,
  .arg4_type  = ARG_CONST_SIZE
};

const struct bpf_func_proto bpf_lwt_seg6_store_bytes_proto = {
  // .func   = bpf_lwt_seg6_store_bytes,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_MEM,
  .arg4_type  = ARG_CONST_SIZE
};

const struct bpf_func_proto bpf_lwt_seg6_action_proto = {
  // .func   = bpf_lwt_seg6_action,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_MEM,
  .arg4_type  = ARG_CONST_SIZE
};

const struct bpf_func_proto bpf_lwt_seg6_adjust_srh_proto = {
  // .func   = bpf_lwt_seg6_adjust_srh,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_skc_lookup_tcp_proto = {
  // .func   = bpf_skc_lookup_tcp,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_PTR_TO_SOCK_COMMON_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_lookup_tcp_proto = {
  // .func   = bpf_sk_lookup_tcp,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_lookup_udp_proto = {
  // .func   = bpf_sk_lookup_udp,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_release_proto = {
  // .func   = bpf_sk_release,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
};

const struct bpf_func_proto bpf_xdp_sk_lookup_udp_proto = {
  // .func           = bpf_xdp_sk_lookup_udp,
  .gpl_only       = false,
  .pkt_access     = true,
  .ret_type       = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_PTR_TO_MEM,
  .arg3_type      = ARG_CONST_SIZE,
  .arg4_type      = ARG_ANYTHING,
  .arg5_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_skc_lookup_tcp_proto = {
  // .func           = bpf_xdp_skc_lookup_tcp,
  .gpl_only       = false,
  .pkt_access     = true,
  .ret_type       = RET_PTR_TO_SOCK_COMMON_OR_NULL,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_PTR_TO_MEM,
  .arg3_type      = ARG_CONST_SIZE,
  .arg4_type      = ARG_ANYTHING,
  .arg5_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_xdp_sk_lookup_tcp_proto = {
  // .func           = bpf_xdp_sk_lookup_tcp,
  .gpl_only       = false,
  .pkt_access     = true,
  .ret_type       = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type      = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_PTR_TO_MEM,
  .arg3_type      = ARG_CONST_SIZE,
  .arg4_type      = ARG_ANYTHING,
  .arg5_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sock_addr_skc_lookup_tcp_proto = {
  // .func   = bpf_sock_addr_skc_lookup_tcp,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_SOCK_COMMON_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sock_addr_sk_lookup_tcp_proto = {
  // .func   = bpf_sock_addr_sk_lookup_tcp,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sock_addr_sk_lookup_udp_proto = {
  // .func   = bpf_sock_addr_sk_lookup_udp,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_ANYTHING,
  .arg5_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_tcp_sock_proto = {
  // .func   = bpf_tcp_sock,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_TCP_SOCK_OR_NULL,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
};

const struct bpf_func_proto bpf_get_listener_sock_proto = {
  // .func   = bpf_get_listener_sock,
  .gpl_only = false,
  .ret_type = RET_PTR_TO_SOCKET_OR_NULL,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
};

const struct bpf_func_proto bpf_skb_ecn_set_ce_proto = {
  // .func           = bpf_skb_ecn_set_ce,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type      = ARG_PTR_TO_CTX,
};

const struct bpf_func_proto bpf_tcp_check_syncookie_proto = {
  // .func   = bpf_tcp_check_syncookie,
  .gpl_only = true,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_tcp_gen_syncookie_proto = {
  // .func   = bpf_tcp_gen_syncookie,
  .gpl_only = true, /* __cookie_v*_init_sequence() is GPL */
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_SOCK_COMMON,
  .arg2_type  = ARG_PTR_TO_MEM,
  .arg3_type  = ARG_CONST_SIZE,
  .arg4_type  = ARG_PTR_TO_MEM,
  .arg5_type  = ARG_CONST_SIZE,
};

const struct bpf_func_proto bpf_sock_map_update_proto = {
  // .func   = bpf_sock_map_update,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_PTR_TO_MAP_KEY,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_redirect_map_proto = {
  // .func           = bpf_sk_redirect_map,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_ANYTHING,
  .arg4_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_redirect_map_proto = {
  // .func           = bpf_msg_redirect_map,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_ANYTHING,
  .arg4_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sock_hash_update_proto = {
  // .func   = bpf_sock_hash_update,
  .gpl_only = false,
  .pkt_access = true,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_PTR_TO_MAP_KEY,
  .arg4_type  = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_sk_redirect_hash_proto = {
  // .func           = bpf_sk_redirect_hash,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_PTR_TO_MAP_KEY,
  .arg4_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_msg_redirect_hash_proto = {
  // .func           = bpf_msg_redirect_hash,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_PTR_TO_MAP_KEY,
  .arg4_type      = ARG_ANYTHING,
};

const struct bpf_func_proto bpf_tail_call_proto = {
  // .func   = NULL,
  .gpl_only = false,
  .ret_type = RET_VOID,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_CONST_MAP_PTR,
  .arg3_type  = ARG_ANYTHING,
};

static const struct bpf_func_proto bpf_sk_select_reuseport_proto = {
  // .func           = sk_select_reuseport,
  .gpl_only       = false,
  .ret_type       = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type      = ARG_CONST_MAP_PTR,
  .arg3_type      = ARG_PTR_TO_MAP_KEY,
  .arg4_type  = ARG_ANYTHING,
};

static const struct bpf_func_proto bpf_sk_reuseport_load_bytes_proto = {
  // .func   = sk_reuseport_load_bytes,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
};

static const struct bpf_func_proto bpf_sk_reuseport_load_bytes_relative_proto = {
  // .func   = sk_reuseport_load_bytes_relative,
  .gpl_only = false,
  .ret_type = RET_INTEGER,
  .arg1_type  = ARG_PTR_TO_CTX,
  .arg2_type  = ARG_ANYTHING,
  .arg3_type  = ARG_PTR_TO_UNINIT_MEM,
  .arg4_type  = ARG_CONST_SIZE,
  .arg5_type  = ARG_ANYTHING,
};


const bpf_func_proto* get_bpf_func_proto(int func_id);
