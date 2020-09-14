#pragma once

#include <vector>
#include <ostream>
#include "../src/isa/toy-isa/inst.h"

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v);
ostream& operator<<(ostream& out, vector<vector<int> >& v);

// instruction_list set
// N can not greater than 56 because of the limit of combination function
#undef N
#define N 7

#undef NUM_ORIG
#define NUM_ORIG 3
extern inst bm0[N];
extern inst bm1[N];
extern inst bm2[N];

extern inst bm_opti00[N];
extern inst bm_opti01[N];
extern inst bm_opti02[N];
extern inst bm_opti03[N];
extern inst bm_opti04[N];
extern inst bm_opti05[N];

extern inst bm_opti10[N];
extern inst bm_opti11[N];
extern inst bm_opti12[N];
extern inst bm_opti13[N];

extern inst bm_opti20[N];
extern inst bm_opti21[N];
extern inst bm_opti22[N];
extern inst bm_opti23[N];
extern inst bm_opti24[N];
extern inst bm_opti25[N];
extern inst bm_opti26[N];
extern inst bm_opti27[N];

void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id);
inline void init_benchmark_from_file(inst** bm, const char* insn_file, const char* map_file, const char* desc_file) {}
