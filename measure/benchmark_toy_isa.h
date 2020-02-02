#pragma once

#include <vector>
#include <ostream>
#include "../src/isa/toy-isa/inst.h"

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v);
ostream& operator<<(ostream& out, vector<vector<int> >& v);

// instruction_list set
#undef N
#define N MAX_PROG_LEN

#undef NUM_ORIG
#define NUM_ORIG 3
extern inst_t bm0[N];
extern inst_t bm1[N];
extern inst_t bm2[N];

extern inst_t bm_opti00[N];
extern inst_t bm_opti01[N];
extern inst_t bm_opti02[N];
extern inst_t bm_opti03[N];
extern inst_t bm_opti04[N];
extern inst_t bm_opti05[N];

extern inst_t bm_opti10[N];
extern inst_t bm_opti11[N];
extern inst_t bm_opti12[N];
extern inst_t bm_opti13[N];

extern inst_t bm_opti20[N];
extern inst_t bm_opti21[N];
extern inst_t bm_opti22[N];
extern inst_t bm_opti23[N];
extern inst_t bm_opti24[N];
extern inst_t bm_opti25[N];
extern inst_t bm_opti26[N];
extern inst_t bm_opti27[N];
