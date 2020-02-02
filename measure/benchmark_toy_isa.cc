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
inst_t bm0[N] = {inst_t(MOVXC, 2, 4),  /* mov r2, 4  */
                 inst_t(ADDXY, 0, 2),  /* add r0, r2 */
                 inst_t(MOVXC, 3, 15),  /* mov r3, 15  */
                 inst_t(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                 inst_t(RETX, 3),      /* ret r3 */
                 inst_t(RETX, 0),      /* else ret r0 */
                 inst_t(),  /* control never reaches here */
                };
// f(x) = max(2*x, x+4)
// perf_cost = 3 + 1 = 4
inst_t bm1[N] = {inst_t(ADDXY, 1, 0),
                 inst_t(MOVXC, 2, 4),
                 inst_t(ADDXY, 1, 2), // r1 = r0+4
                 inst_t(ADDXY, 0, 0), // r0 += r0
                 inst_t(MAXX, 1, 0),  // r1 = max(r1, r0)
                 inst_t(RETX, 1),
                 inst_t(),
                };
// f(x) = 6*x
// perf_cost = 4 + 0 = 4
inst_t bm2[N] = {inst_t(MOVXC, 1, 0),
                 inst_t(ADDXY, 1, 0), // r1 = 2*r0
                 inst_t(ADDXY, 0, 1),
                 inst_t(ADDXY, 0, 1),
                 inst_t(ADDXY, 0, 1),
                 inst_t(ADDXY, 0, 1),
                 inst_t(ADDXY, 0, 1),
                };
inst_t bm_opti00[N] = {inst_t(MOVXC, 1, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(MAXC, 0, 15),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti01[N] = {inst_t(MAXC, 1, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(MAXC, 0, 15),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti02[N] = {inst_t(MAXC, 1, 4),
                       inst_t(MAXC, 0, 11),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti03[N] = {inst_t(MOVXC, 1, 4),
                       inst_t(MAXC, 0, 11),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti04[N] = {inst_t(MAXC, 0, 11),
                       inst_t(MOVXC, 1, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti05[N] = {inst_t(MAXC, 0, 11),
                       inst_t(MAXC, 1, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti10[N] = {inst_t(MOVXC, 1, 4),
                       inst_t(MAXX, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti11[N] = {inst_t(MAXC, 1, 4),
                       inst_t(MAXX, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti12[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(MAXC, 0, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti13[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(MAXC, 1, 4),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti20[N] = {inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti21[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 1, 1),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 0),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti22[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 0),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti23[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 0),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti24[N] = {inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(ADDXY, 0, 0),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti25[N] = {inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti26[N] = {inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
inst_t bm_opti27[N] = {inst_t(ADDXY, 0, 0),
                       inst_t(ADDXY, 1, 0),
                       inst_t(ADDXY, 1, 1),
                       inst_t(ADDXY, 0, 1),
                       inst_t(),
                       inst_t(),
                       inst_t(),
                      };
