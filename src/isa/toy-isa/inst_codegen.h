#pragma once

#include "z3++.h"
#include "../../../src/utils.h"
#include "../../../src/verify/smt_var.h"

using namespace std;

// return (b = op a)
int64_t toy_isa_compute_mov(int32_t a, int64_t b = 0);
// return (c = a op b)
int64_t toy_isa_compute_add(int64_t a, int64_t b, int64_t c = 0);
// return max(a, b)
int64_t toy_isa_compute_max(int64_t a, int64_t b, int64_t c = 0);

// return (b == op a)
z3::expr toy_isa_predicate_mov(z3::expr a, z3::expr b);
// return (c == a op b)
z3::expr toy_isa_predicate_add(z3::expr a, z3::expr b, z3::expr c);
// return (c == max(a, b))
z3::expr toy_isa_predicate_max(z3::expr a, z3::expr b, z3::expr c);
