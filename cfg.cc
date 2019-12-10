#include <iostream>
#include "cfg.h"

using namespace std;

/* class node start */
node::node(unsigned int start, unsigned int end) {
  _start = start;
  _end = end;
}

node::~node() {}

string node::to_str( ) {
  string s = "[" + to_string(_start) + ":" + to_string(_end) + "]";
  return s;
}

ostream& operator<<(ostream& out, const node& n) {
  out << "[" << n._start << ":" << n._end << "]";
  return out;
}

void graph::init() {
  nodes.clear();
  nodes_in.clear();
  nodes_out.clear();
}

void graph::gen_node_starts(inst* inst_lst, int length, set<size_t>& node_starts) {
  node_starts.insert(0);
  for (size_t i = 0; i < length; i++) {
    if (opcode_type[inst_lst[i]._opcode] == OP_JMP) {
      if ((i + 1) < length) {
        node_starts.insert(i + 1);
      } else {
        string err_msg = "illegal input: instruction " + to_string(i) + " -> "\
                         "no jmp goes to an invalid instruction " + to_string(i + 1);
        throw (err_msg);
      }
      int d = inst_lst[i]._args[2];
      // Legal inputs : case1 (d >= 0): (i + 1 + d) < length; case2 (d < 0):(i + 1 >= -d)
      if (((d >= 0) && (i + 1 + d < length)) || ((d < 0) && (i + 1 >= -d))) {
        node_starts.insert(i + 1 + d);
      } else {
        string err_msg = "illegal input: instruction " + to_string(i) + " -> "\
                         "jmp goes to an invalid instruction " + to_string((int)(i) + 1 + d);
        throw (err_msg);
      }
    }
  }
}

// return end instruction ID in [start: end]
size_t graph::get_end_inst_id(inst* inst_lst, size_t start, size_t end) {
  for (size_t i = start; i < end; i++) {
    if (opcode_type[inst_lst[i]._opcode] == OP_RET) {
      return i;
    }
  }
  return end;
}

void graph::gen_node_ends(inst* inst_lst, int length, set<size_t>& node_starts, vector<size_t>& node_ends) {
  /* Traverse all starts in node_starts, find an end for each start
     The end for all starts except the last one is the OP_RET instruction OR the ${next start - 1} instruction
     The end for the last start is the OP_RET instruction OR the last instruction.
  */
  // ends for all starts except the last start
  set<size_t>::iterator i = node_starts.begin();
  set<size_t>::iterator i2 = node_starts.begin();
  for (i2++; i2 != node_starts.end(); i++, i2++) {
    // Traverse all instructions in between [start_i: start_i+1)
    size_t end = get_end_inst_id(inst_lst, *i, *i2 - 1);
    node_ends.push_back(end);
  }
  // end for the last start
  size_t end = get_end_inst_id(inst_lst, *i, length - 1);
  node_ends.push_back(end);
}

void graph::gen_all_nodes_graph(vector<node>& gnodes, set<size_t>& node_starts, vector<size_t>& node_ends) {
  // (STL documentation) set iterators traverse the set in increasing values
  // example: http://www.cplusplus.com/reference/set/set/begin/
  // explanation about iterators: http://www.cplusplus.com/reference/iterator/BidirectionalIterator/
  set<size_t>::iterator s = node_starts.begin();
  size_t e = 0;
  // Each i^th item pair (start, end) from node_starts and node_ends
  // contains the start and end instruction IDs for the same node.
  // Beacuse each end is generated according to each start one by one.
  // More details are in function gen_node_ends.
  for (; s != node_starts.end(); s++, e++) {
    node nd(*s, node_ends[e]);
    gnodes.push_back(nd);
  }
}

// key: node._start (the start instruction ID) -> value: node_id in gnodes
void graph::gen_id_map(unsigned_map& id_map, vector<node>& gnodes) {
  for (size_t i = 0; i < gnodes.size(); i++) {
    id_map[gnodes[i]._start] = i;
  }
}

void graph::gen_all_edges_graph(vector<vector<unsigned int> >& gnodes_out, vector<node>& gnodes, inst* inst_lst) {
  for (size_t i = 0; i < gnodes.size(); i++) {
    gnodes_out.push_back(vector<unsigned int> {});
  }

  unsigned_map inid_2_gndid;
  gen_id_map(inid_2_gndid, gnodes);

  for (size_t i = 0; i < gnodes.size(); i++) {
    size_t end_inst_id = gnodes[i]._end;
    vector <unsigned int> next_inst_ids;
    int inst_type = opcode_type[inst_lst[end_inst_id]._opcode];
    if (inst_type == OP_OTHERS || inst_type == OP_NOP) {
      next_inst_ids.push_back(end_inst_id + 1);
    } else if (inst_type == OP_JMP) {
      // keep order: insert no jmp first
      next_inst_ids.push_back(end_inst_id + 1); //no jmp
      next_inst_ids.push_back(end_inst_id + 1 + inst_lst[end_inst_id]._args[2]); //jmp
    }

    for (size_t j = 0; j < next_inst_ids.size(); j++) {
      unsigned_map::iterator it = inid_2_gndid.find(next_inst_ids[j]);
      if (it != inid_2_gndid.end()) {
        gnodes_out[i].push_back(it->second);
      }
    }
  }
}

