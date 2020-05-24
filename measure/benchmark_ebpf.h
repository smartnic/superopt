#pragma once

#include <vector>
#include "../src/isa/ebpf/inst.h"

using namespace std;
void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id);
