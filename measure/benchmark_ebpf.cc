#include "benchmark_ebpf.h"

using namespace std;

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
inst bm3[N3] = {inst(191, 1, 6, 0, 0),
                inst(183, 0, 1, 0, 0),
                inst(97, 6, 2, 36, 0),
                inst(86, 0, 2, 4, 6),
                inst(0, 0, 0, 0, 0), // call 7 modified as nop
                inst(188, 0, 1, 0, 0),
                inst(103, 0, 1, 0, 32),
                inst(119, 0, 1, 0, 32),
                inst(123, 1, 10, -40, 0),
                inst(97, 6, 1, 4, 0),
                inst(99, 1, 10, -32, 0),
                inst(97, 6, 1, 24, 0),
                inst(99, 1, 10, -16, 0),
                inst(180, 0, 8, 0, 0),
                inst(107, 8, 10, -26, 0),
                inst(97, 10, 1, -16, 0),
                inst(107, 1, 10, -28, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -40),
                inst(183, 0, 1, 0, 0),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(191, 0, 7, 0, 0),
                inst(21, 0, 7, 65, 0),
                inst(97, 7, 1, 0, 0),
                inst(99, 1, 10, -56, 0),
                inst(105, 7, 1, 4, 0),
                inst(107, 8, 10, -48, 0),
                inst(107, 8, 10, -50, 0),
                inst(107, 8, 10, -46, 0),
                inst(107, 1, 10, -52, 0),
                inst(22, 0, 1, 10, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -56),
                inst(183, 0, 1, 0, 1),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 2, 0),
                inst(105, 0, 1, 4, 0),
                inst(86, 0, 1, 10, 0),
                inst(180, 0, 1, 0, 0),
                inst(107, 1, 10, -52, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -56),
                inst(183, 0, 1, 0, 1),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 5, 0),
                inst(105, 0, 1, 4, 0),
                inst(22, 0, 1, 3, 0),
                inst(105, 7, 1, 6, 0),
                inst(105, 0, 2, 6, 0),
                inst(30, 1, 2, 20, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -40),
                inst(183, 0, 1, 0, 0),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 3),
                inst(183, 0, 6, 0, 0),
                inst(123, 6, 10, -8, 0),
                inst(123, 6, 10, -16, 0),
                inst(183, 0, 1, 0, 264),
                inst(123, 1, 10, -24, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -24),
                inst(183, 0, 1, 0, 2),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 9, 0),
                inst(121, 0, 1, 0, 0),
                inst(7, 0, 1, 0, 1),
                inst(123, 1, 0, 0, 0),
                inst(5, 0, 0, 16, 0),
                inst(97, 7, 1, 0, 0),
                inst(99, 1, 6, 4, 0),
                inst(105, 7, 1, 4, 0),
                inst(99, 1, 6, 24, 0),
                inst(5, 0, 0, 11, 0),
                inst(123, 6, 10, -8, 0),
                inst(183, 0, 1, 0, 1),
                inst(123, 1, 10, -16, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -24),
                inst(191, 10, 3, 0, 0),
                inst(7, 0, 3, 0, -16),
                inst(183, 0, 1, 0, 2),
                inst(0, 0, 0, 0),
                inst(180, 0, 4, 0, 0),
                inst(133, 0, 0, 0, 2),
                inst(180, 0, 0, 0, 1),
                inst(149, 0, 0, 0, 0),
               };

// r0 = r1 * 16
inst bm4[N4] = {inst(MOV64XY, 0, 1), // r0 = r1
                inst(ADD64XY, 0, 0), // r0 = r1 * 2
                inst(ADD64XY, 0, 0), // r0 = r1 * 4
                inst(ADD64XY, 0, 0), // r0 = r1 * 8
                inst(ADD64XY, 0, 0), // r0 = r1 * 16
                inst(),
                inst(),
               };

inst bm_opti40[N4] = {inst(MOV64XY, 0, 1), // r0 = r1
                      inst(LSH64XC, 0, 4), // r0 << 4
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };

// *(u32 *)pkt = 0
inst bm5[N5] = {inst(STB, 1, 0, 0), // *(u8 *)(pkt + 0) = 0
                inst(STB, 1, 1, 0), // *(u8 *)(pkt + 1) = 0
                inst(STB, 1, 2, 0), // *(u8 *)(pkt + 2) = 0
                inst(STB, 1, 3, 0), // *(u8 *)(pkt + 3) = 0
                inst(),
                inst(),
                inst(),
               };
inst bm_opti50[N5] = {inst(STW, 1, 0, 0), // *(u32 *)(pkt + 0) = 0
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
      inst::max_prog_len = N0;
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      return;
    case 1:
      inst::max_prog_len = N1;
      inst::add_sample_imm(vector<int32_t> {0xff0000});
      mem_t::add_map(map_attr(8, 8, N1));
      *bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      return;
    case 2:
      inst::max_prog_len = N2;
      *bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      return;
    case 3:
      inst::max_prog_len = N3;
      inst::add_sample_imm(vector<int32_t> {264});
      mem_t::set_pkt_sz(128);
      mem_t::add_map(map_attr(128, 64, N3)); // 8 items
      mem_t::add_map(map_attr(96, 96, N3));  // 12 items
      mem_t::add_map(map_attr(64, 128, N3)); // 16 items  => 36 items
      *bm = bm3;
      return;
    case 4:
      inst::max_prog_len = N4;
      *bm = bm4;
      bm_optis_orig.push_back(bm_opti40);
      return;
    case 5:
      inst::max_prog_len = N5;
      mem_t::set_pkt_sz(4);
      *bm = bm5;
      bm_optis_orig.push_back(bm_opti50);
      return;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
}
