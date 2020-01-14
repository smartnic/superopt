#pragma once

#include "z3++.h"
#include "../../src/utils.h"

using namespace std;

// return (b = op a)
int64_t compute_mov(int64_t a, int64_t b = 0);
int64_t compute_le16(int64_t a, int64_t b = 0);
int64_t compute_le32(int64_t a, int64_t b = 0);
int64_t compute_le64(int64_t a, int64_t b = 0);
int64_t compute_be16(int64_t a, int64_t b = 0);
int64_t compute_be32(int64_t a, int64_t b = 0);
int64_t compute_be64(int64_t a, int64_t b = 0);

// return (c = a op b)
int64_t compute_add(int64_t a, int64_t b, int64_t c = 0);
int64_t compute_add(int64_t a, int32_t b, int64_t c = 0);
int32_t compute_add(int32_t a, int32_t b, int32_t c = 0);
int64_t compute_lsh(int64_t a, int64_t b, int64_t c = 0);
int64_t compute_lsh(int64_t a, int32_t b, int64_t c = 0);
int32_t compute_lsh(int32_t a, int32_t b, int32_t c = 0);
int64_t compute_rsh(int64_t a, int64_t b, int64_t c = 0);
int64_t compute_rsh(int64_t a, int32_t b, int64_t c = 0);
int32_t compute_rsh(int32_t a, int32_t b, int32_t c = 0);
int64_t compute_arsh(int64_t a, int64_t b, int64_t c = 0);
int64_t compute_arsh(int64_t a, int32_t b, int64_t c = 0);
int32_t compute_arsh(int32_t a, int32_t b, int32_t c = 0);
// return max(a, b)
int64_t compute_max(int64_t a, int64_t b, int64_t c = 0);
// return c == a + b
z3::expr predicate_add(z3::expr a, z3::expr b, z3::expr c);
//return b == a
z3::expr predicate_mov(int a, z3::expr b);
// return c == max(a, b)
z3::expr predicate_max(z3::expr a, int b, z3::expr c);
z3::expr predicate_max(z3::expr a, z3::expr b, z3::expr c);
