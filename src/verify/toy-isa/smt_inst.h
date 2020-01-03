#pragma once

#include "z3++.h"
#include "../../../src/isa/inst_codegen.h"
#include "../../../src/isa/inst.h"
#include "../../../src/isa/toy-isa/inst.h"
#include "../smt_var.h"

using namespace z3;

// return SMT for the given CFG_OTHERS type instruction, other types return false
expr smt_inst(smt_var& sv, const inst& in);
expr smt_inst_jmp(smt_var& sv, const inst& in);
