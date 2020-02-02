#pragma once

#include "../src/isa/ebpf/inst.h"

using namespace std;

// instruction_list set 
#undef N
#define N MAX_PROG_LEN

#undef NUM_ORIG
#define NUM_ORIG 1
extern inst_t ebpf_bm0[N];

extern inst_t ebpf_bm_opti00[N];
