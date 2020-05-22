#include <fstream>
#include <cstdio>
#include "meas_mh_bhv.h"
#include "benchmark_toy_isa.h"
#include "../src/utils.h"

using namespace std;

void read_data_from_file(string file_name, string str_to_print) {
  ifstream fin(file_name, ios::in);
  char line[256];
  cout << str_to_print << endl;
  while (! fin.eof()) {
    fin.getline (line, 256);
    cout << line << endl;
  }
  fin.clear();
  fin.close();
}

void test1() {
  vector<prog> optimals;
  optimals.push_back(prog(bm_opti00));
  optimals.push_back(prog(bm_opti01));
  string file_name = "measure/test.txt";
  meas_mh_data d;
  d._mode = true;

  store_optimals_to_file(file_name, optimals, d._mode);
  read_data_from_file(file_name, "Optimals:");

  d.insert_proposal(prog(bm0), 1);
  d.insert_proposal(prog(bm1), 0);
  store_proposals_to_file(file_name, d, optimals);
  read_data_from_file(file_name, "Proposals:");

  d.insert_program(0, prog(bm0));
  d.insert_program(5, prog(bm1));
  store_programs_to_file(file_name, d, optimals);
  read_data_from_file(file_name, "Programs:");

  examples exs;
  inout_t input, output;
  input.init();
  output.init();
  input.reg = 5;
  output.reg = 10;
  exs.insert(inout(input, output));
  d.insert_examples(0, exs);
  input.reg = 3;
  output.reg = 6;
  d.insert_examples(1, inout(input, output));
  input.reg = 4;
  output.reg = 8;
  d.insert_examples(2, inout(input, output));
  store_examples_to_file(file_name, d);
  read_data_from_file(file_name, "Examples:");

  remove("measure/test.txt");
}

void test2() {
  cout << "test combination" << endl;
  print_test_res(combination(10, 2) == 45, "1");
  print_test_res(combination(10, 5) == 252, "2");
  print_test_res(combination(20, 19) == 20, "3");
  print_test_res(combination(50, 20) == 47129212243960, "4");
  print_test_res(combination(50, 25) == 126410606437752, "5");
  print_test_res(combination(56, 28) == 7648690600760440, "6");
}

int main() {
  test1();
  test2();
  return 0;
}
