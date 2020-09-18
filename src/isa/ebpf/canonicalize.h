#pragma once

#include <unordered_set>
#include "inst.h"

using namespace std;

void liveness_analysis(unordered_set<int>& live_regs, inst* program, int start, int end,
                       const unordered_set<int>& initial_live_regs);