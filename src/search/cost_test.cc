#include <iostream>
#include "../../src/utils.h"
#include "cost.h"

using namespace std;

toy_isa_inst instructions[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),  /* mov r2, 4  */
                                toy_isa_inst(toy_isa::ADDXY, 0, 2),  /* add r0, r2 */
                                toy_isa_inst(toy_isa::MOVXC, 3, 15),  /* mov r3, 15  */
                                toy_isa_inst(toy_isa::JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                                toy_isa_inst(toy_isa::RETX, 3),      /* ret r3 */
                                toy_isa_inst(toy_isa::RETX, 0),      /* else ret r0 */
                                toy_isa_inst(), /* nop */
                               };

toy_isa_inst instructions2[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),     /* mov r2, 4 */
                                 toy_isa_inst(toy_isa::ADDXY, 0, 2),     /* add r0, r2 */
                                 toy_isa_inst(toy_isa::MAXC, 0, 15),     /* max r0, 15 */
                                 toy_isa_inst(toy_isa::RETX, 0),         /* ret r0 */
                                 toy_isa_inst(), /* nop */
                                 toy_isa_inst(), /* nop */
                                 toy_isa_inst(), /* nop */
                                };

toy_isa_inst instructions3[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),  /* mov r2, 4  */
                                 toy_isa_inst(toy_isa::ADDXY, 0, 2),  /* add r0, r2 */
                                 toy_isa_inst(toy_isa::MOVXC, 3, 15),  /* mov r3, 15  */
                                 toy_isa_inst(toy_isa::JMPEQ, 0, 3, 1),  /* if r0 != r3: */
                                 toy_isa_inst(toy_isa::RETX, 3),      /* ret r3 */
                                 toy_isa_inst(toy_isa::RETX, 0),      /* else ret r0 */
                                 toy_isa_inst(), /* nop */
                                };

toy_isa_inst instructions4[7] = {toy_isa_inst(toy_isa::MOVXC, 2, 4),  /* mov r2, 4  */
                                 toy_isa_inst(toy_isa::ADDXY, 0, 2),  /* add r0, r2 */
                                 toy_isa_inst(toy_isa::MOVXC, 3, 16),  /* mov r3, 16  */
                                 toy_isa_inst(toy_isa::JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                                 toy_isa_inst(toy_isa::RETC, 15),      /* ret 15 */
                                 toy_isa_inst(toy_isa::RETX, 0),      /* else ret r0 */
                                 toy_isa_inst(), /* nop */
                                };

vector<inout> ex_set(2);

void test1() {
#define NUM_INTS 6
  cout << "test 1: pop_count_asm starts...\n";
  unsigned int ints_list[NUM_INTS] = {0, 1, 5, 7, 63, 114};
  int truth[NUM_INTS] = {0, 1, 2, 3, 6, 4};
  for (int i = 0; i < NUM_INTS; i++) {
    print_test_res(pop_count_asm(ints_list[i]) == truth[i],
                   to_string(i + 1));
  }
}

void test2() {
  cout << "test 2: error_cost check starts...\n";
  prog orig(instructions);
  cost c;
  c.set_orig(&orig, 7);
  c._examples.clear();
  for (size_t i = 0; i < ex_set.size(); i++) {
    c._examples.insert(ex_set[i]);
  }
  int err_cost = c.error_cost(&orig, 7);
  cout << "err_cost " << err_cost << endl;
  print_test_res(err_cost == 0, "1");
  prog synth1(instructions2);
  err_cost = c.error_cost(&synth1, 7);
  print_test_res(err_cost == 0, "2");
  prog synth2(instructions3);
  err_cost = c.error_cost(&synth2, 7);
  print_test_res(err_cost == 6, "3");
  prog synth3(instructions4);
  err_cost = c.error_cost(&synth3, 7);
  print_test_res(err_cost == 2, "4");
}

int main() {
  ex_set[0].set_in_out(10, 15);
  ex_set[1].set_in_out(16, 20);
  test1();
  test2();
  return 0;
}
