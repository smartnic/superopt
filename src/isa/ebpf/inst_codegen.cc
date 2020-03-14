#include "inst_codegen.h"

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_stack& s, z3::expr out) {
  int i = s.addr.size() - 1;
  z3::expr a = addr + off;
  z3::expr c = (a == s.addr[i]);
  z3::expr f = z3::implies(c, out == s.val[i]);
  for (i = i - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s.addr[i]), out == s.val[i]);
    c = c || (a == s.addr[i]);
  }
  return f;
}
