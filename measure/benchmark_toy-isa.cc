#include "benchmark_toy-isa.h"

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
toy_isa_inst bm0[N] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),  /* mov r2, 4  */
                       toy_isa_inst(toy_isa::ADDXY, 0, 2),  /* add r0, r2 */
                       toy_isa_inst(toy_isa::MOVXC, 3, 15),  /* mov r3, 15  */
                       toy_isa_inst(toy_isa::JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                       toy_isa_inst(toy_isa::RETX, 3),      /* ret r3 */
                       toy_isa_inst(toy_isa::RETX, 0),      /* else ret r0 */
                       toy_isa_inst(),  /* control never reaches here */
                      };
// f(x) = max(2*x, x+4)
// perf_cost = 3 + 1 = 4
toy_isa_inst bm1[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                       toy_isa_inst(toy_isa::MOVXC, 2, 4),
                       toy_isa_inst(toy_isa::ADDXY, 1, 2), // r1 = r0+4
                       toy_isa_inst(toy_isa::ADDXY, 0, 0), // r0 += r0
                       toy_isa_inst(toy_isa::MAXX, 1, 0),  // r1 = max(r1, r0)
                       toy_isa_inst(toy_isa::RETX, 1),
                       toy_isa_inst(),
                      };
// f(x) = 6*x
// perf_cost = 4 + 0 = 4
toy_isa_inst bm2[N] = {toy_isa_inst(toy_isa::MOVXC, 1, 0),
                       toy_isa_inst(toy_isa::ADDXY, 1, 0), // r1 = 2*r0
                       toy_isa_inst(toy_isa::ADDXY, 0, 1),
                       toy_isa_inst(toy_isa::ADDXY, 0, 1),
                       toy_isa_inst(toy_isa::ADDXY, 0, 1),
                       toy_isa_inst(toy_isa::ADDXY, 0, 1),
                       toy_isa_inst(toy_isa::ADDXY, 0, 1),
                      };
toy_isa_inst bm_opti00[N] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::MAXC, 0, 15),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti01[N] = {toy_isa_inst(toy_isa::MAXC, 1, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::MAXC, 0, 15),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti02[N] = {toy_isa_inst(toy_isa::MAXC, 1, 4),
                             toy_isa_inst(toy_isa::MAXC, 0, 11),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti03[N] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),
                             toy_isa_inst(toy_isa::MAXC, 0, 11),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti04[N] = {toy_isa_inst(toy_isa::MAXC, 0, 11),
                             toy_isa_inst(toy_isa::MOVXC, 1, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti05[N] = {toy_isa_inst(toy_isa::MAXC, 0, 11),
                             toy_isa_inst(toy_isa::MAXC, 1, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti10[N] = {toy_isa_inst(toy_isa::MOVXC, 1, 4),
                             toy_isa_inst(toy_isa::MAXX, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti11[N] = {toy_isa_inst(toy_isa::MAXC, 1, 4),
                             toy_isa_inst(toy_isa::MAXX, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti12[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::MAXC, 0, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti13[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::MAXC, 1, 4),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti20[N] = {toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti21[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti22[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti23[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti24[N] = {toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti25[N] = {toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti26[N] = {toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
toy_isa_inst bm_opti27[N] = {toy_isa_inst(toy_isa::ADDXY, 0, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 0),
                             toy_isa_inst(toy_isa::ADDXY, 1, 1),
                             toy_isa_inst(toy_isa::ADDXY, 0, 1),
                             toy_isa_inst(),
                             toy_isa_inst(),
                             toy_isa_inst(),
                            };
