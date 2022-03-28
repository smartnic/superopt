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

bool check_top_k_progs_res(vector<double> perf_costs, top_k_progs& topk_progs) {
  if (perf_costs.size() != topk_progs.progs.size()) return false;
  topk_progs.sort();
  for (int i = 0; i < topk_progs.progs.size(); i++) {
    if (topk_progs.progs[i]->_error_cost != 0) return false;
    if (topk_progs.progs[i]->_perf_cost != perf_costs[i]) return false;
  }
  return true;
}

void test2() {
  cout << "test2: test top_k_progs" << endl;
  top_k_progs topk_progs1(1), topk_progs2(3);
  inst p1[inst::max_prog_len];
  prog* pgm = new prog(p1);
  pgm->_error_cost = 0;
  pgm->_perf_cost = 1.25;

  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  vector<double> perf_costs_1, perf_costs_2;
  perf_costs_1 = {1.25};
  perf_costs_2 = {1.25};
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "1.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "1.2");

  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "2.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "2.2");

  pgm->inst_list[0] = inst(MOV64XC, 0, 0);
  pgm->_error_cost = 0;
  pgm->_perf_cost = 1.25;
  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  perf_costs_1 = {1.25};
  perf_costs_2 = {1.25, 1.25};
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "3.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "3.2");

  pgm->inst_list[1] = inst(MOV64XC, 0, 1);
  pgm->_error_cost = 1;
  pgm->_perf_cost = 1.25;
  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  perf_costs_1 = {1.25};
  perf_costs_2 = {1.25, 1.25};
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "4.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "4.2");

  pgm->inst_list[2] = inst(MOV64XC, 1, 0);
  pgm->_error_cost = 0;
  pgm->_perf_cost = 1.37;
  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  perf_costs_1 = {1.25};
  perf_costs_2 = {1.25, 1.25, 1.37};
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "5.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "5.2");

  pgm->inst_list[3] = inst(MOV64XC, 1, 0);
  pgm->_error_cost = 0;
  pgm->_perf_cost = 0.81;
  topk_progs1.insert(pgm);
  topk_progs2.insert(pgm);
  perf_costs_1 = {0.81};
  perf_costs_2 = {0.81, 1.25, 1.25};
  print_test_res(check_top_k_progs_res(perf_costs_1, topk_progs1), "6.1");
  print_test_res(check_top_k_progs_res(perf_costs_2, topk_progs2), "6.2");
}

void test3(){

  //Testing set_safety_cost
  prog * myProg = new prog();
  (* myProg).set_safety_cost(23);
  
  //cout << "\n--------------------------------------\n";
  cout << "Testing set_safety_cost\n";

  if(myProg->_safety_cost == 23){
    cout << "Safety cost successfully verified to be 23\n";
  }else{
    cout << "NOT SUCCESS\n";
  }
  

}

int main() {
  test1();
  test2();
  test3();
  return 0;
}
