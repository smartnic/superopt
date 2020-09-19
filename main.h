#pragma once

#include <vector>

using namespace std;

struct input_paras {
  int niter;
  int bm;
  bool bm_from_file;
  string bytecode;
  string map;
  string desc;
  double w_e;
  double w_p;
  bool meas_mode;
  string path_out;
  int st_ex;
  int st_eq;
  int st_avg;
  int st_perf;
  int st_when_to_restart;
  int st_when_to_restart_niter;
  int st_start_prog;
  vector<double> restart_w_e_list;
  vector<double> restart_w_p_list;
  int reset_win_niter;
  vector<int> win_s_list;
  vector<int> win_e_list;
  double p_inst_operand;
  double p_inst;
  int server_port;
  bool disable_prog_eq_cache;
};
