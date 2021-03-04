#pragma once

#include "../../src/isa/inst_header.h"

using namespace std;

void gen_wins(vector<pair<int, int>>& wins, inst* pgm, int len, prog_static_state& pss);
