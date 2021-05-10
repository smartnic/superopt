#pragma once
#include "../src/isa/ebpf/inst.h"

using namespace std;

inst program[] = {inst(MOV64XC, 0, 0),    // 0: r0 = 0
                  inst(STXH, 10, -2, 0),  // 1: *(u16 *)(r10 - 2) = r0
                  inst(MOV64XC, 0, 1),    // 2: r0 = 1
                  inst(STXB, 10, -5, 0),  // 3: *(u8 *)(r10 - 5) = r0
                  INSN_LDMAPID(1, 0),     // 4-5: r1 = map0 fd
                  inst(MOV64XY, 2, 10),   // 6: r2 = r10
                  inst(ADD64XC, 2, -2),   // 7: r2 += -2
                  inst(CALL, BPF_FUNC_map_lookup_elem),  // 8: r0 = lookup v map0, where v = *(u16*)r2
                  inst(JEQXC, 0, 0, 1),   // 9: if r0 = 0, jmp 1 (exit)
                  inst(MOV64XC, 0, 1),    // 10: r0 = 1
                  inst(EXIT),             // 11: exit, return r0
                 };
