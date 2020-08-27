#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

inst instructions1[3] = {inst(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                         inst(ADD64XY, 0, 0),           /* add64 r0, r0 */
                         inst(EXIT),                    /* exit, return r0 */
                        };

inst instructions2[3] = {inst(MOV64XC, 0, 0xffffffff),  /* mov64 r0, 0xffffffff */
                         inst(ADD32XY, 0, 0),           /* add32 r0, r0 */
                         inst(EXIT),                    /* exit, return r0 */
                        };
/* test JEQXC */
inst instructions3[9] = {inst(MOV32XC, 0, -1),         /* r0 = 0x00000000ffffffff */
                         inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                         inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                         inst(JEQXC, 0, 0, 4),         /* if r0 == 0, ret r0 = 0x100000000 */
                         inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                         inst(JEQXC, 0, 0xffffffff, 1),/* if r0 == -1, ret r0 = 0 */
                         inst(EXIT),                   /* else ret r0 = 0xffffffffffffffff */
                         inst(MOV64XC, 0, 0),
                         inst(EXIT),
                        };
/* test JEQXY */
inst instructions4[9] = {inst(MOV32XC, 0, 0xffffffff), /* r0 = 0x00000000ffffffff */
                         inst(ADD64XC, 0, 0x1),        /* r0 = 0x0000000100000000 */
                         inst(MOV64XC, 1, 0x0),        /* r1 = 0 */
                         inst(JEQXY, 0, 1, 4),         /* if r0 == r1, ret r0 = 0x100000000 */
                         inst(MOV64XY, 1, 0),          /* else r1 = r0 */
                         inst(JEQXY, 0, 1, 1),         /* if r0 == r1, ret r0 = 0x100000001 */
                         inst(EXIT),                   /* else ret r0 = 0x100000000 */
                         inst(ADD64XC, 0, 0x1),
                         inst(EXIT),
                        };

inst instructions5[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LE, 0, 16),                /* le16 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions6[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LE, 0, 32),                /* le32 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions7[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(BE, 0, 16),                /* be16 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions8[3] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(BE, 0, 32),                /* be32 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions9[6] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                         inst(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                         inst(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                         inst(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                         inst(LE, 0, 64),                /* le64 r0 */
                         inst(EXIT),                   /* exit, return r0 */
                        };

inst instructions10[6] = {inst(MOV64XC, 0, 0x01234567), /* mov64 r0, 0x01234567 */
                          inst(LSH64XC, 0, 32),         /* r0 = 0x0123456700000000 */
                          inst(MOV32XC, 1, 0x89abcdef), /* r1 = 0x0000000089abcdef */
                          inst(ADD64XY, 0, 1),          /* r0 = 0x0123456789abcdef */
                          inst(BE, 0, 64),                /* be64 r0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions11[7] = {inst(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                          inst(RSH64XC, 0, 63),         /* r0 >> 63 */
                          inst(JEQXC, 0, 1, 1),         /* if r0 != 0x1, exit */
                          inst(EXIT),                   /* exit */
                          inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                          inst(RSH32XC, 0, 1),          /* r0 >>32 1 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions12[8] = {inst(MOV64XC, 0, -1),         /* r0 = 0xffffffffffffffff */
                          inst(ARSH64XC, 0, 63),        /* r0 >> 63 */
                          inst(MOV64XC, 1, -1),         /* r1 = 0xffffffffffffffff */
                          inst(JEQXY, 0, 1, 1),         /* if r0 != r1, exit */
                          inst(EXIT),                   /* exit */
                          inst(MOV64XC, 0, -1),         /* else r0 = 0xffffffffffffffff */
                          inst(ARSH32XC, 0, 1),         /* r0 >>32 1 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions13[8] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                          inst(JGTXC, 0, 0, 1),         /* if r0 <= 0, ret r0 = 0xffffffff */
                          inst(EXIT),
                          inst(MOV64XC, 1, -1),         /* else r1 = 0xffffffffffffffff */
                          inst(JGTXY, 1, 0, 1),         /* if r1 <= r0, ret r0 = 0xffffffff */
                          inst(EXIT),
                          inst(MOV64XC, 0, 0),          /* else r0 = 0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions14[7] = {inst(MOV64XC, 0, -1),         /* r0 = -1 */
                          inst(JSGTXC, 0, 0, 4),        /* if r0 s>= 0, ret r0 = -1 */
                          inst(JSGTXC, 0, 0xffffffff, 3),/* elif r1 s> 0xffffffff, ret r0 = -1 */
                          inst(MOV64XC, 1, 0),          /* r1 = 0 */
                          inst(JSGTXY, 0, 1),           /* if r0 s> r1, ret r0 = -1 */
                          inst(MOV64XC, 0, 0),          /* else r0 = 0 */
                          inst(EXIT),                   /* exit, return r0 */
                         };

inst instructions15[4] = {inst(MOV32XC, 0, -1),         /* r0 = 0xffffffff */
                          inst(JGTXC, 0, -2, 1),        /* if r0 > 0xfffffffffffffffe, ret r0 = 0xffffffff */
                          inst(MOV64XC, 0, 0),          /* else ret r0 = 0 */
                          inst(EXIT),
                         };

inst instructions16[3] = {inst(STXW, 10, -4, 1),
                          inst(LDXW, 0, 10, -4),
                          inst(EXIT),
                         };

inst instructions17[3] = {inst(STXB, 10, -1, 1),
                          inst(LDXB, 0, 10, -1),
                          inst(EXIT),
                         };

inst instructions18[3] = {inst(STXH, 10, -2, 1),
                          inst(LDXH, 0, 10, -2),
                          inst(EXIT),
                         };

inst instructions19[6] = {inst(MOV64XY, 2, 1),
                          inst(LSH64XC, 1, 32),
                          inst(ADD64XY, 1, 2),
                          inst(STXDW, 10, -8, 1),
                          inst(LDXDW, 0, 10, -8),
                          inst(EXIT),
                         };

inst instructions20[3] = {inst(STXH, 10, -2, 1),
                          inst(LDXB, 0, 10, -2),
                          inst(EXIT),
                         };
// TODO: when safety check is added, these map related programs need to be modified
// after calling map_update function, r1-r5 are unreadable.
// r0 = *(lookup &k (update &k &v m)), where k = 0x11, v = L8(input)
inst instructions21[13] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                           inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                           inst(STXB, 10, -1, 1),
                           inst(LDMAPID, 1, 0), // r1 = map_id (0)
                           inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                           inst(ADD64XC, 2, -1),
                           inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                           inst(ADD64XC, 3, -2),
                           inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[0x11] = L8(input)
                           inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                           inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                           inst(LDXB, 0, 0, 0),
                           inst(EXIT),
                          };
// r0 = *(lookup &k (delete &k (update &k &v m))), where k = 0x11, v = L8(input)
inst instructions22[14] = {inst(STXB, 10, -2, 1), // *addr_v = r1
                           inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                           inst(STXB, 10, -1, 1),
                           inst(LDMAPID, 1, 0), // r1 = map_id (0)
                           inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                           inst(ADD64XC, 2, -1),
                           inst(MOV64XY, 3, 10), // r3(addr_v) = r10 - 2
                           inst(ADD64XC, 3, -2),
                           inst(CALL, BPF_FUNC_map_update), // map0[k] = v, i.e., map0[r1] = 0x11
                           inst(CALL, BPF_FUNC_map_delete), // delete map0[k]
                           inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                           inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                           inst(LDXB, 0, 0, 0),
                           inst(EXIT),
                          };
