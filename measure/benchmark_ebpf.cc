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
                inst(LDMAPID, 1, 0),      // r1 = map0
                inst(MOV64XY, 2, 10),     // r2 = r10 - 1
                inst(ADD64XC, 2, -1),
                inst(CALL, BPF_FUNC_map_lookup), // r0 = &map0[0x01]
                inst(JEQXC, 0, 0, 7),   // if r0 == 0, exit else map0[0x01] = L8(input)
                inst(LDMAPID, 1, 0),    // r1 = map0
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
                      inst(LDMAPID, 1, 0),      // r1 = map0
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
                inst(MOV32XY, 0, 10), // call 7 modified as w0 = r10
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
                inst(LDMAPID, 1, 0),
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
                inst(LDMAPID, 1, 1),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 2, 0),
                inst(105, 0, 1, 4, 0),
                inst(86, 0, 1, 10, 0),
                inst(180, 0, 1, 0, 0),
                inst(107, 1, 10, -52, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -56),
                inst(LDMAPID, 1, 1),
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
                inst(LDMAPID, 1, 0),
                inst(0, 0, 0, 0),
                inst(133, 0, 0, 0, 3),
                inst(183, 0, 6, 0, 0),
                inst(123, 6, 10, -8, 0),
                inst(123, 6, 10, -16, 0),
                inst(183, 0, 1, 0, 264),
                inst(123, 1, 10, -24, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -24),
                inst(LDMAPID, 1, 2),
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
                inst(LDMAPID, 1, 2),
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
                inst(MOV64XC, 0, 0),
                inst(),
                inst(),
               };
