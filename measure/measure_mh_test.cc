#include <iostream>
#include <fstream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <map>
#include <set>
#include <utility>
#include "../prog.h"
#include "../inout.h"
#include "../mh_prog.h"
#include "../inst.h"

using namespace std;

string file_config_store = "config";
string file_raw_data_prog = "raw_data_prog";
string file_raw_data_prog_dic = "raw_data_prog_dic";
string file_raw_data_proposal = "raw_data_proposal";
string file_raw_data_proposal_prog = "raw_data_proposal_prog";
string file_raw_data_ex = "raw_data_ex";
string file_raw_data_optimal = "raw_data_optimal";
vector<inst*> origs;
vector<vector<inst*> > optis;
int origs_len = MAX_PROG_LEN;
vector<int> origs_best_perf_cost;
vector<int> inputs;
std::unordered_map<int, vector<prog*> > prog_dic;

default_random_engine gen_mh_test;
uniform_real_distribution<double> unidist_mh_test(0.0, 1.0);

// output = max(input+4, 15)
// min_len: 3
// perf_cost = 3 + 1 = 4
inst orig0[MAX_PROG_LEN] = {inst(MOVXC, 2, 4),  /* mov r2, 4  */
                            inst(ADDXY, 0, 2),  /* add r0, r2 */
                            inst(MOVXC, 3, 15),  /* mov r3, 15  */
                            inst(JMPGT, 0, 3, 1),  /* if r0 <= r3: */
                            inst(RETX, 3),      /* ret r3 */
                            inst(RETX, 0),      /* else ret r0 */
                            NOP,  /* control never reaches here */
                           };
