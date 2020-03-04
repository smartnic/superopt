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
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti01[N] = {inst(MOV32XC, 0, 0x2),  /* mov32 r0, 0x2 */
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti02[N] = {inst(ADD64XC, 0, 0x2),  /* add64 r0, 0x2 */
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti03[N] = {inst(ADD32XC, 0, 0x2),  /* add32 r0, 0x2 */
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };

void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      return;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0}" << endl;
      return;
  }
}