inst bm_opti50[N5] = {inst(STW, 1, 0, 0), // *(u32 *)(pkt + 0) = 0
                      inst(MOV64XC, 0, 0),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
// r0 = L32(r1)
inst bm6[N6] = {inst(MOV32XY, 0, 1),
                inst(LSH64XC, 0, 32),
                inst(RSH64XC, 0, 32),
                inst(),
                inst(),
                inst(),
                inst(),
               };
inst bm_opti60[N6] = {inst(MOV32XY, 0, 1),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
// r0 = r1
inst bm7[N7] = {inst(MOV64XY, 0, 1),
                inst(STXB, 10, -1, 1),
                inst(STXB, 10, -2, 1),
                inst(),
                inst(),
                inst(),
                inst(),
               };
inst bm_opti70[N7] = {inst(MOV64XY, 0, 1),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                      inst(),
                     };
// sampleip_kern
inst bm8[N8] = {inst(183, 0, 2, 0, 1),
                inst(99, 2, 10, -12, 0),
                inst(121, 1, 1, 128, 0),
                inst(123, 1, 10, -8, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -8),
                inst(LDMAPID, 1, 0),
                inst(0, 0, 0, 0, 0),
                inst(133, 0, 0, 0, 1),
                inst(21, 0, 0, 4, 0),
                inst(97, 0, 1, 0, 0),
                inst(7, 0, 1, 0, 1),
                inst(99, 1, 0, 0, 0),
                inst(5, 0, 0, 8, 0),
                inst(191, 10, 2, 0, 0),
                inst(7, 0, 2, 0, -8),
                inst(191, 10, 3, 0, 0),
                inst(7, 0, 3, 0, -12),
                inst(LDMAPID, 1, 0),
                inst(0, 0, 0, 0, 0),
                inst(183, 0, 4, 0, 1),
                inst(133, 0, 0, 0, 2),
                inst(183, 0, 0, 0, 0),
                inst(149, 0, 0, 0, 0),
               };
// code segment from xdp1_kern
inst bm9[N9] = {inst(LDXB, 3, 1, 0), // r3 = *(u8 *)(r1 + 0)
                inst(LDXB, 4, 1, 1), // r4 = *(u8 *)(r1 + 1)
                inst(LSH64XC, 4, 8), // r4 <<= 8
                inst(OR64XY, 4, 3),  // r4 |= r3
                inst(MOV64XY, 0, 4), // r0 = r4
                inst(),
                inst(),
               };
// code segment from xdp1_kern
inst bm10[N10] = {inst(LDXH, 2, 1, 0),
                  inst(LDXH, 3, 1, 6),
                  inst(STXH, 1, 0, 3),
                  inst(LDXH, 3, 1, 8),
                  inst(LDXH, 4, 1, 2),
                  inst(STXH, 1, 8, 4),
                  inst(STXH, 1, 2, 3),
                  inst(LDXH, 3, 1, 10),
                  inst(LDXH, 4, 1, 4),
                  inst(STXH, 1, 10, 4),
                  inst(STXH, 1, 4, 3),
                  inst(STXH, 1, 6, 2),
                  inst(MOV64XC, 0, 0),
                 };
// syscall_tp_kern
inst bm11[N11] = {inst(183, 0, 1, 0, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(183, 0, 1, 0, 1),
                  inst(99, 1, 10, -8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(LDMAPID, 1, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 4, 0),
                  inst(97, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(99, 1, 0, 0, 0),
                  inst(5, 0, 0, 8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(191, 10, 3, 0, 0),
                  inst(7, 0, 3, 0, -8),
                  inst(LDMAPID, 1, 0),
                  inst(),
                  inst(183, 0, 4, 0, 1),
                  inst(133, 0, 0, 0, 2),
                  inst(183, 0, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp1_kern
inst bm12[N12] = {inst(97, 1, 2, 4, 0),
                  inst(97, 1, 1, 0, 0),
                  inst(191, 1, 3, 0, 0),
                  inst(7, 0, 3, 0, 14),
                  inst(45, 2, 3, 54, 0),
                  inst(113, 1, 3, 12, 0),
                  inst(113, 1, 4, 13, 0),
                  inst(103, 0, 4, 0, 8),
                  inst(79, 3, 4, 0, 0),
                  inst(21, 0, 4, 2, 43144),
                  inst(183, 0, 3, 0, 14),
                  inst(85, 0, 4, 5, 129),
                  inst(191, 1, 3, 0, 0),
                  inst(7, 0, 3, 0, 18),
                  inst(45, 2, 3, 44, 0),
                  inst(183, 0, 3, 0, 18),
                  inst(105, 1, 4, 16, 0),
                  inst(191, 4, 5, 0, 0),
                  inst(87, 0, 5, 0, 65535),
                  inst(21, 0, 5, 1, 43144),
                  inst(85, 0, 5, 9, 129),
                  inst(191, 3, 5, 0, 0),
                  inst(7, 0, 5, 0, 4),
                  inst(191, 1, 4, 0, 0),
                  inst(15, 5, 4, 0, 0),
                  inst(45, 2, 4, 33, 0),
                  inst(191, 1, 4, 0, 0),
                  inst(15, 3, 4, 0, 0),
                  inst(105, 4, 4, 2, 0),
                  inst(191, 5, 3, 0, 0),
                  inst(87, 0, 4, 0, 65535),
                  inst(21, 0, 4, 8, 56710),
                  inst(85, 0, 4, 15, 8),
                  inst(15, 3, 1, 0, 0),
                  inst(183, 0, 3, 0, 0),
                  inst(191, 1, 4, 0, 0),
                  inst(7, 0, 4, 0, 20),
                  inst(45, 2, 4, 8, 0),
                  inst(113, 1, 3, 9, 0),
                  inst(5, 0, 0, 6, 0),
                  inst(15, 3, 1, 0, 0),
                  inst(183, 0, 3, 0, 0),
                  inst(191, 1, 4, 0, 0),
                  inst(7, 0, 4, 0, 40),
                  inst(45, 2, 4, 1, 0),
                  inst(113, 1, 3, 6, 0),
                  inst(99, 3, 10, -4, 0),
                  inst(5, 0, 0, 2, 0),
                  inst(183, 0, 1, 0, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0), // ldmapid
                  inst(0, 0, 0, 0, 0),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 3, 0),
                  inst(121, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 0, 0),
                  inst(183, 0, 0, 0, 1),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp_monitor_kern, section: tracepoint/xdp/xdp_devmap_xmit
inst bm13[N13] = {inst(191, 1, 6, 0, 0),
                  inst(183, 0, 7, 0, 0),
                  inst(99, 7, 10, -4, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 25, 0),
                  inst(97, 6, 1, 24, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(199, 0, 1, 0, 32),
                  inst(121, 0, 2, 0, 0),
                  inst(15, 1, 2, 0, 0),
                  inst(123, 2, 0, 0, 0),
                  inst(121, 0, 2, 16, 0),
                  inst(7, 0, 2, 0, 1),
                  inst(97, 6, 1, 20, 0),
                  inst(123, 2, 0, 16, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(199, 0, 1, 0, 32),
                  inst(121, 0, 2, 8, 0),
                  inst(15, 1, 2, 0, 0),
                  inst(123, 2, 0, 8, 0),
                  inst(97, 6, 2, 36, 0),
                  inst(21, 0, 2, 3, 0),
                  inst(121, 0, 2, 24, 0),
                  inst(7, 0, 2, 0, 1),
                  inst(123, 2, 0, 24, 0),
                  inst(183, 0, 7, 0, 1),
                  inst(101, 0, 1, 3, -1),
                  inst(121, 0, 1, 24, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 24, 0),
                  inst(191, 7, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp_monitor_kern, xdp_cpumap_kthread
inst bm14[N14] = {inst(191, 1, 6, 0, 0),
                  inst(183, 0, 1, 0, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 13, 0),
                  inst(97, 6, 1, 24, 0),
                  inst(121, 0, 2, 0, 0),
                  inst(15, 1, 2, 0, 0),
                  inst(123, 2, 0, 0, 0),
                  inst(121, 0, 1, 8, 0),
                  inst(97, 6, 2, 20, 0),
                  inst(15, 2, 1, 0, 0),
                  inst(123, 1, 0, 8, 0),
                  inst(97, 6, 1, 28, 0),
                  inst(21, 0, 1, 3, 0),
                  inst(121, 0, 1, 16, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 16, 0),
                  inst(183, 0, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp_monitor_kern, xdp_redirect_err
inst bm15[N15] = {inst(97, 1, 2, 20, 0),
                  inst(183, 0, 6, 0, 1),
                  inst(183, 0, 1, 0, 1),
                  inst(85, 0, 2, 1, 0),
                  inst(183, 0, 1, 0, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 4, 0),
                  inst(121, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 0, 0),
                  inst(183, 0, 6, 0, 0),
                  inst(191, 6, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp_monitor_kern, xdp_exception
inst bm16[N16] = {inst(97, 1, 1, 12, 0),
                  inst(183, 0, 2, 0, 5),
                  inst(45, 1, 2, 1, 0),
                  inst(183, 0, 1, 0, 5),
                  inst(99, 1, 10, -4, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(183, 0, 1, 0, 1),
                  inst(21, 0, 0, 4, 0),
                  inst(121, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(123, 1, 0, 0, 0),
                  inst(183, 0, 1, 0, 0),
                  inst(191, 1, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// xdp_monitor_kern, xdp_cpumap_enqueue
inst bm17[N17] = {inst(191, 1, 6, 0, 0),
                  inst(183, 0, 0, 0, 1),
                  inst(97, 6, 1, 28, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(37, 0, 1, 20, 63),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(191, 0, 1, 0, 0),
                  inst(183, 0, 0, 0, 0),
                  inst(21, 0, 1, 12, 0),
                  inst(97, 6, 2, 24, 0),
                  inst(121, 1, 3, 0, 0),
                  inst(15, 2, 3, 0, 0),
                  inst(123, 3, 1, 0, 0),
                  inst(97, 6, 3, 20, 0),
                  inst(121, 1, 4, 8, 0),
                  inst(15, 3, 4, 0, 0),
                  inst(123, 4, 1, 8, 0),
                  inst(21, 0, 2, 3, 0),
                  inst(121, 1, 2, 16, 0),
                  inst(7, 0, 2, 0, 1),
                  inst(123, 2, 1, 16, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// syscall_tp_kern, section: tracepoint/syscalls/sys_enter_open
inst bm18[N18] = {inst(183, 0, 1, 0, 0),
                  inst(99, 1, 10, -4, 0),
                  inst(183, 0, 1, 0, 1),
                  inst(99, 1, 10, -8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 4, 0),
                  inst(97, 0, 1, 0, 0),
                  inst(7, 0, 1, 0, 1),
                  inst(99, 1, 0, 0, 0),
                  inst(5, 0, 0, 8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -4),
                  inst(191, 10, 3, 0, 0),
                  inst(7, 0, 3, 0, -8),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(183, 0, 4, 0, 1),
                  inst(133, 0, 0, 0, 2),
                  inst(183, 0, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
// lwt_len_hist_kern
inst bm19[N19] = {inst(183, 0, 2, 0, 1),
                  inst(123, 2, 10, -16, 0),
                  inst(97, 1, 1, 0, 0),
                  inst(183, 0, 4, 0, 1),
                  inst(37, 0, 1, 1, 65535),
                  inst(183, 0, 4, 0, 0),
                  inst(103, 0, 4, 0, 4),
                  inst(127, 4, 1, 0, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(119, 0, 1, 0, 32),
                  inst(183, 0, 3, 0, 1),
                  inst(37, 0, 1, 1, 255),
                  inst(183, 0, 3, 0, 0),
                  inst(103, 0, 3, 0, 3),
                  inst(127, 3, 1, 0, 0),
                  inst(79, 4, 3, 0, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(119, 0, 1, 0, 32),
                  inst(183, 0, 4, 0, 1),
                  inst(37, 0, 1, 1, 15),
                  inst(183, 0, 4, 0, 0),
                  inst(103, 0, 4, 0, 2),
                  inst(79, 4, 3, 0, 0),
                  inst(127, 4, 1, 0, 0),
                  inst(103, 0, 1, 0, 32),
                  inst(119, 0, 1, 0, 32),
                  inst(37, 0, 1, 1, 3),
                  inst(183, 0, 2, 0, 0),
                  inst(103, 0, 2, 0, 1),
                  inst(79, 2, 3, 0, 0),
                  inst(127, 2, 1, 0, 0),
                  inst(24, 0, 2, 0, -2),
                  inst(95, 2, 1, 0, 0),
                  inst(119, 0, 1, 0, 1),
                  inst(79, 1, 3, 0, 0),
                  inst(123, 3, 10, -8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -8),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(133, 0, 0, 0, 1),
                  inst(21, 0, 0, 3, 0),
                  inst(183, 0, 1, 0, 1),
                  inst(219, 1, 0, 0, 0),
                  inst(5, 0, 0, 8, 0),
                  inst(191, 10, 2, 0, 0),
                  inst(7, 0, 2, 0, -8),
                  inst(191, 10, 3, 0, 0),
                  inst(7, 0, 3, 0, -16),
                  inst(24, 0, 1, 0, 0),
                  inst(),
                  inst(183, 0, 4, 0, 0),
                  inst(133, 0, 0, 0, 2),
                  inst(183, 0, 0, 0, 0),
                  inst(149, 0, 0, 0, 0),
                 };
void init_benchmarks(inst** bm, vector<inst*> &bm_optis_orig, int bm_id) {
  switch (bm_id) {
    case 0:
      inst::max_prog_len = N0;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      *bm = bm0;
      bm_optis_orig.push_back(bm_opti00);
      bm_optis_orig.push_back(bm_opti01);
      bm_optis_orig.push_back(bm_opti02);
      bm_optis_orig.push_back(bm_opti03);
      break;
    case 1:
      inst::max_prog_len = N1;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      *bm = bm1;
      bm_optis_orig.push_back(bm_opti10);
      break;
    case 2:
      inst::max_prog_len = N2;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      mem_t::add_map(map_attr(8, 8, N2));
      *bm = bm2;
      bm_optis_orig.push_back(bm_opti20);
      break;
    case 3:
      inst::max_prog_len = N3;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(128);
      mem_t::add_map(map_attr(128, 64, N3)); // 8 items
      mem_t::add_map(map_attr(96, 96, N3));  // 12 items
      mem_t::add_map(map_attr(64, 128, N3)); // 16 items  => 36 items
      *bm = bm3;
      break;
    case 4:
      inst::max_prog_len = N4;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      *bm = bm4;
      bm_optis_orig.push_back(bm_opti40);
      break;
    case 5:
      inst::max_prog_len = N5;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(4);
      *bm = bm5;
      bm_optis_orig.push_back(bm_opti50);
      break;
    case 6:
      inst::max_prog_len = N6;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      *bm = bm6;
      bm_optis_orig.push_back(bm_opti60);
      break;
    case 7:
      inst::max_prog_len = N7;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      *bm = bm7;
      bm_optis_orig.push_back(bm_opti70);
      break;
    case 8:
      inst::max_prog_len = N8;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(160);
      mem_t::add_map(map_attr(64, 32, N8));
      *bm = bm8;
      break;
    case 9:
      inst::max_prog_len = N9;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(4);
      *bm = bm9;
      break;
    case 10:
      inst::max_prog_len = N10;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(16);
      *bm = bm10;
      break;
    case 11:
      inst::max_prog_len = N11;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      mem_t::add_map(map_attr(32, 32, N11));
      *bm = bm11;
      break;
    case 12:
      inst::max_prog_len = N12;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
      mem_t::set_pkt_sz(256);
      mem_t::add_map(map_attr(32, 32, N12));
      *bm = bm12;
      break;
    case 13:
      inst::max_prog_len = N13;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(64);
      mem_t::add_map(map_attr(32, 256, N13));
      *bm = bm13;
      break;
    case 14:
      inst::max_prog_len = N14;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(40);
      mem_t::add_map(map_attr(32, 256, N14));
      *bm = bm14;
      break;
    case 15:
      inst::max_prog_len = N15;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(32);
      mem_t::add_map(map_attr(32, 64, N15));
      *bm = bm15;
      break;
    case 16:
      inst::max_prog_len = N16;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(32);
      mem_t::add_map(map_attr(32, 64, N16));
      *bm = bm16;
      break;
    case 17:
      inst::max_prog_len = N17;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(40);
      mem_t::add_map(map_attr(32, 256, N17));
      *bm = bm17;
      break;
    case 18:
      inst::max_prog_len = N18;
      mem_t::set_pgm_input_type(PGM_INPUT_constant);
      mem_t::add_map(map_attr(32, 32, N18));
      *bm = bm18;
      break;
    case 19:
      inst::max_prog_len = N19;
      mem_t::set_pgm_input_type(PGM_INPUT_pkt);
      mem_t::set_pkt_sz(8);
      mem_t::add_map(map_attr(64, 64, N19));
      *bm = bm19;
      break;
    default:
      cout << "ERROR: ebpf bm_id " + to_string(bm_id) + " is out of range {0, 1, 2}" << endl;
      return;
  }
  // init sample immediate numbers and offsets
  vector<int32_t> imms;
  vector<int16_t> offs;
  for (int i = 0; i < inst::max_prog_len; i++) {
    imms.push_back((*bm)[i]._imm);
    offs.push_back((*bm)[i]._off);
  }
  inst::add_sample_imm(imms);
  inst::add_sample_off(offs);
}