// r0 = *(lookup &k m), where k = 0x11, v = L8(input)
inst instructions23[9] = {inst(MOV64XC, 1, 0x11), // *addr_k = 0x11
                          inst(STXB, 10, -1, 1),
                          inst(LDMAPID, 1, 0), // r1 = map_id (0)
                          inst(MOV64XY, 2, 10), // r2(addr_k) = r10 - 1
                          inst(ADD64XC, 2, -1),
                          inst(CALL, BPF_FUNC_map_lookup), // r0 = addr_v = lookup k map0
                          inst(JEQXC, 0, 0, 1), // if r0 == 0, exit else r0 = *addr_v
                          inst(LDXB, 0, 0, 0),
                          inst(EXIT),
                         };

// check or32xc, and32xc
// r0 = (r1 & 0xffff) | 0xff0000
inst instructions24[4] = {inst(AND32XC, 1, 0xffff),  // w1 &= 0xffff
                          inst(OR32XC, 1, 0xff0000), // w1 |= 0xff0000
                          inst(MOV64XY, 0, 1), // r0 = r1
                          inst(EXIT),
                         };
// check or32xy, and32xy
// r0 = (r1 & 0xffff) | 0xff0000
inst instructions25[6] = {inst(MOV64XC, 2, 0xffff),
                          inst(AND32XY, 1, 2),  // w1 &= 0xffff
                          inst(MOV64XC, 2, 0xff0000),
                          inst(OR32XY, 1, 2),  // w1 |= 0xff0000
                          inst(MOV64XY, 0, 1),  // r0 = r1
                          inst(EXIT),
                         };
// w0 = (w1 >> 16) | ((w1 << 16) & 0xff0000)
inst instructions26[7] = {inst(MOV32XY, 2, 1), // w2 = w1
                          inst(RSH32XC, 2, 16), // w2 >>= 16
                          inst(LSH32XC, 1, 16), // w1 <<= 16
                          inst(AND32XC, 1, 0xff0000), // w1 &= 0xff0000
                          inst(OR32XY, 1, 2), // w1 |= w2
                          inst(MOV32XY, 0, 1), // w0 = w1
                          inst(EXIT),
                         };

// r0 = 0x11, *(uint8*)(r1 + 0) = 0x11
inst instructions27[4] = {inst(MOV64XY, 6, 1),
                          inst(MOV64XC, 0, 0x11),
                          inst(STXB, 6, 0, 0),
                          inst(EXIT),
                         };

// test pkt, r0 = *(uint8*)(r1 + 0)
inst instructions28[3] = {inst(MOV64XY, 6, 1),
                          inst(LDXB, 0, 6, 0),
                          inst(EXIT),
                         };

// test jmp32
inst instructions29[9] = {inst(MOV64XC, 0, 0),
                          inst(JEQ32XC, 1, 0x1, 1),
                          inst(EXIT),
                          inst(MOV64XC, 2, 1),
                          inst(LSH64XC, 2, 33),
                          inst(MOV64XC, 1, 0x0),
                          inst(JNE32XY, 1, 2, 1),
                          inst(MOV64XC, 0, 1),
                          inst(EXIT),
                         };

// test xadd64
// r0 = 0x100000002 + 0x300000004 = 0x400000006
inst instructions30[10] = {inst(MOV64XC, 1, 0x1),
                           inst(LSH64XC, 1, 32),
                           inst(ADD64XC, 1, 0x2), // r1 = 0x100000002
                           inst(STXDW, 10, -8, 1),
                           inst(MOV64XC, 2, 0x3),
                           inst(LSH64XC, 2, 32),
                           inst(ADD64XC, 2, 0x4), // r2 = 0x300000004
                           inst(XADD64, 10, -8, 2),
                           inst(LDXDW, 0, 10, -8),
                           inst(EXIT),
                          };
// test xadd32
// r0 = 0x100000002 + L32(0x300000004) = 0x100000006
inst instructions31[10] = {inst(MOV64XC, 1, 0x1),
                           inst(LSH64XC, 1, 32),
                           inst(ADD64XC, 1, 0x2), // r1 = 0x100000002
                           inst(STXDW, 10, -8, 1),
                           inst(MOV64XC, 2, 0x3),
                           inst(LSH64XC, 2, 32),
                           inst(ADD64XC, 2, 0x4), // r2 = 0x300000004
                           inst(XADD32, 10, -8, 2),
                           inst(LDXDW, 0, 10, -8),
                           inst(EXIT),
                          };
// test or64xc, or64xy
// r0 = (r1 | 0x110) | 0x011 = r1 | 0x111
inst instructions32[4] = {inst(OR64XC, 1, 0x110),
                          inst(MOV64XC, 0, 0x011),
                          inst(OR64XY, 0, 1),
                          inst(EXIT),
                         };
// test and64xc, and64xy
// r0 = (r1 & 0x011) & 0x110 = r1 & 0x10
inst instructions33[4] = {inst(AND64XC, 1, 0x011),
                          inst(MOV64XC, 0, 0x110),
                          inst(AND64XY, 0, 1),
                          inst(EXIT),
                         };

