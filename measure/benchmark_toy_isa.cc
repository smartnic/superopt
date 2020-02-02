#include "benchmark_toy_isa.h"

using namespace std;

ostream& operator<<(ostream& out, vector<int>& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << v[i] << " ";
  }
  return out;
}

ostream& operator<<(ostream& out, vector<vector<int> >& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << i << ": " << v[i] << endl;
  }
  return out;
}
// output = max(input+4, 15)
// perf_cost = 3 + 1 = 4
inst bm0[N] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
               inst(ADDXY, 0, 2),  /* add r0, r2 */
               inst(MOVXC, 3, 15),  /* mov r3, 15  */
               inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
               inst(RETX, 3),      /* ret r3 */
               inst(RETX, 0),      /* else ret r0 */
               inst(),  /* control never reaches here */
              };
// f(x) = max(2*x, x+4)
// perf_cost = 3 + 1 = 4
inst bm1[N] = {inst(ADDXY, 1, 0),
               inst(MOVXC, 2, 4),
               inst(ADDXY, 1, 2), // r1 = r0+4
               inst(ADDXY, 0, 0), // r0 += r0
               inst(MAXX, 1, 0),  // r1 = max(r1, r0)
               inst(RETX, 1),
               inst(),
              };
// f(x) = 6*x
// perf_cost = 4 + 0 = 4
inst bm2[N] = {inst(MOVXC, 1, 0),
               inst(ADDXY, 1, 0), // r1 = 2*r0
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
               inst(ADDXY, 0, 1),
              };
inst bm_opti00[N] = {inst(MOVXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(MAXC, 0, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti01[N] = {inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(MAXC, 0, 15),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti02[N] = {inst(MAXC, 1, 4),
                     inst(MAXC, 0, 11),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti03[N] = {inst(MOVXC, 1, 4),
                     inst(MAXC, 0, 11),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti04[N] = {inst(MAXC, 0, 11),
                     inst(MOVXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti05[N] = {inst(MAXC, 0, 11),
                     inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti10[N] = {inst(MOVXC, 1, 4),
                     inst(MAXX, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti11[N] = {inst(MAXC, 1, 4),
                     inst(MAXX, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti12[N] = {inst(ADDXY, 1, 0),
                     inst(MAXC, 0, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti13[N] = {inst(ADDXY, 1, 0),
                     inst(MAXC, 1, 4),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti20[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti21[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 1),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti22[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti23[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti24[N] = {inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 0),
                     inst(ADDXY, 0, 1),
                     inst(ADDXY, 0, 0),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti25[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti26[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 0, 0),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
inst bm_opti27[N] = {inst(ADDXY, 0, 0),
                     inst(ADDXY, 1, 0),
                     inst(ADDXY, 1, 1),
                     inst(ADDXY, 0, 1),
                     inst(),
                     inst(),
                     inst(),
                    };
