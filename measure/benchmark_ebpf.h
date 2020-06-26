#pragma once

#include <vector>
#include "../src/isa/ebpf/inst.h"

using namespace std;
void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id);
// N can not greater than 56 because of the limit of combination function
#undef N0
#undef N1
#undef N2
#undef N3
#undef N4
#undef N5
#define N0 7
#define N1 7
#define N2 16
#define N3 91
#define N4 7
#define N5 7

extern inst bm0[N0];
extern inst bm1[N1];
extern inst bm2[N2];
extern inst bm3[N3];
extern inst bm4[N4];
extern inst bm5[N5];
