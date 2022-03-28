#pragma once

#include <vector>

using namespace std;

class input_paras {
 public:
  int niter;
  unsigned int k;
  int bm;
  bool bm_from_file;
  string bytecode;
  string map;
  string desc;
  double w_e;
  double w_p;
  //added weight for safety
  double w_s;
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
  double p_inst_as_nop;
  bool disable_prog_eq_cache;
  bool enable_prog_uneq_cache;
  bool is_win;
  int logger_level;
  int server_port;
  // memory exchange move related
  bool disable_move_mem_exchange;
  bool disable_move_mem_exchange_gen_operands;
  //add an int flag (repair(1)/optimize(k2)(0))
  //refer to main.cc code
  //default value: optimize(0)
  //for now: public member in cost class
  //if needed in other classes, then as a globalvalue
  int functionality;
};
