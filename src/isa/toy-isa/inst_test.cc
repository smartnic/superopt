#include <iostream>
#include "../../../src/utils.h"
#include "inst.h"

/* r0 contains the input */
toy_isa_inst instructions[6] = {toy_isa_inst(MOVXC, 2, 4),  /* mov r2, 4  */
                                toy_isa_inst(ADDXY, 0, 2),  /* add r0, r2 */
                                toy_isa_inst(MOVXC, 3, 15),  /* mov r3, 15  */
                                toy_isa_inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                                toy_isa_inst(RETX, 3),      /* ret r3 */
                                toy_isa_inst(RETX, 0),      /* else ret r0 */
                               };

toy_isa_inst instructions2[4] = {toy_isa_inst(MOVXC, 2, 4),     /* mov r2, 4 */
                                 toy_isa_inst(ADDXY, 0, 2),     /* add r0, r2 */
                                 toy_isa_inst(MAXC, 0, 15),     /* max r0, 15 */
                                 toy_isa_inst(RETX, 0),         /* ret r0 */
                                };

toy_isa_inst instructions3[2] = {toy_isa_inst(NOP), /* test no-op */
                                 toy_isa_inst(RETX, 0), /* ret r0 */
                                };

/* test unconditional jmp */
toy_isa_inst instructions4[3] = {toy_isa_inst(JMP, 1),
                                 toy_isa_inst(ADDXY, 0, 0),
                                 toy_isa_inst(RETX, 0),
                                };

void test1(int input) {
  toy_isa_prog_state ps;
  cout << "Test 1: full interpretation check" << endl;
  print_test_res(instructions->interpret(6, ps, input) == max(input + 4, 15),
                 "interpret program 1");
  print_test_res(instructions2->interpret(4, ps, input) == max(input + 4, 15),
                 "interpret program 2");
  print_test_res(instructions3->interpret(2, ps, input) == input,
                 "interpret program 3");
  print_test_res(instructions4->interpret(3, ps, input) == input,
                 "interpret program 4");
}

void test2() {
  cout << "Test 2" << endl;
  toy_isa_inst x = toy_isa_inst(MOVXC, 2, 4);
  toy_isa_inst y = toy_isa_inst(MOVXC, 2, 4);
  toy_isa_inst z = toy_isa_inst(MOVXC, 2, 3);
  toy_isa_inst w = toy_isa_inst(RETX, 3);

  cout << "Instruction operator== check" << endl;
  print_test_res((x == y) == true, "operator== 1");
  print_test_res((toy_isa_inst(RETX, 3) == toy_isa_inst(RETC, 3)) == false, "operator== 2");
  print_test_res((toy_isa_inst(RETX, 3) == toy_isa_inst(RETX, 2)) == false, "operator== 3");
  print_test_res((toy_isa_inst(RETX, 3) == toy_isa_inst(RETX, 3)) == true, "operator== 4");

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
    toy_isa_inst x = instructions[i];
    bv_str += x.inst_to_abs_bv().to_string();
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
