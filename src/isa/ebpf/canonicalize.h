#pragma once

#include <unordered_set>
#include "inst.h"

using namespace std;

void canonicalize(inst* program, int len);

void remove_nops(inst* program, int len);