void test1() {
  mem_t::_layout.clear();
  mem_t::add_map(map_attr(8, 8, 512));
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(512); // pkt sz: 512 bytes
  prog_state ps;
  ps.init();
  inout_t input, output, expected;
  input.init();
  output.init();
  expected.init();
  input.input_simu_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  cout << "Test 1: full interpretation check" << endl;

  expected.reg = 0xfffffffffffffffe;
  interpret(output, instructions1, 3, ps, input);
  print_test_res(output == expected, "interpret program 1");

  expected.reg = 0xfffffffe;
  interpret(output, instructions2, 3, ps, input);
  print_test_res(output == expected, "interpret program 2");

  expected.reg = 0;
  interpret(output, instructions3, 9, ps, input);
  print_test_res(output == expected, "interpret program 3");

  expected.reg = 0x100000001;
  interpret(output, instructions4, 9, ps, input);
  print_test_res(output == expected, "interpret program 4");

  bool is_le = is_little_endian();
  if (is_le) expected.reg = 0x4567;
  else expected.reg = 0x6745;
  interpret(output, instructions5, 3, ps, input);
  print_test_res(output == expected, "interpret program 5");

  if (is_le) expected.reg = 0x01234567;
  else expected.reg = 0x67452301;
  interpret(output, instructions6, 3, ps, input);
  print_test_res(output == expected, "interpret program 6");

  if (is_le) expected.reg = 0x6745;
  else expected.reg = 0x4567;
  interpret(output, instructions7, 3, ps, input);
  print_test_res(output == expected, "interpret program 7");

  if (is_le) expected.reg = 0x67452301;
  else expected.reg = 0x01234567;
  interpret(output, instructions8, 3, ps, input);
  print_test_res(output == expected, "interpret program 8");

  if (is_le) expected.reg = 0x0123456789abcdef;
  else expected.reg = 0xefcdab8967452301;
  interpret(output, instructions9, 6, ps, input);
  print_test_res(output == expected, "interpret program 9");

  if (is_le) expected.reg = 0xefcdab8967452301;
  else expected.reg = 0x0123456789abcdef;
  interpret(output, instructions10, 6, ps, input);
  print_test_res(output == expected, "interpret program 10");

  expected.reg = 0x7fffffff;
  interpret(output, instructions11, 7, ps, input);
  print_test_res(output == expected, "interpret rsh64 & rsh32");

  expected.reg = 0xffffffff;
  interpret(output, instructions12, 8, ps, input);
  print_test_res(output == expected, "interpret arsh64 & arsh32");

  expected.reg = 0;
  interpret(output, instructions13, 8, ps, input);
  print_test_res(output == expected, "interpret jgt");

  expected.reg = 0;
  interpret(output, instructions14, 7, ps, input);
  print_test_res(output == expected, "interpret jsgt");

  expected.reg = 0;
  interpret(output, instructions15, 4, ps, input);
  print_test_res(output == expected, "interpret jgt");

  input.reg = 1;
  expected.reg = 1;
  interpret(output, instructions16, 3, ps, input);
  print_test_res(output == expected, "interpret ldxw & stxw 1");
  expected.reg = 0xffffffff;
  input.reg = -1;
  interpret(output, instructions16, 3, ps, input);
  print_test_res(output == expected, "interpret ldxw & stxw 2");

  input.reg = 1;
  expected.reg = 1;
  interpret(output, instructions17, 3, ps, input);
  print_test_res(output == expected, "interpret ldxb & stxb 1");
  expected.reg = 0x78;
  input.reg = 0x12345678;
  interpret(output, instructions17, 3, ps, input);
  print_test_res(output == expected, "interpret ldxb & stxb 2");

  input.reg = 1;
  expected.reg = 1;
  interpret(output, instructions18, 3, ps, input);
  print_test_res(output == expected, "interpret ldxh & stxh 1");
  expected.reg = 0x5678;
  input.reg = 0x12345678;
  interpret(output, instructions18, 3, ps, input);
  print_test_res(output == expected, "interpret ldxh & stxh 2");

  input.reg = 1;
  expected.reg = 0x100000001;
  interpret(output, instructions19, 6, ps, input);
  print_test_res(output == expected, "interpret ldxdw & stxdw 1");
  expected.reg = 0x1234567812345678;
  input.reg = 0x12345678;
  interpret(output, instructions19, 6, ps, input);
  print_test_res(output == expected, "interpret ldxdw & stxdw 2");

  input.reg = 1;
  expected.reg = 1;
  interpret(output, instructions20, 3, ps, input);
  print_test_res(output == expected, "interpret ldxb & stxh 1");
  expected.reg = 0x78;
  input.reg = 0x12345678;
  interpret(output, instructions20, 3, ps, input);
  print_test_res(output == expected, "interpret ldxb & stxh 2");

  input.reg = 0x123456;
  expected.reg = 0x56;
  expected.update_kv(0, "11", vector<uint8_t> {0x56});
  interpret(output, instructions21, 13, ps, input);
  print_test_res(output == expected, "interpret map helper function 1.1");

  expected.reg = 0x0f;
  input.reg = 0x0f;
  expected.update_kv(0, "11", vector<uint8_t> {0x0f});
  interpret(output, instructions21, 13, ps, input);
  print_test_res(output == expected, "interpret map helper function 1.2");

  input.reg = 0x56;
  expected.reg = 0;
  interpret(output, instructions22, 14, ps, input);
  print_test_res(output == expected, "interpret map helper function 2.1");
  input.reg = 0x0f;
  expected.reg = 0;
  interpret(output, instructions22, 14, ps, input);
  print_test_res(output == expected, "interpret map helper function 2.2");

  // r0 = L8(input), map0[0x11] = L8(input)
  input.reg = 0x1f;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  expected.reg = 0x1f;
  expected.update_kv(0, "11", vector<uint8_t> {0x1f});
  // r0 = *(lookup &k (update &k &v m)), where k = 0x11, v = L8(input)
  interpret(output, instructions21, 13, ps, input);
  input = output;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  output.clear();
  // r0 = *(lookup &k m), where k = 0x11
  interpret(output, instructions23, 9, ps, input);
  print_test_res(output == expected, "interpret map input 1");

  input = output;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  output.clear();
  // r0 = 0, no kv in map0
  expected.reg = 0;
  // r0 = *(lookup &k (delete &k (update &k &v m))), where k = 0x11, v = L8(input)
  interpret(output, instructions22, 14, ps, input);
  // r0 = *(lookup &k m), where k = 0x11
  input = output;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  output.clear();
  interpret(output, instructions23, 9, ps, input);
  print_test_res(output == expected, "interpret map input 2");

  input = output;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  input.reg = 0x1f;
  output.clear();
  // r0 = L8(input), map0[0x11] = L8(input)
  expected.reg = 0x1f;
  expected.update_kv(0, "11", vector<uint8_t> {0x1f});
  interpret(output, instructions21, 13, ps, input);
  input = output;
  input.input_simu_r10 = 1024; // set as a random value (> 512)
  output.clear();
  interpret(output, instructions23, 9, ps, input);
  print_test_res(output == expected, "interpret map input 3");

  // w0 = (w1 & 0xffff) | 0xff0000
  input.reg = 0xffff0f0f;
  expected.reg = 0xff0f0f;
  interpret(output, instructions24, 4, ps, input);
  print_test_res(output == expected, "interpret or32xc & and32xc");
  // w0 = (w1 & 0xffff) | 0xff0000
  input.reg = 0xff010f0f;
  expected.reg = 0xff0f0f;
  interpret(output, instructions25, 6, ps, input);
  print_test_res(output == expected, "interpret or32xy & and32xy");

  // w0 = (w1 >> 16) | ((w1 << 16) & 0xff0000)
  input.reg = 0x09abcdef12345678;
  expected.reg = 0x781234;
  interpret(output, instructions26, 7, ps, input);
  print_test_res(output == expected, "interpret or32xy & and32xy");

  // r0 = 0x11, *(uint8*)(r1 + 0) = 0x11
  input.reg = (uint64_t)ps._mem.get_pkt_start_addr();
  memset(input.pkt, 0, sizeof(uint8_t) * mem_t::_layout._pkt_sz);
  memset(expected.pkt, 0, sizeof(uint8_t) * mem_t::_layout._pkt_sz);
  expected.reg = 0x11;
  expected.pkt[0] = 0x11;
  interpret(output, instructions27, 4, ps, input);
  print_test_res(output == expected, "interpret packet");
  // r0 = *(uint8*)(r1 + 0)
  input = output;
  input.reg = (uint64_t)ps._mem.get_pkt_start_addr();
  expected.reg = 0x11;
  expected.pkt[0] = 0x11;
  interpret(output, instructions28, 3, ps, input);
  print_test_res(output == expected, "interpret packet input");

  input.reg = 0x1000000001;
  expected.reg = 0x1;
  interpret(output, instructions29, 9, ps, input);
  print_test_res(output == expected, "interpret jmp32");

  input.clear();
  expected.clear();
  input.reg = 0;
  expected.reg = 0x400000006;
  interpret(output, instructions30, 10, ps, input);
  print_test_res(output == expected, "interpret xadd64");
  input.reg = 0;
  expected.reg = 0x100000006;
  interpret(output, instructions31, 10, ps, input);
  print_test_res(output == expected, "interpret xadd32");

  input.clear();
  expected.clear();
  input.reg = 0x100000000;
  expected.reg = 0x100000111;
  interpret(output, instructions32, 10, ps, input); // r0 = r1 | 0x11
  print_test_res(output == expected, "interpret or64xc, or64xy");
  input.reg = 0x100000011;
  expected.reg = 0x10;
  interpret(output, instructions33, 10, ps, input); // r0 = r1 & 0x10
  print_test_res(output == expected, "interpret and64xc, and64xy");
}

