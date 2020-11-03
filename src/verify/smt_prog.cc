#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "smt_prog.h"

using namespace z3;

ostream& operator<<(ostream& out, vector<expr>& _expr_vec) {
  if (_expr_vec.size() > 0) {
    out << "\n";
  }
  for (size_t i = 0; i < _expr_vec.size(); i++) {
    out << i << ": " << _expr_vec[i] << "\n";
  }
  return out;
}

ostream& operator<<(ostream& out, vector<vector<expr> >& _expr_vec) {
  if (_expr_vec.size() > 0) {
    out << "\n";
  }
  for (size_t i = 0; i < _expr_vec.size(); i++) {
    out << "block" << i << ": " << _expr_vec[i] << "\n";
  }
  return out;
}

bool is_smt_valid(expr smt) {
  solver s(smt_c);
  s.add(!smt);
  switch (s.check()) {
    case unsat: return true;
    case sat: return false;
    case unknown: return false;
  }
  return false;
}

/* class smt_prog start */
smt_prog::smt_prog() {}

smt_prog::~smt_prog() {}

// assume Block has no branch and is an ordered sequence of instructions
void smt_prog::smt_block(expr& smt_b, expr& smt_sc,
                         inst* program, int start, int end, smt_var& sv, size_t cur_bid) {
  expr p = string_to_expr("true");
  smt_sc = Z3_true;
  for (size_t i = start; i <= end; i++) {
    int op_type = program[i].get_opcode_type();
    if ((op_type == OP_OTHERS) || (op_type == OP_LD) ||
        (op_type == OP_ST) || (op_type == OP_CALL)) {
      // call smt_inst_safety_chk() first, this function does not update the register expression version
      // while smt_inst() will update
      smt_sc = smt_sc && program[i].smt_inst_safety_chk(sv);
      p = p && program[i].smt_inst(sv, cur_bid);
    }
  }
  // generate program output expression if the block is an end block
  if (g.nodes_out[cur_bid].size() == 0) {
    p = p && program[end].smt_inst_end(sv);
  }
  smt_b = p.simplify();
}

// init variables in class
void smt_prog::init(unsigned int num_regs) {
  post_reg_val.clear();
  path_con.clear();
  reg_iv.clear();
  bl.clear();
  post.clear();

  size_t block_num = g.nodes.size();

  // Assume the parent of the root basic block is -1
  g.nodes_in[0].push_back(-1);

  post_reg_val.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    post_reg_val[i].resize(num_regs, string_to_expr("true"));
  }

  path_con.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    // Because of the corresponding relationship between path_con and g.nodes_in,
    // the size of path_con[i] is equal to that of g.nodes_in[i];
    // Here alse set the path_con of the root basic block is Z3_true
    path_con[i].resize(g.nodes_in[i].size(), string_to_expr("true"));
  }

  reg_iv.resize(block_num);
  for (size_t i = 0; i < block_num; i++) {
    reg_iv[i].resize(g.nodes_in[i].size(), string_to_expr("true"));
  }
  bl.resize(block_num, string_to_expr("true"));
  post.resize(block_num, string_to_expr("true"));
}

// may need to modify
void smt_prog::gen_block_prog_logic(expr& e, expr& f_mem, expr& f_sc,
                                    smt_var& sv, size_t cur_bid, inst* inst_lst) {
  // b_pc: basic block path condition,
  // don't care about the default value, gen_block_c_in will reset b_pc
  expr b_pc = Z3_false;
  gen_block_c_in(b_pc, cur_bid);
  smt_var_bl sv_bl;
  sv_bl.store_state_before_smt_block(sv);
  smt_block(e, f_sc, inst_lst, g.nodes[cur_bid]._start, g.nodes[cur_bid]._end, sv, cur_bid);
  f_mem = sv_bl.gen_smt_after_smt_block(sv, b_pc);
  bl[cur_bid] = e; // store
}

void smt_prog::store_post_reg_val(smt_var& sv, size_t cur_bid, unsigned int num_regs) {
  for (size_t i = 0; i < num_regs; i++) {
    post_reg_val[cur_bid][i] = sv.get_cur_reg_var(i);
  }
}

