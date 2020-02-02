#include "benchmark_ebpf.h"

using namespace std;

inst_t bm0[N] = {inst_t(MOV64XC, 0, 0x1),  /* mov64 r0, 0x1 */
                 inst_t(ADD64XY, 0, 0),  /* add64 r0, r0 */
                 inst_t(EXIT),  /* exit, return r0 */
                 inst_t(),
                 inst_t(),
                 inst_t(),
                 inst_t(),
                };

inst_t bm_opti00[N] = {inst_t(MOV64XC, 0, 0x2),  /* mov64 r0, 0x2 */
                       inst_t(EXIT),  /* exit, return r0 */
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
