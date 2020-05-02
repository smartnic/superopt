#include <iostream>
#include <bitset>
#include "../../../src/utils.h"
#include "inst.h"

/* r0 contains the input */
inst instructions[6] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                        inst(ADDXY, 0, 2),  /* add r0, r2 */
                        inst(MOVXC, 3, 15),  /* mov r3, 15  */
                        inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                        inst(RETX, 3),      /* ret r3 */
                        inst(RETX, 0),      /* else ret r0 */
                       };

inst instructions2[4] = {inst(MOVXC, 2, 4),     /* mov r2, 4 */
                         inst(ADDXY, 0, 2),     /* add r0, r2 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(RETX, 0),         /* ret r0 */
                        };

inst instructions3[2] = {inst(NOP), /* test no-op */
                         inst(RETX, 0), /* ret r0 */
                        };

/* test unconditional jmp */
inst instructions4[3] = {inst(JMP, 1),
                         inst(ADDXY, 0, 0),
                         inst(RETX, 0),
                        };

void test1(int input_reg) {
  prog_state ps;
  inout_t input, output, expected;
  input.init();
  output.init();
  expected.init();
  cout << "Test 1: full interpretation check" << endl;

  input.reg = input_reg;
  expected.reg = max(input_reg + 4, 15);
  interpret(output, instructions, 6, ps, input);
  print_test_res(output == expected, "interpret program 1");

  input.reg = input_reg;
  expected.reg = max(input_reg + 4, 15);
  interpret(output, instructions2, 4, ps, input);
  print_test_res(output == expected, "interpret program 2");

  input.reg = input_reg;
  expected.reg = input_reg;
  interpret(output, instructions3, 2, ps, input);
  print_test_res(output == input, "interpret program 3");

  input.reg = input_reg;
  expected.reg = input_reg;
  interpret(output, instructions4, 3, ps, input);
  print_test_res(output == input, "interpret program 4");
}

void test2() {
  cout << "Test 2" << endl;
  inst x = inst(MOVXC, 2, 4);
  inst y = inst(MOVXC, 2, 4);
  inst z = inst(MOVXC, 2, 3);
  inst w = inst(RETX, 3);

  cout << "Instruction operator== check" << endl;
  print_test_res((x == y) == true, "operator== 1");
  print_test_res((inst(RETX, 3) == inst(RETC, 3)) == false, "operator== 2");
  print_test_res((inst(RETX, 3) == inst(RETX, 2)) == false, "operator== 3");
  print_test_res((inst(RETX, 3) == inst(RETX, 3)) == true, "operator== 4");

  cout << "Instruction hash value check" << endl;
  print_test_res(instHash()(x) == 22, "hash value 1");
  print_test_res(instHash()(y) == 22, "hash value 2");
  print_test_res(instHash()(z) == 10, "hash value 3");
  print_test_res(instHash()(w) == 5, "hash value 4");
}

void test3() {
  cout << "Test 3" << endl;
  string expected_bv_str = string("00010000100010000000") +
                           string("00001000000001000000") +
                           string("00010000110111100000") +
                           string("00111000000001100001") +
                           string("00011000110000000000") +
                           string("00011000000000000000");
  string bv_str = "";
  for (int i = 0; i < 6; i++) {
    inst x = instructions[i];
    vector<int> abs_bv;
    x.to_abs_bv(abs_bv);
    for (int j = 0; j < abs_bv.size(); j++) {
      bv_str += bitset<OP_NUM_BITS>(abs_bv[j]).to_string();
    }
  }
  print_test_res(bv_str == expected_bv_str, "inst_to_abs_bv");
}

int main(int argc, char *argv[]) {
  /* Add the notion of program input */
  int input = 10;
  if (argc > 1) {
    input = atoi(argv[1]);
  }

  test1(input);
  test2();
  test3();

  return 0;
}
