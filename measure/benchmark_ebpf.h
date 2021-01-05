#pragma once

#include <vector>
#include "../src/isa/ebpf/inst.h"

using namespace std;
void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id);
void init_benchmark_from_file(inst** bm, const char* insn_file, const char* map_file, const char* desc_file);
// N can not greater than 56 because of the limit of combination function
#undef N0
#undef N1
#undef N2
#undef N3
#undef N4
#undef N5
#undef N6
#undef N7
#undef N8
#undef N9
#undef N11
#undef N12
#define N0 7
#define N1 7
#define N2 16
#define N3 91
#define N4 7
#define N5 7
#define N6 7
#define N7 7
#define N8 24
#define N9 7
#define N10 13
#define N11 24
#define N12 61
#define N13 36
#define N14 24
#define N15 18
#define N16 18
#define N17 26
#define N18 24
#define N19 57
#define N20 38
#define N21 38
#define N22 41
#define N23 43
#define N24 22
#define N25 35

extern inst bm0[N0];
extern inst bm1[N1];
extern inst bm2[N2];
extern inst bm3[N3];
extern inst bm4[N4];
extern inst bm5[N5];
extern inst bm6[N6];
extern inst bm7[N7];
extern inst bm8[N8];
extern inst bm9[N9];
extern inst bm10[N10];
extern inst bm11[N11];
extern inst bm12[N12];
extern inst bm13[N13];
extern inst bm14[N14];
extern inst bm15[N15];
extern inst bm16[N16];
extern inst bm17[N17];
extern inst bm18[N18];
extern inst bm19[N19];
extern inst bm20[N20];
extern inst bm21[N21];
extern inst bm22[N22];
extern inst bm23[N23];
extern inst bm24[N24];
extern inst bm25[N25];
