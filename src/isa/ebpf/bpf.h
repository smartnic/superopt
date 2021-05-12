/* copy from linux/bpf.h, linux/bpf_common.h
 * commit: 9e6c535c64adf6155e4a11fe8d63b384fe3452f8
 */

#pragma once

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
  FN(tcp_gen_syncookie),    \
  FN(skb_output),     \
  FN(probe_read_user),    \
  FN(probe_read_kernel),    \
  FN(probe_read_user_str),  \
  FN(probe_read_kernel_str),  \
  FN(tcp_send_ack),   \
  FN(send_signal_thread),   \
  FN(jiffies64),

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
  BPF_MAP_TYPE_STRUCT_OPS,
};


// Struct needs to be defined because the loader writes to
// the .ins file using bpf_insn which has a different size
// than insn
struct bpf_insn {

  uint8_t code;
  uint8_t dst_reg: 4;
  uint8_t src_reg: 4;
  short off;
  int imm;
};
