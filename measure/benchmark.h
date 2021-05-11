#pragma once
#include "../src/isa/ebpf/inst.h"

using namespace std;

inst program[] = {inst(MOV64XC, 0, 0x1),  // 0: r0 = 0x1
                  inst(ADD64XY, 0, 0),    // 1: r0 += r0
                  inst(EXIT),             // 2: exit, return r0
                 };
