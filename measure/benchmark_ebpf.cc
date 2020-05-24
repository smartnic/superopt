#include "benchmark_ebpf.h"

using namespace std;

// N can not greater than 56 because of the limit of combination function
#undef N0
#undef N1
#undef N2
#define N0 7
#define N1 7
#define N2 16
inst bm0[N0] = {inst(MOV64XC, 0, 0x1),  /* mov64 r0, 0x1 */
                inst(ADD64XY, 0, 0),  /* add64 r0, r0 */
                inst(EXIT),  /* exit, return r0 */
                inst(),
                inst(),
                inst(),
                inst(),
               };
inst bm_opti00[N0] = {inst(MOV64XC, 0, 0x2),  /* mov64 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti01[N0] = {inst(MOV32XC, 0, 0x2),  /* mov32 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti02[N0] = {inst(ADD64XC, 0, 0x2),  /* add64 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
inst bm_opti03[N0] = {inst(ADD32XC, 0, 0x2),  /* add32 r0, 0x2 */
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };

// w0 = (w1 >> 16) | ((w1 << 16) & 0xff0000)
inst bm1[N1] = {inst(MOV32XY, 2, 1), // w2 = w1
                inst(RSH32XC, 2, 16), // w2 >>= 16
                inst(LSH32XC, 1, 16), // w1 <<= 16
                inst(AND32XC, 1, 0xff0000), // w1 &= 0xff0000
                inst(OR32XY, 1, 2), // w1 |= w2
                inst(MOV32XY, 0, 1), // w0 = w1
                inst(EXIT),
               };

inst bm_opti10[N1] = {inst(MOV32XY, 0, 1), // w0 = w1
                      inst(RSH32XC, 0, 16), // w0 >>= 16
                      inst(LSH32XC, 1, 16), // w1 <<= 16
                      inst(AND32XC, 1, 0xff0000), // w1 &= 0xff0000
                      inst(OR32XY, 0, 1), // w0 |= w1
                      inst(),
                      inst(),
                     };

inst bm2[N2] = {inst(STXB, 10, -2, 1),    // *(r10-2) = L8(input)
                inst(MOV64XC, 1, 0x01),   // *(r10-1) = 0x01
                inst(STXB, 10, -1, 1),
                inst(MOV64XC, 1, 0),      // r1 = map0
                inst(MOV64XY, 2, 10),     // r2 = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup), // r0 = &map0[0x01]
                inst(JEQXC, 0, 0, 7),   // if r0 == 0, exit else map0[0x01] = L8(input)
                inst(MOV64XC, 1, 0),    // r1 = map0
                inst(MOV64XY, 2, 10),   // r2 = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(MOV64XY, 3, 10),   // r3 = r10 - 2
                inst(ADD64XC, 3, -2),
                inst(CALL, BPF_FUNC_map_update), // r0 = addr_v1
                inst(MOV64XC, 0, 0),
                inst(EXIT),
               };
inst bm_opti20[N2] = {inst(STXB, 10, -2, 1),    // *(r10-2) = L8(input)
                      inst(MOV64XC, 1, 0x01),   // *(r10-1) = 0x01
                      inst(STXB, 10, -1, 1),
                      inst(MOV64XC, 1, 0),      // r1 = map0
                      inst(MOV64XY, 2, 10),     // r2 = r10 - 1
                      inst(ADD64XC, 2, -1),
                      inst(CALL, BPF_FUNC_map_lookup), // r0 = &map0[0x01]
                      inst(JEQXC, 0, 0, 7),   // if r0 == 0, exit
                      inst(LDXB, 1, 10, -2),  // r1 = L8(input)
                      inst(STXB, 0, 0, 1),    // *r0 = r1
                      inst(MOV64XC, 0, 0),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };

void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      inst::max_prog_len = N0;
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      return;
    case 1:
      inst::max_prog_len = N1;
      mem_t::add_map(map_attr(8, 8, N1));
      *bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      return;
    case 2:
      inst::max_prog_len = N2;
      *bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      return;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
}