inst opti00[MAX_PROG_LEN] = {inst(MOVXC, 1, 4),
                             inst(ADDXY, 0, 1),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti01[MAX_PROG_LEN] = {inst(MOVXC, 2, 4),
                             inst(ADDXY, 0, 2),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti02[MAX_PROG_LEN] = {inst(MOVXC, 3, 4),
                             inst(ADDXY, 0, 3),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti03[MAX_PROG_LEN] = {inst(MAXC, 1, 4),
                             inst(ADDXY, 0, 1),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti04[MAX_PROG_LEN] = {inst(MAXC, 2, 4),
                             inst(ADDXY, 0, 2),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti05[MAX_PROG_LEN] = {inst(MAXC, 3, 4),
                             inst(ADDXY, 0, 3),
                             inst(MAXC, 0, 15),
                             inst(),
                             inst(),
                             inst(),
                             inst(),
                            };
// f(x) = min(x, r1, 10) = min(x, 0)
// min_len: 2
// perf_cost = 2
inst orig1[MAX_PROG_LEN] = {inst(JMPLE, 0, 1, 2), // skip r0 <- r1, if r0 <= r1
                            inst(MOVXC, 0, 0),
                            inst(ADDXY, 0, 1),
                            inst(MOVXC, 1, 10),   // r1 <- 10
                            inst(JMPLE, 0, 1, 1), // if r0 <= r1, return r0, else r1
                            inst(RETX, 1),
                            inst(RETX, 0),
                           };
// inst opti10[MAX_PROG_LEN] = {inst(JMPLE, 0, 1, 1),
//                              inst(RETX, 1),
//                              inst(),
//                              inst(),
//                              inst(),
//                              inst(),
//                              inst(),
//                             };
// inst opti11[MAX_PROG_LEN] = {inst(JMPLT, 0, 1, 1),
//                              inst(RETX, 1),
//                              inst(),
//                              inst(),
//                              inst(),
//                              inst(),
//                              inst(),
//                             };
// f(x) = max(2*x, x+4)
// MOVC 1 4
// MAXX 1 0
// ADDXY 0 1
// perf_cost = 3+1=4
inst orig2[MAX_PROG_LEN] = {inst(ADDXY, 1, 0),
                            inst(MOVXC, 2, 4),
                            inst(ADDXY, 1, 2), // r1 = r0+4
                            inst(ADDXY, 0, 0), // r0 += r0
                            inst(MAXX, 1, 0),  // r1 = max(r1, r0)
                            inst(RETX, 1),
                            NOP,
                           };
inst opti20[MAX_PROG_LEN] = {inst(MOVXC, 1, 4),
                             inst(MAXX, 1, 0),
                             inst(ADDXY, 0, 1),
                             inst(ADDXY, 0, 1),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti21[MAX_PROG_LEN] = {inst(MOVXC, 2, 4),
                             inst(MAXX, 2, 0),
                             inst(ADDXY, 0, 2),
                             inst(ADDXY, 0, 2),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti22[MAX_PROG_LEN] = {inst(MOVXC, 3, 4),
                             inst(MAXX, 3, 0),
                             inst(ADDXY, 0, 3),
                             inst(ADDXY, 0, 3),
                             inst(),
                             inst(),
                             inst(),
                            };
// f(x) = 6*x
// ADDXY 0 0  r0 = 2*r0
// ADDXY 1 0  r1 = 2*r0
// ADDXY 0 1  r0 = 4*r0
// ADDXY 0 1  r0 = 6*r0
// perf_cost = 4+0 = 4
inst orig3[MAX_PROG_LEN] = {inst(MOVXC, 1, 0),
                            inst(ADDXY, 1, 0), // r1 = 2*r0
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                            inst(ADDXY, 0, 1),
                           };
inst opti30[MAX_PROG_LEN] = {inst(ADDXY, 0, 0),
                             inst(ADDXY, 1, 0),
                             inst(ADDXY, 0, 1),
                             inst(ADDXY, 0, 1),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti31[MAX_PROG_LEN] = {inst(ADDXY, 0, 0),
                             inst(ADDXY, 2, 0),
                             inst(ADDXY, 0, 2),
                             inst(ADDXY, 0, 2),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti32[MAX_PROG_LEN] = {inst(ADDXY, 0, 0),
                             inst(ADDXY, 3, 0),
                             inst(ADDXY, 0, 3),
                             inst(ADDXY, 0, 3),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti33[MAX_PROG_LEN] = {inst(ADDXY, 1, 0),
                             inst(ADDXY, 1, 1),
                             inst(ADDXY, 0, 1),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti34[MAX_PROG_LEN] = {inst(ADDXY, 2, 0),
                             inst(ADDXY, 2, 2),
                             inst(ADDXY, 0, 2),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti35[MAX_PROG_LEN] = {inst(ADDXY, 1, 0),
                             inst(ADDXY, 0, 1),
                             inst(ADDXY, 0, 1),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti36[MAX_PROG_LEN] = {inst(ADDXY, 2, 0),
                             inst(ADDXY, 0, 2),
                             inst(ADDXY, 0, 2),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti37[MAX_PROG_LEN] = {inst(ADDXY, 3, 0),
                             inst(ADDXY, 0, 3),
                             inst(ADDXY, 0, 3),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
inst opti38[MAX_PROG_LEN] = {inst(ADDXY, 3, 0),
                             inst(ADDXY, 3, 3),
                             inst(ADDXY, 0, 3),
                             inst(ADDXY, 0, 0),
                             inst(),
                             inst(),
                             inst(),
                            };
ostream& operator<<(ostream& out, vector<int>& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << v[i] << " ";
  }
  return out;
}

ostream& operator<<(ostream& out, vector<vector<int> >& v) {
  for (size_t i = 0; i < v.size(); i++) {
    out << i << ": " << v[i] << endl;
  }
  return out;
}

void init_origs() {
  for (int i = 0; i < 4; i++)
    optis.push_back(vector<inst*> {});
  origs.push_back(orig0);
  optis[0].push_back(opti00);
  optis[0].push_back(opti01);
  optis[0].push_back(opti02);
  optis[0].push_back(opti03);
  optis[0].push_back(opti04);
  optis[0].push_back(opti05);
  origs_best_perf_cost.push_back(4);
  origs.push_back(orig1);
  // optis.push_back(opti1);
  origs_best_perf_cost.push_back(2);
  origs.push_back(orig2);
  optis[2].push_back(opti20);
  optis[2].push_back(opti21);
  optis[2].push_back(opti22);
  origs_best_perf_cost.push_back(4);
  origs.push_back(orig3);
  optis[3].push_back(opti30);
  optis[3].push_back(opti31);
  optis[3].push_back(opti32);
  optis[3].push_back(opti33);
  optis[3].push_back(opti34);
  optis[3].push_back(opti35);
  optis[3].push_back(opti36);
  optis[3].push_back(opti37);
  optis[3].push_back(opti38);
  origs_best_perf_cost.push_back(4);
}

void gen_random_input(vector<int>& inputs, int min, int max) {
  unordered_set<int> input_set;
  for (size_t i = 0; i < inputs.size();) {
    int input = min + (max - min) * unidist_mh_test(gen_mh_test);
    if (input_set.find(input) == input_set.end()) {
      input_set.insert(input);
      inputs[i] = input;
      i++;
    }
  }
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

// generate all combinations that picks n unrepeated numbers from s to e
// row_s is the staring row in `res` that store the combinations
// e.g. s=0, e=3, n=2, row_s=0, res=[[1,2], [1,3], [2,3]]
void gen_n_numbers(int n, int s, int e,
                   int row_s, vector<vector<int> >& res) {
  if (n == 0) return;
  for (int i = s; i <= e - n + 1; i++) {
    int num_comb = combination(e - i, n - 1);
    for (int j = row_s; j < row_s + num_comb; j++)
      res[j].push_back(i);
    gen_n_numbers(n - 1, i + 1, e, row_s, res);
    row_s += num_comb;
  }
}

// should ensure the first real_length instructions in program p are not NOP,
// the remainings are NOP.
void gen_opti_set(const prog& p, const int& len, vector<prog>& opti_set) {
  int n = num_real_instructions((inst*)p.inst_list, len);
  // C_len^n
  int num_opti = combination(len, n);
  vector<vector<int> > comb_set(num_opti);
  gen_n_numbers(n, 0, len - 1, 0, comb_set);
  opti_set.resize(num_opti);
  for (size_t i = 0; i < comb_set.size(); i++) {
    for (size_t j = 0; j < len; j++)
      opti_set[i].inst_list[j] = inst(NOP);
    for (size_t j = 0; j < comb_set[i].size(); j++) {
      size_t pos = comb_set[i][j];
      opti_set[i].inst_list[pos] = p.inst_list[j];
    }
  }
}

void gen_opti_progs(vector<prog> &opti_set, int orig_id) {
  for (size_t i = 0; i < optis[orig_id].size(); i++) {
    vector<prog> op_set;
    prog op(optis[orig_id][i]);
    gen_opti_set(op, MAX_PROG_LEN, op_set);
    for (size_t j = 0; j < op_set.size(); j++)
      opti_set.push_back(op_set[j]);
  }
  cout << "opti_set is\n";
  for (size_t i = 0; i < opti_set.size(); i++) {
    opti_set[i].print();
  }
}

void running_sampler(int orig_id, int len,
                     int nrolls, double w_e, double w_p,
                     int strategy_ex, int strategy_eq, int strategy_avg,
                     vector<prog> &opti_set) {
  mh_sampler mh;
  mh.measure_start(opti_set, file_raw_data_prog,
                   file_raw_data_proposal, file_raw_data_ex);
  prog orig(origs[orig_id]);
  mh._cost.init(&orig, len, inputs, w_e, w_p,
                strategy_ex, strategy_eq, strategy_avg);
  mh.mcmc_iter(nrolls, orig, prog_dic);
  mh.measure_stop();
}

void file_rename(string path, double w_e, double w_p, int orig_id) {
  file_raw_data_prog = path + file_raw_data_prog;
  file_raw_data_prog_dic = path + file_raw_data_prog_dic;
  file_raw_data_proposal_prog = path + file_raw_data_proposal_prog;
  file_raw_data_proposal = path + file_raw_data_proposal;
  file_raw_data_ex = path + file_raw_data_ex;
  file_raw_data_optimal = path + file_raw_data_optimal;
  string str_w_e = to_string(w_e);
  string str_w_p = to_string(w_p);
  str_w_e.erase(str_w_e.find_last_not_of('0') + 1, string::npos);
  str_w_e.erase(str_w_e.find_last_not_of('.') + 1, string::npos);
  str_w_p.erase(str_w_p.find_last_not_of('0') + 1, string::npos);
  str_w_p.erase(str_w_p.find_last_not_of('.') + 1, string::npos);
  string suffix = "_" + to_string(orig_id) +
                  "_" + str_w_e +
                  "_" + str_w_p +
                  ".txt";
  file_raw_data_prog += suffix;
  file_raw_data_prog_dic += suffix;
  file_raw_data_proposal_prog += suffix;
  file_raw_data_proposal += suffix;
  file_raw_data_ex += suffix;
  file_raw_data_optimal += "_" + to_string(orig_id) + ".txt";
}

void store_raw_data(double w_e, double w_p, vector<prog> opti_set) {
  ofstream fout;
  fout.open(file_raw_data_prog_dic, ios::out | ios::trunc);
  for (std::pair<int, vector <prog*> > element : prog_dic) {
    vector<prog*> pl = element.second;
    for (auto p : pl) {
      fout << p->_error_cost << " "
           << p->_perf_cost << " "
           << p->freq_count << endl;
    }
  }
  fout.close();
  fout.open(file_raw_data_optimal, ios::out | ios::trunc);
  for (size_t i = 0; i < opti_set.size(); i++) {
    fout << opti_set[i].prog_abs_bit_vec() << endl;
  }
  fout.close();
}

int main(int argc, char* argv[]) {
  int nrolls = 10;
  double w_e = 1.0;
  double w_p = 0.0;
  int orig_id = 0;
  string path = "";
  int strategy_ex = 0;
  int strategy_eq = 0;
  int strategy_avg = 0;
  if (argc > 1)
    orig_id = atoi(argv[1]);
  if (argc > 2)
    nrolls = atoi(argv[2]);
  if (argc > 4) {
    w_e = atof(argv[3]);
    w_p = atof(argv[4]);
  }
  if (argc > 5) {
    path = argv[5];
  }
  if (argc > 6) {
    strategy_ex = atoi(argv[6]);
  }
  if (argc > 7) {
    strategy_eq = atoi(argv[7]);
  }
  if (argc > 8) {
    strategy_avg = atoi(argv[8]);
  }
  // cout << "start measuring" << endl;
  file_rename(path, w_e, w_p, orig_id);
  init_origs();
  inputs.resize(30);
  gen_random_input(inputs, -50, 50);
  vector<prog> opti_set;
  gen_opti_progs(opti_set, orig_id);
  running_sampler(orig_id, origs_len, nrolls, w_e, w_p,
                  strategy_ex, strategy_eq, strategy_avg,
                  opti_set);
  store_raw_data(w_e, w_p, opti_set);
  return 0;
}
