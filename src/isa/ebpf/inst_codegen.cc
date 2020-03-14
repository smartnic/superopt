#include "inst_codegen.h"

// Assume the input is safe, eg. addr+off can be found in s.addr
z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_stack& s, z3::expr out) {
  z3::expr a = addr + off;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s.addr[i]), out == s.val[i]);
    c = c || (a == s.addr[i]);
  }
  return f;
}
