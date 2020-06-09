#pragma once

#include <vector>
#include "z3++.h"
#include "../../src/utils.h"
#include "cfg.h"
#include "../../src/isa/inst_header.h"

using namespace z3;

/* smt_prog algorithm document: https://github.com/ngsrinivas/superopt/tree/master/doc */

ostream& operator<< (ostream& out, vector<expr>& _expr_vec);
ostream& operator<< (ostream& out, vector<vector<expr> >& _expr_vec);
bool is_smt_valid(expr smt);

class smt_prog {
 private:
  // post_reg_val[i] is post register values of basic block i,
  // which are initial values for NEXT basic blocks
  vector<vector<expr> > post_reg_val;
  // return the SMT for the given program without branch and loop
  void smt_block(expr& smt_b, inst* program, int start, int end, smt_var& sv, expr b_pc);
  void init(unsigned int num_regs);
  void topo_sort_dfs(size_t cur_bid, vector<unsigned int>& blocks, vector<bool>& finished);
  void gen_block_prog_logic(expr& e, expr& f_mem, smt_var& sv, size_t cur_bid, inst* inst_lst);
  void store_post_reg_val(smt_var& sv, size_t cur_bid, unsigned int num_regs);
  void add_path_cond(expr p_con, size_t cur_bid, size_t next_bId);
  void gen_post_path_con(smt_var& sv, size_t cur_bid, inst& inst_end);
  void get_init_val(expr& f_iv, smt_var& sv, size_t in_bid, unsigned int num_regs);
  expr smt_end_block_inst(size_t cur_bid, inst& inst_end, unsigned int prog_id);
  void gen_block_c_in(expr& c_in, size_t cur_bid);
  void process_output(expr& f_p_output, inst* inst_lst, unsigned int prog_id);
 public:
  // `public` for unit test check
  smt_var sv;
  // program logic
  expr pl = string_to_expr("true");
  // store path_con, reg_iv, bl, post, g
  // 1. path_con[i] stores pre path condition formulas of basic block i
  // There is a corresponding relationship between path_con and g.nodesIn
  // more specifically, path_con[i][j] stores the all pre path condition formulae from basic block g.nodesIn[i][j] to i
  vector<vector<expr> > path_con;
  // 2. reg_iv[i][j] stores pre register initial value formula
  // that values from the last node(g.nodes[i][j]) are fed to the node(i)
  vector<vector<expr> > reg_iv;
  // 3. bl[i] stores block logic formula of basic block i
  // more specifically, bl[i] = instLogic_i_0 && instLogic_i_1 && ... && instLogic_i_n
  vector<expr> bl;
  // 4. post[i] store post logic formula (output formula) for the end basic block i
  vector<expr> post;
  // control flow graph
  graph g;
  smt_prog();
  ~smt_prog();
  // Return the program logic FOL formula 'PL' including basic program logic
  // and the formula of capturing the output of the program in the variable output[prog_id]
  expr gen_smt(unsigned int prog_id, inst* inst_lst, int length);
};