// "p_con" is (path condition reaching cur_bid basic block) && (branch condition of cur_bid -> next_bid)
// update ALL path_con[next_bid][i] where g.nodes_in[next_bid][i] == cur_bid
// Why all: jmp distance is 0, for the next basic block, there may be two same in basic block
void smt_prog::add_path_cond(expr p_con, size_t cur_bid, size_t next_bid) {
  // keep the one-to-one correspondence between g.nodes_in[next_bid][idx] and path_con[next_bid][idx]
  for (size_t i = 0; i < path_con[next_bid].size(); i++) {
    if (g.nodes_in[next_bid][i] != cur_bid) continue;
    path_con[next_bid][i] = p_con;
    // cannot add a "break" or "return" here, since all
    // g.nodes_in[next_bid][i] == cur_bid cases shoud be processed
  }
}

// generate path condition formula that goes through basic block "cur_bid"
// i.e., \/ (path conditions from ALL incoming edges of basic block "cur_bid")
void smt_prog::gen_block_c_in(expr& c_in, size_t cur_bid) {
  if (path_con[cur_bid].size() > 0) { // calculate c_in by parents, that is, path_con[cur_bid]
    c_in = string_to_expr("false"); // if c_in is "ture", the "||" makes the final c_in always "true"
    for (size_t i = 0; i < path_con[cur_bid].size(); i++) {
      c_in = c_in || path_con[cur_bid][i];
    }
  }
  c_in = c_in.simplify();
}

// steps:
// 1. calculate path condition "c_in" of the current basic block
// 2. calculate post path (branch) condition generated by current basic block (jmp instruction);
// 3. use path condition and branch condition to update path_con[next_bid]
// three cases: 1. no next blocks 2. one next block 3. two next blocks
void smt_prog::gen_post_path_con(smt_var& sv, size_t cur_bid, inst& inst_end) {
  // case 1: no next blocks
  if (g.nodes_out[cur_bid].size() == 0) {
    return;
  }

  // get the path condition "c_in" reaching cur basic block
  expr c_in = string_to_expr("true");
  gen_block_c_in(c_in, cur_bid);
  // When cur_bid is processed, path_con[cur_bid] already has been
  // updated with the correct value because of topo sort
  // case 2: one next block
  // In this case, the path condition is the same as that of parents
  if ((g.nodes_out[cur_bid].size() == 1) ||
      (g.nodes_out[cur_bid][0] == g.nodes_out[cur_bid][1])) {
    unsigned int next_bid = g.nodes_out[cur_bid][0];
    // update path condition to the path_con[next_bid]
    add_path_cond(c_in, cur_bid, next_bid);
    return;
  }
  // case 3: two next blocks: no jmp and jmp have two different next blocks
  // keep order: !e: no jmp branch condition; e: jmp branch condition,
  expr e = inst_end.smt_inst_jmp(sv);
  // keep order: insert no jmp first
  // no jmp
  expr pc_no_jmp = c_in && (!e);
  unsigned int next_bid = g.nodes_out[cur_bid][0];
  add_path_cond(pc_no_jmp, cur_bid, next_bid);
  // jmp
  expr pc_jmp = c_in && e;
  next_bid = g.nodes_out[cur_bid][1];
  add_path_cond(pc_jmp, cur_bid, next_bid);
}

void smt_prog::get_init_val(expr& f_iv, smt_var& sv, size_t in_bid, unsigned int num_regs) {
  expr e = (sv.get_init_reg_var(0) == post_reg_val[in_bid][0]);
  for (size_t i = 1; i < num_regs; i++) {
    e = e && (sv.get_init_reg_var(i) == post_reg_val[in_bid][i]);
  }
  f_iv = e;
}

void smt_prog::init_pgm_dag(unsigned int root_node) {
  sv.pgm_dag.init(g.nodes.size(), root_node);
  for (int i = 0; i < g.nodes.size(); i++) {
    for (int j = 0; j < g.nodes_out[i].size(); j++) {
      sv.pgm_dag.add_edge_a2b(i, g.nodes_out[i][j]);
    }
  }
  // cout << sv.pgm_dag << endl;
}

