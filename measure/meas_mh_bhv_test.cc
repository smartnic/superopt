#include <fstream>
#include <cstdio>
#include "meas_mh_bhv.h"
#include "benchmark_toy_isa.h"

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
  exs.insert(inout(5, 10));
  d.insert_examples(0, exs);
  d.insert_examples(1, inout(3, 6));
  d.insert_examples(2, inout(4, 8));
  store_examples_to_file(file_name, d);
  read_data_from_file(file_name, "Examples:");

  remove("measure/test.txt");
}

int main() {
  test1();
  return 0;
}
