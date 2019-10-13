#include "inst.h"
#include "inout.h"

using namespace std;

unsigned int pop_count_asm(unsigned int);
int error_cost(inout*, int, inst*, inst*, bool=false);
int perf_cost(inout*, int, inst*, inst*);
