#include "inst_var.h"

void test1() {
  cout << "Test 1: check class dag functions" << endl;
  cout << "check is_b_on_root2a_path()" << endl;
  unsigned int n_nodes = 6, root = 0;
  dag g1(n_nodes, root);
  vector<vector<unsigned int>> out_edges = {{1, 2}, {3}, {3, 5}, {4}, {}, {}};
  for (int i = 0; i < out_edges.size(); i++) {
    for (int j = 0; j < out_edges[i].size(); j++) {
      g1.add_edge_a2b(i, out_edges[i][j]);
    }
  }
  int test_count = 0;
  // check root must be on the path of root to each node
  for (int i = 0; i < n_nodes; i++) {
    print_test_res(g1.is_b_on_root2a_path(i, root) == INT_true, to_string(++test_count));
  }
  for (int i = 1; i < n_nodes; i++) {
    print_test_res(g1.is_b_on_root2a_path(root, i) == INT_false, to_string(++test_count));
  }
  print_test_res(g1.is_b_on_root2a_path(3, 1) == INT_uncertain, to_string(++test_count));
  print_test_res(g1.is_b_on_root2a_path(3, 2) == INT_uncertain, to_string(++test_count));
  print_test_res(g1.is_b_on_root2a_path(3, 5) == INT_false, to_string(++test_count));
  print_test_res(g1.is_b_on_root2a_path(5, 2) == INT_true, to_string(++test_count));
}

int main() {
  test1();
  return 0;
}
