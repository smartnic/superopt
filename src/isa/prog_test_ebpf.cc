#include "../../src/utils.h"
#include "prog.h"

void test1() {
  // test r10 (value of frame pointer), r1 won't be modified in canonicalize()
  inst insts1[7] = {inst(STXW, 10, -4, 1),
                    inst(LDXW, 0, 10, -4),
                    inst(EXIT),
                    inst(),
                    inst(),
                    inst(),
                    inst(),
                   };
  prog p11(insts1);
  prog p12(insts1);
  p11.canonicalize();
  print_test_res(p11 == p12, "canonicalize 1");
  return;

  // test input r1 won't be modified
  inst insts21[7] = {inst(MOV64XC, 3, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 0, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  inst insts22[7] = {inst(MOV64XC, 0, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 3, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  prog p21(insts21);
  prog p22(insts22);
  p21.canonicalize();
  print_test_res(p21 == p22, "canonicalize 2");

  // test when r1 is not used, r1 can be used in register renaming
  inst insts31[7] = {inst(MOV64XC, 3, 0x1),
                     inst(MOV64XC, 2, 0x2),
                     inst(MOV64XY, 0, 2),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  inst insts32[7] = {inst(MOV64XC, 0, 0x1),
                     inst(MOV64XC, 1, 0x2),
                     inst(MOV64XY, 2, 1),
                     inst(EXIT),
                     inst(),
                     inst(),
                     inst(),
                    };
  prog p31(insts31);
  prog p32(insts32);
  p31.canonicalize();
  print_test_res(p31 == p32, "canonicalize 3");
}

void test2() {
  cout << "test2: test top_k_progs" << endl;
  top_k_progs topk_progs(1);
  inst p1[inst::max_prog_len];
  vector<prog*> progs(10);
  for (int i = 0; i < progs.size(); i++) {
    progs[i] = new prog(p1);
    progs[i]->_error_cost = 0;
    progs[i]->_perf_cost = i + 10;
  }
  for (int i = 0; i < progs.size(); i++) {
    topk_progs.insert(progs[i]);
  }
  bool res = (topk_progs.progs.size() == 1);
  res &= (topk_progs.progs.begin()->first == 10);
  print_test_res(res, "1.1");
  // check `topk_progs.progs` won't be modified if modify `progs`
  progs[0]->_error_cost = 1;
  res = (topk_progs.progs.begin()->second->_error_cost == 0);
  print_test_res(res, "1.2");

  top_k_progs topk_progs2(3);
  for (int i = 0; i < progs.size(); i++) {
    progs[i]->_error_cost = 0;
    progs[i]->_perf_cost = 20 - i;
  }
  progs[9]->_error_cost = 1;
  progs[5]->_perf_cost = 4;
  progs[6]->_perf_cost = 5;
  for (int i = 0; i < progs.size(); i++) {
    topk_progs2.insert(progs[i]);
  }
  vector<int> expected = {12, 5, 4};
  vector<int> actual;
  for (auto it : topk_progs2.progs) {
    actual.push_back(it.first);
  }
  res = (topk_progs2.progs.size() == 3);
  for (int i = 0; i < expected.size(); i++) {
    res &= (expected[i] == actual[i]);
  }
  print_test_res(res, "2");

  top_k_progs topk_progs3(3);
  progs[0]->_error_cost = 0; progs[0]->_perf_cost = 3;
  progs[1]->_error_cost = 1; progs[1]->_perf_cost = 2;
  progs[2]->_error_cost = 0; progs[2]->_perf_cost = 4;
  for (int i = 0; i <= 2; i++) {
    topk_progs3.insert(progs[i]);
  }
  expected = {4, 3};
  actual = {};
  for (auto it : topk_progs3.progs) {
    actual.push_back(it.first);
  }
  res = (topk_progs3.progs.size() == 2);
  for (int i = 0; i < expected.size(); i++) {
    res &= (expected[i] == actual[i]);
  }
  print_test_res(res, "3");
}

int main() {
  test1();
  test2();
  return 0;
}
