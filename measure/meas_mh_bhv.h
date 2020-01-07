#pragma once

#include <utility>
#include <vector>
#include "../src/isa/toy-isa/inst.h"
#include "../src/isa/prog.h"
#include "../src/utils.h"
#include "../src/inout.h"

using namespace std;

/* Class meas_mh_data is used to store measurement data when mh_sampler is sampling.
 * It ONLY stores data when `_mode` is set as `true`. Now, it is supported to store
 * three kinds of data, that is, proposals, programs and examples, the details are in
 * class commments.
 */
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
  void insert_examples(unsigned int iter_num, const inout &exs);
};

/* The following `store_[]_to_file` functions store raw data of various objects
 * into files. It ONLY works when `_mode` in class `meas_mh_data` is set as `true`
 */
void store_proposals_to_file(string file_name,
                             const meas_mh_data &d,
                             const vector<prog> &optimals);
void store_programs_to_file(string file_name,
                            const meas_mh_data &d,
                            const vector<prog> &optimals);
void store_examples_to_file(string file_name,
                            const meas_mh_data &d);
void store_optimals_to_file(string file_name,
                            const vector<prog> &optimals,
                            bool measure_mode);
void meas_store_raw_data(meas_mh_data &d, string meas_path_out, string suffix,
                         int meas_bm, vector<prog> &bm_optimals);

void gen_optis_for_progs(const vector<inst*> &bm_optis_orig, vector<prog> &bm_optimals);
