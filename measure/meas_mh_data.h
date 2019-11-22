#pragma once

#include <utility>
#include <vector>
#include "../prog.h"
#include "../inout.h"

using namespace std;

class meas_mh_data {
 public:
  // true: measure; false: do not measure
  bool _mode;
  // (proposal program, accepted or rejected)
  vector<pair<prog, bool> > _proposals;
  // (iteration number, sampled program)
  vector<pair<unsigned int, prog> > _programs;
  // (iteration number, new examples)
  vector<pair<unsigned int, examples> > _examples;
  meas_mh_data();
  ~meas_mh_data();
  void insert_proposal(const prog &proposal, bool accepted);
  void insert_program(unsigned int iter_num, const prog &program);
  void insert_examples(unsigned int iter_num, const examples &exs);
};

// proposal raw data to a file
void store_proposals_to_file(string file_name,
                             const meas_mh_data &d,
                             const vector<prog> &optimals);
void store_programs_to_file(string file_name,
                            const meas_mh_data &d,
                            const vector<prog> &optimals);
void store_examples_to_file(string file_name,
                            const meas_mh_data &d);
void store_optimals_to_file(string file_name,
                            const vector<prog> &optimals);
