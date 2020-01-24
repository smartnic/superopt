#pragma once

#include "../src/isa/ebpf/inst.h"

using namespace std;

// instruction_list set 
#undef N
#define N ebpf::MAX_PROG_LEN

#undef NUM_ORIG
#define NUM_ORIG 1
extern ebpf_inst ebpf_bm0[N];

extern ebpf_inst ebpf_bm_opti00[N];
