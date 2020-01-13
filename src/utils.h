#pragma once

#include <vector>
#include <iostream>

using namespace std;

#define NOW chrono::steady_clock::now()
#define DUR(t1, t2) chrono::duration <double, micro> (t2 - t1).count()

#define H32(v) (0xffffffff00000000 & v)
#define H48(v) (0xffffffffffff0000 & v)
#define L16(v) (0x000000000000ffff & v)
#define L32(v) (0x00000000ffffffff & v)

void print_test_res(bool res, string test_name);
void gen_random_input(vector<int>& inputs, int min, int max);
ostream& operator<<(ostream& out, const vector<double>& vec);
void split_string(const string& s, vector<string>& v, const string& c);
unsigned int pop_count_asm(unsigned int x);
bool is_little_endian();
