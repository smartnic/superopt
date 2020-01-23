#pragma once

#include <vector>
#include <ostream>
#include "../src/isa/toy-isa/inst.h"

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v);
ostream& operator<<(ostream& out, vector<vector<int> >& v);

// instruction_list set
#define N toy_isa::MAX_PROG_LEN

#define NUM_ORIG 3
extern toy_isa_inst bm0[N];
extern toy_isa_inst bm1[N];
extern toy_isa_inst bm2[N];

extern toy_isa_inst bm_opti00[N];
extern toy_isa_inst bm_opti01[N];
extern toy_isa_inst bm_opti02[N];
extern toy_isa_inst bm_opti03[N];
extern toy_isa_inst bm_opti04[N];
extern toy_isa_inst bm_opti05[N];

extern toy_isa_inst bm_opti10[N];
extern toy_isa_inst bm_opti11[N];
extern toy_isa_inst bm_opti12[N];
extern toy_isa_inst bm_opti13[N];

extern toy_isa_inst bm_opti20[N];
extern toy_isa_inst bm_opti21[N];
extern toy_isa_inst bm_opti22[N];
extern toy_isa_inst bm_opti23[N];
extern toy_isa_inst bm_opti24[N];
extern toy_isa_inst bm_opti25[N];
extern toy_isa_inst bm_opti26[N];
extern toy_isa_inst bm_opti27[N];
