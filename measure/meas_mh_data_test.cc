#include <fstream>
#include <cstdio>
#include "meas_mh_data.h"
#include "common.h"

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
  optimals.push_back(prog(opti00));
  optimals.push_back(prog(opti01));
  string file_name = "measure/test.txt";

  store_optimals_to_file(file_name, optimals);
  read_data_from_file(file_name, "Optimals:");

  meas_mh_data d;
  d._mode = true;
  d.insert_proposal(prog(orig0), 1);
  d.insert_proposal(prog(orig1), 0);
  store_proposals_to_file(file_name, d, optimals);
  read_data_from_file(file_name, "Proposals:");

  d.insert_program(0, prog(orig0));
  d.insert_program(5, prog(orig1));
  store_programs_to_file(file_name, d, optimals);
  read_data_from_file(file_name, "Programs:");

  examples exs;
  exs.insert(inout(5, 10));
  d.insert_examples(0, exs);
  exs.insert(inout(3, 6));
  d.insert_examples(1, exs);
  d.insert_examples(2, exs);
  store_examples_to_file(file_name, d);
  read_data_from_file(file_name, "Examples:");

  remove("measure/test.txt");
}

int main() {
  test1();
  return 0;
}