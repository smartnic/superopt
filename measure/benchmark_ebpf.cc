#include "benchmark_ebpf.h"

using namespace std;

inst bm0[N] = {inst(MOV64XC, 0, 0x1),  /* mov64 r0, 0x1 */
               inst(ADD64XY, 0, 0),  /* add64 r0, r0 */
               inst(EXIT),  /* exit, return r0 */
               inst(),
               inst(),
               inst(),
               inst(),
              };

inst bm_opti00[N] = {inst(MOV64XC, 0, 0x2),  /* mov64 r0, 0x2 */
                     inst(EXIT),  /* exit, return r0 */
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