int64_t eval_output(z3::expr smt, z3::expr output, bool flag = false) {
  z3::solver s(smt_c);
  s.add(smt);
  if (s.check() == z3::sat) {
    z3::model m = s.get_model();
    // get_numeral_int64() fails when 64th bit value is 1
    int64_t res = m.eval(output).get_numeral_uint64();
    if (flag) {
      cout << m.eval(output) << endl;
      cout << "eval_output: " << hex << res << dec << endl;
      cout << m << endl;
    }
    return res;
  }
  cout << "ERROR: no output, return -1" << endl;
  return -1;
}

bool is_valid(z3::expr smt) {
  // use bv tactic to accelerate
  z3::tactic t = z3::tactic(smt_c, "bv");
  z3::solver s = t.mk_solver();
  s.add(!smt);
  switch (s.check()) {
    case z3::unsat: return true;
    case z3::sat: return false;
    case z3::unknown:
      cout << "ERROR: smt is_valid unknown, return false" << endl;
      return false;
  }
}

void test2() {
  cout << endl << "Test 2: instruction smt check" << endl;

#define CURDST sv.get_cur_reg_var(insn._dst_reg)
#define CURSRC sv.get_cur_reg_var(insn._src_reg)
// Input FOL formulae (A) should set to `smt` first, then add instruction FOL formula (B),
// Since getting instruction FOL formula will update dst register expression.
// Also cannot add A and B together, such as smt = A && B, since some compilers
// may compute B first.
#define SMT_CHECK_XC(dst_input, dst_expected, test_name)                         \
  smt = (CURDST == to_expr((int64_t)dst_input));                                 \
  smt = smt && insn.smt_inst(sv);                                                \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

#define SMT_CHECK_XY(dst_input, src_input, dst_expected, test_name)              \
  smt = (CURDST == to_expr((int64_t)dst_input)) &&                               \
        (CURSRC == to_expr((int64_t)src_input));                                 \
  smt = smt && insn.smt_inst(sv);                                                \
  output = CURDST;                                                               \
  print_test_res(eval_output(smt, output) == (int64_t)dst_expected, test_name);  \

  mem_t::_layout.clear();
  mem_t::add_map(map_attr(8, 8, 512));
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(512); // pkt sz: 512 bytes
  inst insn = (NOP);
  int prog_id = 0, node_id = 0;
  smt_var sv(prog_id, node_id, NUM_REGS);
  sv.init();
  z3::expr f_mem_layout_constrain = sv.mem_layout_constrain();
  z3::expr smt = string_to_expr("false");
  z3::expr output = string_to_expr("false");
  z3::expr cond = Z3_true;

  insn = inst(ADD64XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffffffffffff, 0xfffffffffffffffe, "smt ADD64XC");

  insn = inst(ADD64XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0x1fffffffe, "smt ADD64XY");

  insn = inst(ADD32XC, 0, 0xffffffff);
  SMT_CHECK_XC(0xffffffff, 0xfffffffe, "smt ADD32XC");

  insn = inst(ADD32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff, 0xffffffff, 0xfffffffe, "smt ADD32XY");

  insn = inst(OR32XC, 0, 0xf0f0f0f0);
  SMT_CHECK_XC(0xfff0f00f0f, 0xf0f0ffff, "smt OR32XC");

  insn = inst(OR32XY, 0, 1);
  SMT_CHECK_XY(0xffff00000000, 0x12345678, 0x12345678, "smt OR32XY");

  insn = inst(AND32XC, 0, 0xffffffff);
  SMT_CHECK_XC(0x1, 0x1, "smt AND32XC");

  insn = inst(AND32XY, 0, 1);
  SMT_CHECK_XY(0x1, 0x11, 0x1, "smt AND32XY");

  insn = inst(MOV64XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0x0, 0xfffffffffffffffe, "smt MOV64XC");

  insn = inst(MOV64XY, 0, 1);
  SMT_CHECK_XY(0x0, 0x0123456789abcdef, 0x0123456789abcdef, "smt MOV64XY");

  insn = inst(MOV32XC, 0, 0xfffffffe);
  SMT_CHECK_XC(0xffffffff00000000, 0xfffffffe, "smt MOV32XC");

  insn = inst(MOV32XY, 0, 1);
  SMT_CHECK_XY(0xffffffff00000000, 0x0123456789abcdef, 0x89abcdef, "smt MOV32XY");

  insn = inst(LSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 0x8000000000000000, "smt LSH64XC");

  insn = inst(LSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x8000000000000000, "smt LSH64XY");

  insn = inst(LSH32XC, 0, 31);
  SMT_CHECK_XC(0xffffffffffffffff, 0x80000000, "smt LSH32XC");

  insn = inst(LSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 0xff, 0x80000000, "smt LSH32XY");

  insn = inst(RSH64XC, 0, 63);
  SMT_CHECK_XC(0xffffffffffffffff, 1, "smt RSH64XC");

  insn = inst(RSH64XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 63, 1, "smt RSH64XY");

  insn = inst(RSH32XC, 0, 31);
  SMT_CHECK_XC((int32_t)0xffffffff, 1, "smt RSH32XC");

  insn = inst(RSH32XY, 0, 1);
  SMT_CHECK_XY(0xffffffffffffffff, 31, 1, "smt RSH32XY");

  insn = inst(ARSH64XC, 0, 0x1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffffffffffff, "smt ARSH64XC");

  insn = inst(ARSH64XY, 0, 1);
  SMT_CHECK_XY(0x8000000000000000, 63, 0xffffffffffffffff, "smt ARSH64XY");

  insn = inst(ARSH32XC, 0, 1);
  SMT_CHECK_XC(0xfffffffffffffffe, 0xffffffff, "smt ARSH32XC");

  insn = inst(ARSH32XY, 0, 1);
  SMT_CHECK_XY(0xfffffffffffffffe, 31, 0xffffffff, "smt ARSH32XY");

  int64_t expected;
  bool is_le = is_little_endian();
  insn = inst(LE, 0, 16);
  if (is_le) expected = 0xcdef;
  else expected = 0xefcd;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE16");

  insn = inst(LE, 0, 32);
  if (is_le) expected = 0x89abcdef;
  else expected = 0xefcdab89;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE32");

  insn = inst(LE, 0, 64);
  if (is_le) expected = 0x0123456789abcdef;
  else expected = 0xefcdab8967452301;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt LE64");

  insn = inst(BE, 0, 16);
  if (is_le) expected = 0xefcd;
  else expected = 0xcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE16");

  insn = inst(BE, 0, 32);
  if (is_le) expected = 0xefcdab89;
  else expected = 0x89abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE32");

  insn = inst(BE, 0, 64);
  if (is_le) expected = 0xefcdab8967452301;
  else expected = 0x0123456789abcdef;
  SMT_CHECK_XC(0x0123456789abcdef, expected, "smt BE64");

#define SMT_JMP_CHECK_XC(dst_input, bool_expected, test_name)                        \
  smt = z3::implies(CURDST == to_expr((int64_t)dst_input), insn.smt_inst_jmp(sv));   \
  print_test_res(is_valid(smt) == (bool)bool_expected, test_name);

#define SMT_JMP_CHECK_XY(dst_input, src_input, bool_expected, test_name)             \
  smt = (CURDST == to_expr((int64_t)dst_input)) &&                                   \
        (CURSRC == to_expr((int64_t)src_input));                                     \
  smt = z3::implies(smt, insn.smt_inst_jmp(sv));                                     \
  print_test_res(is_valid(smt) == (bool)bool_expected, test_name);

  insn = inst(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0xffffffffffffffff, true, "smt JEQXC 1");

  insn = inst(JEQXC, 0, 0xffffffff, 1);
  SMT_JMP_CHECK_XC(0x00000000ffffffff, false, "smt JEQXC 2");

  insn = inst(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0xffffffffffffffff, true, "smt JEQXY 1");

  insn = inst(JEQXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x00000000ffffffff, 0xffffffffffffffff, false, "smt JEQXY 2");

  insn = inst(JGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0xffffffff80000001, true, "smt JGTXC 1");

  insn = inst(JGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JGTXC 2");

  insn = inst(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x7fffffffffffffff, true, "smt JGTXY 1");

  insn = inst(JNEXC, 0, -1, 1);
  SMT_JMP_CHECK_XC(1, true, "smt JNEXC 1");

  insn = inst(JNEXC, 0, -1, 1);
  SMT_JMP_CHECK_XC(-1, false, "smt JNEXC 2");

  insn = inst(JNEXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x00000000ffffffff, true, "smt JNEXY 1");

  insn = inst(JNEXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x1, false, "smt JNEXY 2");

  insn = inst(JGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x2, false, "smt JGTXY 2");

  insn = inst(JSGTXC, 0, 0x80000000, 1);
  SMT_JMP_CHECK_XC(0x0, true, "smt JSGTXC 1");

  insn = inst(JSGTXC, 0, 0x2, 1);
  SMT_JMP_CHECK_XC(0x1, false, "smt JSGTXC 2");

  insn = inst(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x0, 0x8000000000000000, true, "smt JSGTXY 1");

  insn = inst(JSGTXY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x2, false, "smt JSGTXY 2");

  insn = inst(JEQ32XC, 0, 0x1, 1);
  SMT_JMP_CHECK_XC(0xff000000001, true, "smt JEQ32XC 1");

  insn = inst(JEQ32XC, 0, 0x1, 1);
  SMT_JMP_CHECK_XC(0x2, false, "smt JEQ32XC 2");

  insn = inst(JEQ32XY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x00000000ffffffff, true, "smt JEQ32XY 1");

  insn = inst(JEQ32XY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x2, false, "smt JEQ32XY 2");

  insn = inst(JNE32XC, 0, 0x1, 1);
  SMT_JMP_CHECK_XC(0x100000001, false, "smt JNE32XC 1");

  insn = inst(JNE32XC, 0, 0x1, 1);
  SMT_JMP_CHECK_XC(0x2, true, "smt JNE32XC 2");

  insn = inst(JNE32XY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0xffffffffffffffff, 0x00000000ffffffff, false, "smt JNE32XY 1");

  insn = inst(JNE32XY, 0, 1, 1);
  SMT_JMP_CHECK_XY(0x1, 0x2, true, "smt JNE32XY 2");

#undef CURDST
#undef CURSRC
#undef SMT_CHECK_XC
#undef SMT_CHECK_XY
#undef SMT_JMP_CHECK_XC
#undef SMT_JMP_CHECK_XY

#define CURDST(insn) sv.get_cur_reg_var(insn._dst_reg)
#define CURSRC(insn) sv.get_cur_reg_var(insn._src_reg)
// Assume two instructions in the program `insns`,
// the first one is STX and the second is LD
#define SMT_CHECK_LDST(st_input, ld_output, test_name, insns)                \
  smt = f_mem_layout_constrain && f_r10;                                     \
  smt = smt && (CURSRC(insns[0]) == to_expr((int64_t)st_input));             \
  smt = smt && insns[0].smt_inst(sv);                                        \
  smt = smt && insns[1].smt_inst(sv);                                        \
  output = CURDST(insns[1]);                                                 \
  print_test_res(eval_output(smt, output) == (int64_t)ld_output, test_name);

  z3::expr f_r10 = (sv.get_cur_reg_var(10) == sv.get_stack_bottom_addr());
  sv.mem_var.add_ptr(sv.get_cur_reg_var(10), sv.mem_var.get_mem_table_id(MEM_TABLE_stack));

  inst insns1[2] = {inst(STXB, 10, -4, 1), inst(LDXB, 0, 10, -4)};
  SMT_CHECK_LDST(10, 10, "smt LDXB & STXB 1", insns1);
  SMT_CHECK_LDST(-2, 0xfe, "smt LDXB & STXB 2", insns1);

  inst insns2[2] = {inst(STXH, 10, -4, 1), inst(LDXH, 0, 10, -4)};
  SMT_CHECK_LDST(10, 10, "smt LDXH & STXH 1", insns2);
  SMT_CHECK_LDST(-2, 0xfffe, "smt LDXH & STXH 2", insns2);

  inst insns3[2] = {inst(STXW, 10, -4, 1), inst(LDXW, 0, 10, -4)};
  SMT_CHECK_LDST(10, 10, "smt LDXW & STXW 1", insns3);
  SMT_CHECK_LDST(-2, 0xfffffffe, "smt LDXW & STXW 2", insns3);

  inst insns4[2] = {inst(STXDW, 10, -8, 1), inst(LDXDW, 0, 10, -8)};
  SMT_CHECK_LDST(10, 10, "smt LDXDW & STXDW 1", insns4);
  SMT_CHECK_LDST(-2, -2, "smt LDXDW & STXDW 2", insns4);

  inst insns5[2] = {inst(STXW, 10, -4, 1), inst(LDXB, 0, 10, -4)};
  SMT_CHECK_LDST(0x12345678, 0x78, "smt LDXW & STXB 1", insns5);

  inst insns6[2] = {inst(STXW, 10, -4, 1), inst(LDXB, 0, 10, -3)};
  SMT_CHECK_LDST(0x12345678, 0x56, "smt LDXW & STXB 2", insns6);

  inst insns7[2] = {inst(STXW, 10, -4, 1), inst(LDXB, 0, 10, -2)};
  SMT_CHECK_LDST(0x12345678, 0x34, "smt LDXW & STXB 3", insns7);

  inst insns8[2] = {inst(STXW, 10, -4, 1), inst(LDXB, 0, 10, -1)};
  SMT_CHECK_LDST(0x12345678, 0x12, "smt LDXW & STXB 4", insns8);
#undef SMT_CHECK_LDST

// Assume two instructions in the program `insns`,
// the first one is ST and the second is LD
#define SMT_CHECK_LDST(ld_output, test_name, insns)                          \
  smt = f_mem_layout_constrain && f_r10 && insns[0].smt_inst(sv);            \
  smt = smt && insns[1].smt_inst(sv);                                        \
  output = CURDST(insns[1]);                                                 \
  print_test_res(eval_output(smt, output) == (int64_t)ld_output, test_name);

  inst insns11[2] = {inst(STB, 10, -1, 0x12), inst(LDXB, 0, 10, -1)};
  SMT_CHECK_LDST(0x12, "smt LDXB & STB", insns11);

  inst insns12[2] = {inst(STH, 10, -2, 0x1234), inst(LDXH, 0, 10, -2)};
  SMT_CHECK_LDST(0x1234, "smt LDXH & STH", insns12);

  inst insns13[2] = {inst(STW, 10, -4, 0x12345678), inst(LDXW, 0, 10, -4)};
  SMT_CHECK_LDST(0x12345678, "smt LDXW & STW", insns13);

  inst insns14[2] = {inst(STDW, 10, -8, -1), inst(LDXDW, 0, 10, -8)};
  SMT_CHECK_LDST(-1, "smt LDXDW & STDW", insns14);

#define SMT_CHECK_XADD(v1, v2, v3, v4, size, ld_output, test_name)           \
  smt = f_mem_layout_constrain && f_r10;                                     \
  smt = smt && inst(STW, 10, -4, v1).smt_inst(sv);                           \
  smt = smt && inst(STW, 10, -8, v2).smt_inst(sv);                           \
  smt = smt && inst(MOV64XC, 1, v3).smt_inst(sv);                            \
  smt = smt && inst(LSH64XC, 1, 32).smt_inst(sv);                            \
  smt = smt && inst(ADD64XC, 1, v4).smt_inst(sv);                            \
  smt = smt && inst(XADD##size, 10, -8, 1).smt_inst(sv);                     \
  smt = smt && inst(LDXDW, 0, 10, -8).smt_inst(sv);                          \
  output = CURDST(inst(LDXDW, 0, 10, -8));                                   \
  print_test_res(eval_output(smt, output) == (int64_t)ld_output, test_name);

  SMT_CHECK_XADD(0x0, 0x1, 0x0, 0x2, 64, 0x3, "smt XADD64 1");
  SMT_CHECK_XADD(0x1, 0x2, 0x3, 0x4, 64, 0x400000006, "smt XADD64 2");
  SMT_CHECK_XADD(0x0, 0x1, 0x0, 0x2, 32, 0x3, "smt XADD32 1");
  SMT_CHECK_XADD(0x1, 0x2, 0x3, 0x4, 32, 0x100000006, "smt XADD32 2");


  // test the address track of "add64xy"
  smt = f_mem_layout_constrain && f_r10;
  smt = smt && inst(MOV64XC, 1, -1).smt_inst(sv);
  smt = smt && inst(MOV64XY, 2, 10).smt_inst(sv);
  smt = smt && inst(STB, 2, 0, 0xff).smt_inst(sv);
  smt = smt && inst(LDXB, 0, 2, 0).smt_inst(sv);
  output = CURDST(inst(LDXB, 0, 2, 0));
  print_test_res(eval_output(smt, output) == (int64_t)0xff, "address track of add64xy");
#undef CURDST
#undef CURSRC
#undef SMT_CHECK_LDST
}

// test3 is to check whether the ebpf inst is the same as linux bpf
void test3() {
  cout << endl << "Test 3: ebpf bytecode check" << endl;
  // this has been checked by bpf-step
  inst prog[] = {inst(MOV64XY, 6, 1),
                 inst(MOV64XY, 2, 10),
                 inst(ADD64XC, 2, -4),
                 inst(MOV64XC, 1, 0x10),
                 inst(JGTXC, 1, -1, 1),
                 inst(MOV64XC, 1, 1),
                 inst(JEQXC, 0, 0, 1),
                 inst(MOV64XC, 0, 1),
                 inst(EXIT),
                };
  string expected = "{191, 6, 1, 0, 0},"\
                    "{191, 2, 10, 0, 0},"\
                    "{7, 2, 0, 0, -4},"\
                    "{183, 1, 0, 0, 16},"\
                    "{37, 1, 0, 1, -1},"\
                    "{183, 1, 0, 0, 1},"\
                    "{21, 0, 0, 1, 0},"\
                    "{183, 0, 0, 0, 1},"\
                    "{149, 0, 0, 0, 0},";
  string prog_bytecode = "";
  for (int i = 0; i < 9; i++) {
    prog_bytecode += prog[i].get_bytecode_str() + ",";
  }
  print_test_res(prog_bytecode == expected, "ebpf bytecode 1");

  // test all opcodes: has included all opcodes except NOP,
  // since there is no NOP in linux bpf
  inst prog2[] = {inst(ADD64XC, 3, 1),
                  inst(ADD64XY, 3, 1),
                  inst(OR64XC, 3, 1),
                  inst(OR64XY, 3, 1),
                  inst(AND64XC, 3, 1),
                  inst(AND64XY, 3, 1),
                  inst(LSH64XC, 3, 1),
                  inst(LSH64XY, 3, 1),
                  inst(RSH64XC, 3, 1),
                  inst(RSH64XY, 3, 1),
                  inst(MOV64XC, 3, 1),
                  inst(MOV64XY, 3, 1),
                  inst(ARSH64XC, 3, 1),
                  inst(ARSH64XY, 3, 1),
                  inst(ADD32XC, 3, 1),
                  inst(ADD32XY, 3, 1),
                  inst(OR32XC, 3, 1),
                  inst(OR32XY, 3, 1),
                  inst(AND32XC, 3, 1),
                  inst(AND32XY, 3, 1),
                  inst(LSH32XC, 3, 1),
                  inst(LSH32XY, 3, 1),
                  inst(RSH32XC, 3, 1),
                  inst(RSH32XY, 3, 1),
                  inst(MOV32XC, 3, 1),
                  inst(MOV32XY, 3, 1),
                  inst(ARSH32XC, 3, 1),
                  inst(ARSH32XY, 3, 1),
                  inst(LE, 3, 16),
                  inst(BE, 3, 16),
                  inst(LDMAPID, 1, 2),
                  inst(LDXB, 1, 10, -4),
                  inst(STXB, 10, -4, 1),
                  inst(LDXH, 1, 10, -4),
                  inst(STXH, 10, -4, 1),
                  inst(LDXW, 1, 10, -4),
                  inst(STXW, 10, -4, 1),
                  inst(LDXDW, 1, 10, -8),
                  inst(STXDW, 10, -8, 1),
                  inst(STB, 10, -4, 1),
                  inst(STH, 10, -4, 1),
                  inst(STW, 10, -4, 1),
                  inst(STDW, 10, -8, 1),
                  inst(XADD64, 10, -8, 1),
                  inst(XADD32, 10, -8, 1),
                  inst(LDABSH, 1),
                  inst(LDINDH, 1),
                  inst(JA, 1),
                  inst(JEQXC, 3, 1, 2),
                  inst(JEQXY, 3, 1, 2),
                  inst(JGTXC, 3, 1, 2),
                  inst(JGTXY, 3, 1, 2),
                  inst(JNEXC, 3, 1, 2),
                  inst(JNEXY, 3, 1, 2),
                  inst(JSGTXC, 3, 1, 2),
                  inst(JSGTXY, 3, 1, 2),
                  inst(JEQ32XC, 3, 1, 2),
                  inst(JEQ32XY, 3, 1, 2),
                  inst(JNE32XC, 3, 1, 2),
                  inst(JNE32XY, 3, 1, 2),
                  inst(CALL, 1),
                  inst(EXIT),
                 };
  expected = "{7, 3, 0, 0, 1},"\
             "{15, 3, 1, 0, 0},"\
             "{71, 3, 0, 0, 1},"\
             "{79, 3, 1, 0, 0},"\
             "{87, 3, 0, 0, 1},"\
             "{95, 3, 1, 0, 0},"\
             "{103, 3, 0, 0, 1},"\
             "{111, 3, 1, 0, 0},"\
             "{119, 3, 0, 0, 1},"\
             "{127, 3, 1, 0, 0},"\
             "{183, 3, 0, 0, 1},"\
             "{191, 3, 1, 0, 0},"\
             "{199, 3, 0, 0, 1},"\
             "{207, 3, 1, 0, 0},"\
             "{4, 3, 0, 0, 1},"\
             "{12, 3, 1, 0, 0},"\
             "{68, 3, 0, 0, 1},"\
             "{76, 3, 1, 0, 0},"\
             "{84, 3, 0, 0, 1},"\
             "{92, 3, 1, 0, 0},"\
             "{100, 3, 0, 0, 1},"\
             "{108, 3, 1, 0, 0},"\
             "{116, 3, 0, 0, 1},"\
             "{124, 3, 1, 0, 0},"\
             "{180, 3, 0, 0, 1},"\
             "{188, 3, 1, 0, 0},"\
             "{196, 3, 0, 0, 1},"\
             "{204, 3, 1, 0, 0},"\
             "{212, 3, 0, 0, 16},"\
             "{220, 3, 0, 0, 16},"\
             "{24, 1, 0, 0, 2},{0, 0, 0, 0, 0},"\
             "{113, 1, 10, -4, 0},"\
             "{115, 10, 1, -4, 0},"\
             "{105, 1, 10, -4, 0},"\
             "{107, 10, 1, -4, 0},"\
             "{97, 1, 10, -4, 0},"\
             "{99, 10, 1, -4, 0},"\
             "{121, 1, 10, -8, 0},"\
             "{123, 10, 1, -8, 0},"\
             "{114, 10, 0, -4, 1},"\
             "{106, 10, 0, -4, 1},"\
             "{98, 10, 0, -4, 1},"\
             "{122, 10, 0, -8, 1},"\
             "{219, 10, 1, -8, 0},"\
             "{195, 10, 1, -8, 0},"\
             "{40, 0, 0, 0, 1},"\
             "{72, 0, 1, 0, 0},"\
             "{5, 0, 0, 1, 0},"\
             "{21, 3, 0, 2, 1},"\
             "{29, 3, 1, 2, 0},"\
             "{37, 3, 0, 2, 1},"\
             "{45, 3, 1, 2, 0},"\
             "{85, 3, 0, 2, 1},"\
             "{93, 3, 1, 2, 0},"\
             "{101, 3, 0, 2, 1},"\
             "{109, 3, 1, 2, 0},"\
             "{22, 3, 0, 2, 1},"\
             "{30, 3, 1, 2, 0},"\
             "{86, 3, 0, 2, 1},"\
             "{94, 3, 1, 2, 0},"\
             "{133, 0, 0, 0, 1},"\
             "{149, 0, 0, 0, 0},";
  prog_bytecode = "";
  for (int i = 0; i < NUM_INSTR - 1; i++) {
    prog_bytecode += prog2[i].get_bytecode_str() + ",";
  }
  print_test_res(prog_bytecode == expected, "ebpf bytecode 2");
}

void test4() {
  cout << endl << "Test 3: ebpf opcode and idx conversion check" << endl;
  bool check_res = true;
  for (int idx = 0; idx < NUM_INSTR; idx++) {
    if (opcode_2_idx(idx_2_opcode[idx]) != idx) {
      print_test_res(false, "idx:" + to_string(idx) + " opcode conversion");
      check_res = false;
    }
  }
  if (check_res) {
    print_test_res(true, "opcode and idx conversion");
  }
}

void test5() {
  cout << endl << "Test 5: memory access safety check" << endl;
  prog_state ps;
  mem_t::_layout.clear();
  ps.init();
  inout_t input, output;
  input.init();
  output.init();
  string msg = "";
#define SMT_CHECK_MEM_SAFE(insns, len, check_expr, test_name) \
  msg = "";\
  try { \
    interpret(output, insns, len, ps, input);\
  } catch (const string err_msg) {\
    msg = err_msg;\
  } \
  print_test_res(check_expr, test_name);

  inst insns1[2] = {inst(STXB, 10, 0, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns1, 2, !msg.empty(), "1");

  inst insns2[2] = {inst(STXB, 10, -1, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns2, 2, msg.empty(), "2");

  inst insns3[2] = {inst(STXB, 10, -512, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns3, 2, msg.empty(), "3");

  inst insns4[2] = {inst(STXB, 10, -513, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns4, 2, !msg.empty(), "4");

  inst insns5[2] = {inst(STXH, 10, -1, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns5, 2, !msg.empty(), "5");

  inst insns6[2] = {inst(STXH, 10, -2, 1), inst(EXIT)};
  SMT_CHECK_MEM_SAFE(insns6, 2, msg.empty(), "6");
#undef SMT_CHECK_MEM_SAFE
}

void test6() {
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_pkt_ptrs);
  mem_t::set_pkt_sz(4); // 4 bytes
  prog_state ps;
  ps.init();
  inout_t input, output, expected;
  input.init();
  output.init();
  expected.init();
  input.input_simu_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  // check when pkt_sz is 2 bytes, which is smaller than the layout pkt sz 4
  input.input_simu_pkt_ptrs[0] = (uint64_t)ps._mem.get_pkt_start_addr();
  input.input_simu_pkt_ptrs[1] = input.input_simu_pkt_ptrs[0] + 1;
  input.reg = (uint64_t)ps._mem.get_pkt_ptrs_start_addr();
  cout << "Test 6: full interpretation check of PGM_INPUT_pkt_ptrs" << endl;
  inst p1[3] = {inst(LDXW, 1, 1, 0), // r1 = pkt_start_addr
                inst(LDXB, 3, 1, 0), // r3 = pkt[0]
                inst(MOV64XY, 0, 3),
               };
  input.pkt[0] = 0x11;
  input.pkt[1] = 0x22;
  interpret(output, p1, 3, ps, input);
  expected.pkt[0] = 0x11;
  expected.pkt[1] = 0x22;
  expected.reg = 0x11;
  print_test_res(output == expected, "interpret program 1");

  inst p2[6] = {inst(LDXW, 2, 1, 4), // r2 = pkt_end_addr
                inst(LDXW, 1, 1, 0), // r1 = pkt_start_addr
                inst(LDXB, 3, 1, 0), // r3 = pkt[0]
                inst(LDXB, 4, 2, 0), // r4 = pkt[1]
                inst(ADD64XY, 3, 4),
                inst(MOV64XY, 0, 3),
               };
  input.pkt[0] = 0x11;
  input.pkt[1] = 0x22;
  interpret(output, p2, 6, ps, input);
  expected.pkt[0] = 0x11;
  expected.pkt[1] = 0x22;
  expected.reg = 0x33;
  print_test_res(output == expected, "interpret program 2");

  inst p3[11] = {inst(LDXW, 2, 1, 4), // r2 = pkt_end_addr
                 inst(LDXW, 1, 1, 0), // r1 = pkt_start_addr
                 inst(MOV64XY, 3, 1),
                 inst(ADD64XC, 3, 1),  // r3 = pkt_start_addr + 1
                 inst(JEQXY, 2, 3, 2),
                 inst(MOV64XC, 0, 0),
                 inst(EXIT),
                 inst(LDXB, 3, 1, 0), // r3 = pkt[0]
                 inst(LDXB, 4, 2, 0), // r4 = pkt[1]
                 inst(ADD64XY, 3, 4),
                 inst(MOV64XY, 0, 3),
                };
  input.pkt[0] = 0x11;
  input.pkt[1] = 0x22;
  interpret(output, p3, 11, ps, input);
  expected.pkt[0] = 0x11;
  expected.pkt[1] = 0x22;
  expected.reg = 0x33;
  print_test_res(output == expected, "interpret program 3");
}

void test_read_before_write(inst* pgm, int len, bool is_safe_expected, string test_name) {
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_constant);
  prog_state ps;
  ps.init();
  inout_t input, output;
  input.init();
  output.init();
  input.input_simu_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  bool is_safe_actual = true;
  string err_msg_actual = "";
  try {
    interpret(output, pgm, len, ps, input);
  } catch (string err_msg) {
    is_safe_actual = false;
    err_msg_actual = err_msg;
  }
  bool res = false;
  if (is_safe_expected) res = (is_safe_actual);
  else res = (! is_safe_actual) && (err_msg_actual.find("not readable") != string::npos);
  print_test_res(res, test_name);
}

void test7() {
  cout << "Test 13: test safety check" << endl;
  cout << "1. register read before write" << endl;
  inst p1[3] = {inst(MOV64XY, 2, 1), // r1, r10 is readable without assigning value
                inst(MOV64XC, 3, 10),
                inst(EXIT),
               };
  test_read_before_write(p1, 3, true, "1");

  inst p2[2] = {inst(MOV64XY, 2, 0), inst(EXIT)};
  test_read_before_write(p2, 2, false, "2");

  inst p3[2] = {inst(JEQXC, 2, 1, 0), inst(EXIT)};
  test_read_before_write(p3, 2, false, "3");

  inst p4[2] = {inst(JEQXY, 2, 1, 0), inst(EXIT)};
  test_read_before_write(p4, 2, false, "4");

  inst p5[2] = {inst(LDXB, 0, 0, 0), inst(EXIT)};
  test_read_before_write(p5, 2, false, "5");

  inst p6[2] = {inst(STXB, 0, 0, 0), inst(EXIT)};
  test_read_before_write(p6, 2, false, "6");

  inst p7[2] = {inst(XADD64, 0, 0, 0), inst(EXIT)};
  test_read_before_write(p7, 2, false, "7");

  inst p8[2] = {inst(ADD64XC, 0, 0), inst(EXIT)};
  test_read_before_write(p8, 2, false, "8");

  inst p9[2] = {inst(CALL, 1), inst(EXIT)};
  test_read_before_write(p9, 2, false, "9");

  inst p10[2] = {inst(LE, 0, 32), inst(EXIT)};
  test_read_before_write(p10, 2, false, "10");

  cout << "2. stack read before write" << endl;
  inst p2_1[3] = {inst(STB, 10, -1, 2),
                  inst(LDXB, 0, 10, -1),
                  inst(EXIT),
                 };
  test_read_before_write(p2_1, 3, true, "2.1");

  inst p2_2[2] = {inst(LDXB, 0, 10, -1), inst(EXIT)};
  test_read_before_write(p2_2, 2, false, "2.2");

  inst p2_3[3] = {inst(STB, 10, -2, 2),
                  inst(LDXH, 0, 10, -2),
                  inst(EXIT),
                 };
  test_read_before_write(p2_3, 3, false, "2.3");

  inst p2_4[2] = {inst(LDXB, 0, 10, -512), inst(EXIT)};
  test_read_before_write(p2_4, 2, false, "2.4");

  inst p2_5[2] = {inst(LDXB, 0, 10, -64), inst(EXIT)};
  test_read_before_write(p2_5, 2, false, "2.5");
}

void test8() {
  mem_t::_layout.clear();
  mem_t::set_pgm_input_type(PGM_INPUT_skb);
  mem_t::set_pkt_sz(16); // pkt sz: 512 bytes
  prog_state ps;
  ps.init();
  inout_t input, output, expected;
  input.init();
  output.init();
  expected.init();
  input.input_simu_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  cout << "Test 1: skb full interpretation check" << endl;
  // test skb load, ldabsh
  inst p1[2] = {inst(LDABSH, 0),
                inst(EXIT),
               };
  input.set_pkt_random_val();
  expected = input;
  expected.reg = (input.pkt[1] << 8) | input.pkt[0];
  interpret(output, p1, 2, ps, input);
  print_test_res(output == expected, "LDABSH 1");

  // test pkt and skb use the same memory
  inst p2[8] = {inst(LDABSH, 0),  // r0 = *(u16*)skb[0]
                inst(MOV64XY, 2, 0),
                inst(LDXH, 0, 1, 0), // r0 = *(u16*)pkt[0]
                inst(JEQXY, 0, 2, 2), // if r0 == r2, r0 = 1, else r0 = 0
                inst(MOV64XC, 0, 0),
                inst(EXIT),
                inst(MOV64XC, 0, 1),
                inst(EXIT),
               };
  input.set_pkt_random_val();
  expected = input;
  expected.reg = 1;
  interpret(output, p2, 8, ps, input);
  print_test_res(output == expected, "LDABSH 2");

  inst p3[3] = {inst(MOV64XC, 2, 0),
                inst(LDINDH, 2),
                inst(EXIT),
               };
  input.set_pkt_random_val();
  expected = input;
  expected.reg = (input.pkt[1] << 8) | input.pkt[0];
  interpret(output, p3, 3, ps, input);
  print_test_res(output == expected, "LDINDH 1");

  inst p4[9] = {inst(MOV64XC, 2, 0),
                inst(LDINDH, 2),
                inst(MOV64XY, 2, 0),
                inst(LDABSH, 0),
                inst(JEQXY, 0, 2, 2), // if r0 == r2, r0 = 1, else r0 = 0
                inst(MOV64XC, 0, 0),
                inst(EXIT),
                inst(MOV64XC, 0, 1),
                inst(EXIT),
               };
  input.set_pkt_random_val();
  expected = input;
  expected.reg = 1;
  interpret(output, p4, 9, ps, input);
  print_test_res(output == expected, "LDINDH 2");
}

void test9() {
  cout << "Test 9: full interpretation check of tail call" << endl;
  mem_t::_layout.clear();
  mem_t::add_map(map_attr(8, 8, 3, MAP_TYPE_prog_array)); // add an prog_array map
  mem_t::set_pgm_input_type(PGM_INPUT_pkt);
  mem_t::set_pkt_sz(16); // pkt sz: 512 bytes
  prog_state ps;
  ps.init();
  inout_t input, output1, output2, expected;
  input.init();
  output1.init();
  output2.init();
  expected.init();
  input.input_simu_r10 = (uint64_t)ps._mem.get_stack_bottom_addr();
  inst p1[6] = {inst(MOV64XY, 1, 1),
                inst(LDMAPID, 2, 0),
                inst(MOV64XC, 3, 1), // Jump to map[1] program
                inst(CALL, BPF_FUNC_tail_call), // in the current implementation, this is an exit insn
                inst(MOV64XC, 0, 0xff),
                inst(EXIT),
               };
  interpret(output1, p1, 6, ps, input);
  expected.tail_call_para = 1;
  expected.pgm_exit_type = PGM_EXIT_TYPE_tail_call;
  print_test_res(output1 == expected, "tail call 1.1");
  inst p1_1[5] = {inst(MOV64XY, 1, 1),
                  inst(LDMAPID, 2, 0),
                  inst(MOV64XC, 3, 1), // Jump to map[1] program
                  inst(CALL, BPF_FUNC_tail_call), // in the current implementation, this is an exit insn
                  inst(EXIT),
                 };
  interpret(output2, p1_1, 5, ps, input);
  print_test_res(output2 == expected, "tail call 1.2");
  vector<int64_t> val_list1, val_list2;
  get_cmp_lists(val_list1, val_list2, output1, output2);
  bool is_equal = true;
  for (int i = 0; i < val_list1.size(); i++) {
    is_equal &= (val_list1[i] == val_list2[i]);
  }
  print_test_res(is_equal, "tail call 1.3");
}

int main(int argc, char *argv[]) {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();

  return 0;
}
