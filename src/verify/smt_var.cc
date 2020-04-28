#include <string>
#include "smt_var.h"

using namespace std;

z3::context smt_c;

z3::expr string_to_expr(string s) {
  if (s == "true") {
    return smt_c.bool_val(true);
  } else if (s == "false") {
    return smt_c.bool_val(false);
  }
  return smt_c.bv_const(s.c_str(), NUM_REG_BITS);
}

z3::expr to_bool_expr(string s) {
  return smt_c.bool_const(s.c_str());
}

z3::expr to_expr(int64_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(uint64_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(int32_t x, unsigned sz) {
  return smt_c.bv_val(x, sz);
}

z3::expr to_expr(string s, unsigned sz) {
  return smt_c.bv_const(s.c_str(), sz);
}
