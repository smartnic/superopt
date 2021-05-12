#include "bpf_insn.h"

struct bpf_insn prog[] = {
  BPF_MOV64_IMM(BPF_REG_0, 0x1), // r0 = 1
  BPF_ALU64_REG(BPF_ADD, BPF_REG_0, BPF_REG_0), // r0 += r0
  BPF_EXIT_INSN(), // exit
};
