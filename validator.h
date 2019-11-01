#pragma once

#include <vector>
#include "inst.h"
#include "cfg.h"
#include "z3++.h"

using namespace z3;

/* Validator algorithm document: https://github.com/ngsrinivas/superopt/tree/doc/doc */

// convert string s into expr e, the type of e is int_const
expr string_to_expr(string s);
ostream& operator<< (ostream& out, vector<expr>& _expr_vec);
ostream& operator<< (ostream& out, vector<vector<expr> >& _expr_vec);
bool is_smt_valid(expr smt);

// SMT Variable format
// [type]_[prog_id]_[node_id]_[reg_id/mem_id]_[version_id]
// [type]: r means register; m means memory
class smt_var {
 private:
  string _name;
  // store the curId
  unsigned int reg_cur_id[NUM_REGS];
  std::vector<expr> reg_var;
 public:
  // 1. Convert prog_id and node_id into _name, that is string([prog_id]_[node_id])
  // 2. Initialize reg_val[i] = r_[_name]_0, i = 0, ..., NUM_REGS
  smt_var(unsigned int prog_id, unsigned int node_id);
  ~smt_var();
  // inital value for [versionId] is 0, and increases when updated
  expr update_reg_var(unsigned int reg_id);
  expr get_cur_reg_var(unsigned int reg_id);
  expr get_init_reg_var(unsigned int reg_id);
};

class prog_smt {
 private:
  // post_reg_val[i] is post register values of basic block i,
  // which are initial values for NEXT basic blocks
  vector<vector<expr> > post_reg_val;
  // return the SMT for the given program without branch and loop
  void smt_block(expr& smt_b, inst* program, int length, smt_var* sv);
  // return SMT for the given CFG_OTHERS type instruction, other types return false
  expr smt_inst(smt_var* sv, inst* in);
  void init();
  void topo_sort_dfs(size_t cur_bid, vector<unsigned int>& blocks, vector<bool>& finished);
  void gen_block_prog_logic(expr& e, smt_var* sv, size_t cur_bid, inst* inst_lst);
  void store_post_reg_val(smt_var* sv, size_t cur_bid);
  void smt_jmp_inst(smt_var* sv, vector<expr>& c_inst_end, inst& inst_end);
  void add_path_cond(expr p_con, size_t cur_bid, size_t next_bId);
  void gen_post_path_con(smt_var* sv, size_t cur_bid, inst& inst_end);
  void get_init_val(expr& f_iv, smt_var* sv, size_t in_bid);
  expr smt_end_block_inst(size_t cur_bid, inst* inst_end, unsigned int prog_id);
  void gen_block_c_in(expr& c_in, size_t cur_bid);
  void process_output(expr& f_p_output, inst* inst_lst, unsigned int prog_id);
 public:
  // program logic
  expr pl = string_to_expr("true");
  // store path_con, reg_iv, bl, post, g
  // 1. path_con[i] stores pre path condition formulas of basic block i
  // There is a corresponding relationship between path_con and g.nodesIn
  // more specifically, path_con[i][j] stores the pre path condition formula from basic block g.nodesIn[i][j] to i
  vector<vector<expr> > path_con;
  // 2. reg_iv[i][j] stores pre register initial value formula
  // that values from the last node(g.nodes[i][j]) are fed to the node(i)
  vector<vector<expr> > reg_iv;
  // 3. bl[i] stores block logic formula of basic block i
  // more specifically, bl[i] = instLogic_i_0 && instLogic_i_1 && ... && instLogic_i_n
  vector<expr> bl;
  // 4. post[i][j] store post logic formula for basic block i
  // If block i is the end block, post[i][j] stores output formula
  // otherwise, post[i][j] stores post path condition formula.
  vector<vector<expr> > post;
  // control flow graph
  graph g;
  prog_smt();
  ~prog_smt();
  // Return the program logic FOL formula 'PL' including basic program logic
  // and the formula of capturing the output of the program in the variable output[prog_id]
  expr gen_smt(unsigned int prog_id, inst* inst_lst, int length);
};

class validator {
 private:
  // set register 0 in basic block 0 as input_i
  void smt_pre(expr& pre, unsigned int prog_id);
  // set the input variable of FOL formula as input_i
  void smt_pre(expr& pre, expr e);
  // setting outputs of two programs are equal
  void smt_post(expr& pst, unsigned int prog_id1, unsigned int prog_id2);
  void smt_post(expr& pst, unsigned int prog_id, expr e);
  void init();
 public:
  // store
  // pre[i]: input formula of program i: setting register 0 in basic block 0 as input_i
  // or the input variable of FOL formula as input_i
  vector<expr> pre;
  // ps[i]: program logic formula, including basic program logic
  // and the formula of capturing the output of the program in the variable output[prog_id]
  vector<prog_smt> ps;
  // two program's output formula of setting outputs of two programs are equal, i.e., output_prog_id1 == output2_prog_id2
  expr post = string_to_expr("true");
  // f = pre^pre2^p1^p2 -> post
  expr f = string_to_expr("true");

  validator();
  ~validator();
  // check whether two programs have the same logic
  bool equal_check(inst* inst_lst1, int len1, inst* inst_lst2, int len2);
  // check whether the program and the FOL formula have the same logic
  // fx is the FOL formula, input/output is the input/output variable of fx
  bool equal_check(inst* inst_lst1, int len1, expr fx, expr input, expr output);
};
