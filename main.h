#pragma once

#include <vector>

using namespace std;

struct input_paras {
  int isa;
  int niter;
  int bm;
  double w_e;
  double w_p;
  bool meas_mode;
  string path_out;
  int st_ex;
  int st_eq;
  int st_avg;
  int st_when_to_restart;
  int st_when_to_restart_niter;
  int st_start_prog;
  vector<double> restart_w_e_list;
  vector<double> restart_w_p_list;
  double p_inst_operand;
  double p_inst;
};
