#pragma once

#include <vector>
#include <ostream>
#include "../inst.h"

using namespace std;

void gen_random_input(vector<int>& inputs, int min, int max);
ostream& operator<<(ostream& out, vector<int>& v);
ostream& operator<<(ostream& out, vector<vector<int> >& v);


// instruction_list set
#define N MAX_PROG_LEN
extern inst orig0[N];
extern inst orig2[N];
extern inst orig3[N];
extern inst opti00[N];
extern inst opti01[N];
extern inst opti02[N];
extern inst opti03[N];
extern inst opti04[N];
extern inst opti05[N];
extern inst opti20[N];
extern inst opti21[N];
extern inst opti22[N];
extern inst opti30[N];
extern inst opti31[N];
extern inst opti32[N];
extern inst opti33[N];
extern inst opti34[N];
extern inst opti35[N];
extern inst opti36[N];
extern inst opti37[N];
extern inst opti38[N];
