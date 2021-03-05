#pragma once

#include "inst.h"
#include "canonicalize.h"

using namespace std;

bool insn_satisfy_isa_win_constraints(const inst& insn, const inst_static_state& iss);
