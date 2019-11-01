#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>
#include "inst.h"

using namespace std;

typedef unordered_map<unsigned int, unsigned int> unsigned_map;

class node {
 private:
 public:
  unsigned int _start = 0; // start intruction ID
  unsigned int _end = 0;   // end instruction ID
  node(unsigned int start, unsigned int end);
  ~node();
  string to_str();
  friend ostream& operator<<(ostream& out, const node& n);
};

class graph {
 private:
  size_t get_end_inst_id(inst* inst_lst, size_t start, size_t end);
  void gen_node_starts(inst* inst_lst, int length, set<size_t>& node_starts);
  void gen_node_ends(inst* inst_lst, int length, set<size_t>& node_starts, vector<size_t>& node_ends);
  void gen_all_nodes_graph(vector<node>& gnodes, set<size_t>& node_starts, vector<size_t>& node_ends);
  void gen_all_edges_graph(vector<vector<unsigned int> >& gnodes_out, vector<node>& gnodes, inst* inst_lst);
  void gen_id_map(unsigned_map& id_map, vector<node>& gnodes);
  void add_node(node& nd, unsigned int& added);
  void dfs(size_t cur_gnode_id, vector<node>& gnodes, vector<vector<unsigned int> >& gnodes_out, \
           vector<unsigned int>& added, vector<bool>& visited, vector<bool>& finished);
  void init();
 public:
  vector<node> nodes;
  vector<vector<unsigned int> > nodes_in;
  vector<vector<unsigned int> > nodes_out;
  graph();
  graph(inst* inst_lst, int length);
  ~graph();
  void gen_graph(inst* inst_lst, int length);
  friend ostream& operator<<(ostream& out, const graph& g);
};

#define CFG_OTHERS 0
#define CFG_CONDJMP 1
#define CFG_END 2
#define CFG_NOP 3
int get_inst_type(inst& ins);

