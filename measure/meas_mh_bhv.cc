#include <iostream>
#include <fstream>
#include <bitset>
#include "meas_mh_bhv.h"

using namespace std;

string FILE_RAW_DATA_PROGRAMS = "raw_data_programs";
string FILE_RAW_DATA_PROPOSALS = "raw_data_proposals";
string FILE_RAW_DATA_EXAMPLES = "raw_data_examples";
string FILE_RAW_DATA_OPTIMALS = "raw_data_optimals";

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

string prog_rel_bv_to_str(int v, int isa_type) {
  switch (isa_type) {
    case TOY_ISA: return bitset<toy_isa::MAX_PROG_LEN>(v).to_string();
    default: cout << "unknown ISA type, return nullptr" << endl; return nullptr;
  }
}

string prog_abs_bv_to_str(vector<int>& v, int isa_type) {
  string str = "";
  switch (isa_type) {
    case TOY_ISA: {
      for (size_t i = 0; i < v.size(); i++)
        str += bitset<toy_isa::INST_NUM_BITS>(v[i]).to_string();
      return str;
    }
    default: cout << "unknown ISA type, return empty string" << endl; return "";
  }
}

// fmt: <accepted?> <error cost> <perf cost> <relative coding> <absolute coding>
void store_proposals_to_file(string file_name,
                             const meas_mh_data &d,
                             const vector<prog> &optimals,
                             int isa_type) {
  if (! d._mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<accepted?> <error cost> <perf cost> <relative coding> <absolute coding>" << endl;
  for (size_t i = 0; i < d._proposals.size(); i++) {
    prog p(d._proposals[i].first);
    vector<int> bv;
    p.to_abs_bv(bv);
    fout << d._proposals[i].second << " "
         << p._error_cost << " "
         << p._perf_cost << " "
         << prog_rel_bv_to_str(p.to_rel_bv(optimals), isa_type) << " "
         << prog_abs_bv_to_str(bv, isa_type) << endl;
  }
  fout.close();
}

// fmt: <iter num> <error cost> <perf cost> <relative coding> <absolute coding>
void store_programs_to_file(string file_name,
                            const meas_mh_data &d,
                            const vector<prog> &optimals,
                            int isa_type) {
  if (! d._mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<iter num> <error cost> <perf cost> <relative coding> <absolute coding>" << endl;
  for (size_t i = 0; i < d._programs.size(); i++) {
    prog p(d._programs[i].second);
    vector<int> bv;
    p.to_abs_bv(bv);
    fout << d._programs[i].first << " "
         << p._error_cost << " "
         << p._perf_cost << " "
         << prog_rel_bv_to_str(p.to_rel_bv(optimals), isa_type) << " "
         << prog_abs_bv_to_str(bv, isa_type) << endl;
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
                            bool measure_mode,
                            int isa_type) {
  if (! measure_mode) return;
  fstream fout;
  fout.open(file_name, ios::out | ios::trunc);
  fout << "<absolute coding>" << endl;
  for (size_t i = 0; i < optimals.size(); i++) {
    vector<int> bv;
    optimals[i].to_abs_bv(bv);
    fout << prog_abs_bv_to_str(bv, isa_type) << endl;
  }
  fout.close();
}

void meas_store_raw_data(meas_mh_data &d, string meas_path_out, string suffix,
                         int meas_bm, vector<prog> &bm_optimals, int isa_type) {
  string file_raw_data_programs = meas_path_out + FILE_RAW_DATA_PROGRAMS + suffix;
  string file_raw_data_proposals = meas_path_out + FILE_RAW_DATA_PROPOSALS + suffix;
  string file_raw_data_examples = meas_path_out + FILE_RAW_DATA_EXAMPLES + suffix;
  string file_raw_data_optimals = meas_path_out + FILE_RAW_DATA_OPTIMALS;
  file_raw_data_optimals += "_" + to_string(meas_bm) + ".txt";
  store_proposals_to_file(file_raw_data_proposals, d, bm_optimals, isa_type);
  store_programs_to_file(file_raw_data_programs, d, bm_optimals, isa_type);
  store_examples_to_file(file_raw_data_examples, d);
  store_optimals_to_file(file_raw_data_optimals, bm_optimals, d._mode, isa_type);
}

// return C_n^m
unsigned int combination(unsigned int n, unsigned m) {
  unsigned int a = 1;
  for (unsigned int i = n; i > (n - m); i--) {
    a *= i;
  }
  unsigned int b = 1;
  for (unsigned int i = 1; i <= m; i++) {
    b *= i;
  }
  return (a / b);
}

// Generate all combinations that picks n unrepeated numbers from s to e
// row_s is the starting row in `res` that stores the combinations
// e.g. s=1, e=3, n=2, row_s=0, res=[[1,2], [1,3], [2,3]]
// steps: compute combinations recursively ranging from large to small,
// while the real computation is from small to large,
// that is, compute combinations in range [s+1:e] first, then [s:e]
void gen_n_combinations(int n, int s, int e,
                        int row_s, vector<vector<int> >& res) {
  if (n == 0) return;
  for (int i = s; i <= e - n + 1; i++) {
    int num_comb = combination(e - i, n - 1);
    for (int j = row_s; j < row_s + num_comb; j++)
      res[j].push_back(i);
    gen_n_combinations(n - 1, i + 1, e, row_s, res);
    row_s += num_comb;
  }
}

// Premise: should ensure the first real_length instructions in program p
// are not NOP, while the remainings are NOP.
// steps: 1. Set all instructions of this optimal program as NOP;
// 2. Compute combinations for real instruction positions;
// 3. replace NOP instructions with real instructions according to combinations.
// e.g. if optimal program has 2 real instuctions, one combination is [2,3],
// then the second and third instructions are replaced with real instructions
void gen_optis_for_prog(const prog& p, const int& len,
                        vector<prog>& opti_set) {
  int n = p.num_real_instructions();
  // C_len^n
  int num_opti = combination(len, n);
  vector<vector<int> > comb_set(num_opti);
  gen_n_combinations(n, 0, len - 1, 0, comb_set);
  opti_set.resize(num_opti, p);
  for (size_t i = 0; i < comb_set.size(); i++) {
    // set all instructions of this optimal program as NOP
    for (size_t j = 0; j < len; j++)
      opti_set[i].instptr_list[j]->set_as_nop_inst();
    // replace some NOP instructions with real instructions
    // according to the combination value
    for (size_t j = 0; j < comb_set[i].size(); j++) {
      size_t pos = comb_set[i][j];
      *opti_set[i].instptr_list[pos] = *p.instptr_list[j];
    }
  }
}

void gen_optis_for_progs(const vector<inst*> &bm_optis_orig, vector<prog> &bm_optimals) {
  for (size_t i = 0; i < bm_optis_orig.size(); i++) {
    prog bm_opti(bm_optis_orig[i]);
    // op_set: temporarily store optimals for one bm optimal program
    vector<prog> op_set;
    gen_optis_for_prog(bm_opti, bm_opti.get_max_prog_len(), op_set);
    for (size_t j = 0; j < op_set.size(); j++)
      bm_optimals.push_back(op_set[j]);
  }
}
