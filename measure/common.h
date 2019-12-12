#pragma once

#include <vector>
#include <ostream>
#include "../src/isa/toy-isa/inst.h"

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v);
ostream& operator<<(ostream& out, vector<vector<int> >& v);

// instruction_list set
#define N MAX_PROG_LEN

#define NUM_ORIG 3
extern inst bm0[N];
extern inst bm1[N];
extern inst bm2[N];

extern inst bm_opti00[N];
extern inst bm_opti01[N];
extern inst bm_opti02[N];
extern inst bm_opti03[N];

extern inst bm_opti10[N];
extern inst bm_opti11[N];

extern inst bm_opti20[N];
extern inst bm_opti21[N];
extern inst bm_opti22[N];