expr smt_prog::gen_smt(unsigned int prog_id, inst* inst_lst, int length, bool is_win, int win_start, int win_end) {
  try {
    // generate a cfg
    // illegal input would be detected: 1. program with loop
    // 2. program that goes to the invalid instruction
    if (is_win) {
      g.gen_graph(&inst_lst[win_start], win_end - win_start + 1);
    } else {
      g.gen_graph(inst_lst, length);
    }
    // cout << "graph: " << g << endl;
  } catch (const string err_msg) {
    throw (err_msg);
  }

  if (is_win) {
    // check whether window program is in one basic block
    bool is_in_one_block = false;
    graph g_full(inst_lst, length);
    for (int i = 0; i <= g_full.nodes.size(); i++) {
      if ((g_full.nodes[i]._start <= win_start) && (win_end <= g_full.nodes[i]._end)) {
        is_in_one_block = true;
        break;
      }
    }
    if (! is_in_one_block) {
      string err_msg = "window not in one basic block";
      throw (err_msg);
    }
    expr f_wl = Z3_true;
    sv.init(prog_id, 0, NUM_REGS, 1, is_win);
    smt_block(f_wl, p_sc, inst_lst, win_start, win_end, sv, 0);
    return f_wl;
  }

  // init class variables
  const unsigned int num_regs = NUM_REGS;
  init(num_regs);

  // blocks stores the block IDs in order after topological sorting
  vector<unsigned int> blocks;
  topo_sort_for_graph(blocks, g);
  init_pgm_dag(blocks[0]);
  // basic block FOL formula;
  // f_block[b] = f_block[b](in_path_1) ^ f_block[b](in_path_2) ^ ...
  // f_block[b](in_path) = implies(path_con[b](in_path), f_iv[b](in_b) && f_bl[b](in_path))
  vector<expr> f_block;
  f_block.resize(g.nodes.size(), string_to_expr("true"));
  p_sc = Z3_true;
  sv.init(prog_id, blocks[0], num_regs, g.nodes.size());
  // process each basic block in order
  for (size_t i = 0; i < blocks.size(); i++) {
    unsigned int b = blocks[i];
    // reset register version for different basic blocks, while memory tables
    // don't need to be reset for being shared among basic blocks.
    sv.set_new_node_id(b, g.nodes_in[b], path_con[b], post_reg_val);
    // generate f_bl: the block program logic
    expr f_bl = string_to_expr("true");
    // f_mem: the constrains on whether variables in memory tables are valid or not
    // because of basic block path condition
    expr f_mem = Z3_true;
    // f_sc: constraints of safty check
    expr f_sc = Z3_true;
    // f_block_pc: constraints of block's path condition
    expr f_block_pc = Z3_true;
    gen_block_prog_logic(f_bl, f_mem, f_sc, sv, b, inst_lst);
    f_block[b] = f_block[b] && f_mem;
    if (b == 0) {
      // basic block 0 does not have pre path condition
      // and its f_iv is the whole program's pre condition which is stored in variable pre of class validator
      f_block[0] = f_block[0] && f_bl;
    } else {
      for (size_t j = 0; j < g.nodes_in[b].size(); j++) {
        // generate f_iv: the logic that the initial values are fed by the last basic block
        expr f_iv = string_to_expr("true");
        get_init_val(f_iv, sv, g.nodes_in[b][j], num_regs);
        reg_iv[b][j] = f_iv; // store
        // f_block[b] = f_block[b] && f_mem && implies(path_con[b][j], f_iv && f_bl);
        f_block[b] = f_block[b] && implies(path_con[b][j], f_iv && f_bl);
        f_block_pc = f_block_pc || path_con[b][j];
      }
    }
    p_sc = p_sc && implies(f_block_pc, f_sc);
    // store post iv for current basic block b
    store_post_reg_val(sv, b, num_regs);
    // update post path condtions "path_con" created by current basic block b
    gen_post_path_con(sv, b, inst_lst[g.nodes[b]._end]);
  }
  // cout << "program final memory tables" << endl;
  // cout << sv.mem_var << endl;
  // program FOL formula; f_prog = f_block[0] && ... && f_block[n]
  expr f_prog = f_block[0];
  for (size_t i = 1; i < f_block.size(); i++) {
    f_prog = f_prog && f_block[i];
  }
  pl = f_prog.simplify();
  return pl;
}
/* class smt_prog end */
