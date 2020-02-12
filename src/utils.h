#pragma once

#include <vector>
#include <iostream>

using namespace std;

#if ISA_TOY_ISA
// register type, also used as input/output type
// since input/output are assigned to/from registers
typedef int reg_t;
typedef int op_t;
// number of register bits, used by smt_var.h/cc, cost.cc
#define NUM_REG_BITS 32
#elif ISA_EBPF
typedef int64_t reg_t;
typedef int32_t op_t;
#define NUM_REG_BITS 64
#else
typedef int reg_t;
typedef int op_t;
#define NUM_REG_BITS 32
#endif

#define NOW chrono::steady_clock::now()
#define DUR(t1, t2) chrono::duration <double, micro> (t2 - t1).count()

#define H32(v) (0xffffffff00000000 & v)
#define H48(v) (0xffffffffffff0000 & v)
#define L5(v)  (0x000000000000001f & v)
#define L6(v)  (0x000000000000003f & v)
#define L16(v) (0x000000000000ffff & v)
#define L32(v) (0x00000000ffffffff & v)

void print_test_res(bool res, string test_name);
void gen_random_input(vector<reg_t>& inputs, reg_t min, reg_t max);
ostream& operator<<(ostream& out, const vector<double>& vec);
void split_string(const string& s, vector<string>& v, const string& c);
unsigned int pop_count_asm(unsigned int x);
bool is_little_endian();
