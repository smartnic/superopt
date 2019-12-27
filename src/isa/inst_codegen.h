#pragma once

#include "z3++.h"

using namespace std;

// return a + b
int compute_add(int a, int b, int c);
// return a
int compute_mov(int a, int b);
// return max(a, b)
int compute_max(int a, int b, int c);
// return c == a + b
z3::expr predicate_add(z3::expr a, z3::expr b, z3::expr c);
//return b == a
z3::expr predicate_mov(int a, z3::expr b);
// return c == max(a, b)
z3::expr predicate_max(z3::expr a, int b, z3::expr c);
z3::expr predicate_max(z3::expr a, z3::expr b, z3::expr c);
