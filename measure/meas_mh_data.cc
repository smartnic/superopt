#include <fstream>
#include "meas_mh_data.h"

using namespace std;

/* class meas_mh_data start */
meas_mh_data::meas_mh_data() {}

meas_mh_data::~meas_mh_data() {}

void meas_mh_data::insert_proposal(const prog &proposal, bool accepted) {
  if (_mode) {
    _proposals.push_back(make_pair(proposal, accepted));
  }
}

void meas_mh_data::insert_program(unsigned int iter_num, const prog &program) {
  if (_mode) {
    _programs.push_back(make_pair(iter_num, program));
  }
}

void meas_mh_data::insert_examples(unsigned int iter_num, const examples &exs) {
  if (_mode) {
    _examples.push_back(make_pair(iter_num, exs));
  }
}

void meas_mh_data::insert_examples(unsigned int iter_num, const inout &exs) {
  if (_mode) {
    examples exs_new;
    exs_new.insert(exs);
    this->_examples.push_back(make_pair(iter_num, exs_new));
  }
}
/* class meas_mh_data end */

// fmt: <accepted?> <error cost> <perf cost> <relative coding> <absolute coding>
void store_proposals_to_file(string file_name,
                             const meas_mh_data &d,
                             const vector<prog> &optimals) {
  if (! d._mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<accepted?> <error cost> <perf cost> <relative coding> <absolute coding>" << endl;
  for (size_t i = 0; i < d._proposals.size(); i++) {
    prog p(d._proposals[i].first);
    fout << d._proposals[i].second << " "
         << p._error_cost << " "
         << p._perf_cost << " "
         << p.prog_rel_bit_vec(optimals) << " "
         << p.prog_abs_bit_vec() << endl;
  }
  fout.close();
}

// fmt: <iter num> <error cost> <perf cost> <relative coding> <absolute coding>
void store_programs_to_file(string file_name,
                            const meas_mh_data &d,
                            const vector<prog> &optimals) {
  if (! d._mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<iter num> <error cost> <perf cost> <relative coding> <absolute coding>" << endl;
  for (size_t i = 0; i < d._programs.size(); i++) {
    prog p(d._programs[i].second);
    fout << d._programs[i].first << " "
         << p._error_cost << " "
         << p._perf_cost << " "
         << p.prog_rel_bit_vec(optimals) << " "
         << p.prog_abs_bit_vec() << endl;
  }
  fout.close();
}

// fmt: <iter_num> <new examples>
void store_examples_to_file(string file_name,
                            const meas_mh_data &d) {
  if (! d._mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<iter_num> <new examples>" << endl;
  for (size_t i = 0; i < d._examples.size(); i++) {
    fout << d._examples[i].first << " "
         << d._examples[i].second._exs << endl;
  }
  fout.close();
}

void store_optimals_to_file(string file_name,
                            const vector<prog> &optimals,
                            bool measure_mode) {
  if (! measure_mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<absolute coding>" << endl;
  for (size_t i = 0; i < optimals.size(); i++)
    fout << optimals[i].prog_abs_bit_vec() << endl;
  fout.close();
}