// if next node has not been added into the graph, add this node
void graph::add_node(node& nd, unsigned int& added) {
  if (added == -1) {
    nodes.push_back(nd);
    nodes_in.push_back(vector<unsigned int> {});
    nodes_out.push_back(vector<unsigned int> {});
    added = nodes.size() - 1;
  }
}


void graph::dfs(size_t cur_gnode_id, vector<node>& gnodes, vector<vector<unsigned int> >& gnodes_out, \
                vector<unsigned int>& added, vector<bool>& visited, vector<bool>& finished) {
  if (finished[cur_gnode_id]) {
    return;
  }

  // 1 set cur_node_id as visited
  visited[cur_gnode_id] = true;

  // 2 for each next node, check loop.
  // Whenever visited[i] && !finished[i] is true, i refers to a node that is
  // a DFS ancestor of the current node which is also reachable from the current node.
  for (size_t i = 0; i < gnodes_out[cur_gnode_id].size(); i++) {
    unsigned int next_gnode_id = gnodes_out[cur_gnode_id][i];
    if (visited[next_gnode_id] && (!finished[next_gnode_id])) {
      string err_msg  = "illegal input: loop from node " + \
                        to_string(cur_gnode_id) + gnodes[cur_gnode_id].to_str() + \
                        " to node " + \
                        to_string(next_gnode_id) + gnodes[next_gnode_id].to_str();
      throw (err_msg);
    }
  }

  // 3 for each next node, update the graph G'
  unsigned int cur_node_id = added[cur_gnode_id];
  for (size_t i = 0; i < gnodes_out[cur_gnode_id].size(); i++) {
    unsigned int next_gnode_id = gnodes_out[cur_gnode_id][i];
    // if next node has not been added into the graph, add this node
    add_node(gnodes[next_gnode_id], added[next_gnode_id]);
    // add edge into the graph
    unsigned int next_node_id = added[next_gnode_id];
    nodes_in[next_node_id].push_back(cur_node_id);
    nodes_out[cur_node_id].push_back(next_node_id);
    // dfs nextNode
    try {
      dfs(next_gnode_id, gnodes, gnodes_out, added, visited, finished);
    } catch (const string err_msg) {
      throw err_msg;
    }
  }

  // 4 set cur_node_id finished
  finished[cur_gnode_id] = true;
}

void graph::gen_graph(inst* inst_lst, int length) {
  init();
  // 1 generate node starts
  // set: keep order and ensure no repeated items
  set<size_t> node_starts;
  try {
    gen_node_starts(inst_lst, length, node_starts);
  } catch (const string err_msg) {
    throw err_msg;
  }
  // 2 generate node ends for each start in node_starts
  vector<size_t> node_ends;
  gen_node_ends(inst_lst, length, node_starts, node_ends);
  // 3 generate graph G containg all nodes and all edges
  vector<node> gnodes;
  vector<vector<unsigned int> > gnodes_out;
  // 3.1 add all nodes in G, that is gnodes
  gen_all_nodes_graph(gnodes, node_starts, node_ends);
  // 3.2 add all edges in G, that is gnodes_out;
  gen_all_edges_graph(gnodes_out, gnodes, inst_lst);

  // 4. generate connected sub-graph G' containing the first instruction from G by DFS
  // added[i] means the gnodes[i] has been added into the graph, and nodesId = added[gnodesId]
  vector<unsigned int> added(gnodes.size(), -1);
  // visited[i] = true means the node has been visited
  // used to check loop. loop exists if the visited[i] && !finished[i]
  vector<bool> visited(gnodes.size(), 0);
  // finished[i] = true means the node in nodesLst has finished DFS of gnodes[i]
  vector<bool> finished(gnodes.size(), 0);
  // add the root into the G'
  add_node(gnodes[0], added[0]);
  try {
    dfs(0, gnodes, gnodes_out, added, visited, finished);
  } catch (const string err_msg) {
    throw err_msg;
  }
}

graph::graph(inst* inst_lst, int length) {
  try {
    gen_graph(inst_lst, length);
  } catch (const string err_msg) {
    throw err_msg;
  }
}

graph::graph() {}

graph::~graph() {}

ostream& operator<<(ostream& out, const graph& g) {
  out << endl << "nodes:" << endl << " ";
  for (size_t i = 0; i < g.nodes.size(); i++) {
    out << i << g.nodes[i] << " ";
  }
  out << endl << "edges:" << endl;
  for (size_t i = 0; i < g.nodes.size(); i++) {
    out << " " << i << " in:";
    for (size_t j = 0; j < g.nodes_in[i].size(); j++) {
      out << g.nodes_in[i][j] << " ";
    }
    out << " out:";
    for (size_t j = 0; j < g.nodes_out[i].size(); j++) {
      out << g.nodes_out[i][j] << " ";
    }
    out << endl;
  }
  return out;
}
