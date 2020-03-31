#include "inst_codegen.h"

using namespace std;

z3::expr latest_write_addr(int idx, smt_wt& x);
z3::expr addr_not_in_wt(z3::expr& addr, smt_wt& x);

void predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_mem& m) {
  m._mem_table._wt.add(addr + off, in.extract(7, 0));
}

inline z3::expr addr_in_range(z3::expr addr, z3::expr start, z3::expr end) {
  return (uge(addr, start) && uge(end, addr));
}

z3::expr urt_element_constrain(z3::expr a, z3::expr v, mem_wt& x) {
  z3::expr f = string_to_expr("true");
  // add constrains on the new symbolic value "v" according to the following cases:
  // case 1: "a" can be found in x._wt(addr1).
  // if addr1 is the latest write address in x._wt and a is equal to addr1,
  // it implies v is equal to the value of addr1
  // The constrains of this case can be removed, since if the addr exists in the memory WT,
  // the value in that address is read from WT but not URT.

  // case 2: "a" cannot be found in x._wt, but x._urt(addr1).
  // if there is no address equal to a in x._wt and addr1 in x._urt is equal to
  // a, it implies v is equal to the value of addr1
  z3::expr f2 = string_to_expr("true");
  z3::expr f3 = addr_not_in_wt(a, x._wt);
  // "-2" is to skip the latest add just now
  for (int i = x._urt.addr.size() - 2; i >= 0; i--) {
    f2 = f2 && z3::implies(f3 && (a == x._urt.addr[i]), v == x._urt.val[i]);
  }
  f = f && f2;
  // case 3: "a" cannot be found in x._wt or x._urt.
  // there is no constrains on the new symbolic value "v"
  return f;
}

z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_mem& m, z3::expr out, mem_info& m_info) {
  smt_wt *s = &m._mem_table._wt;
  z3::expr a = addr + off;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s->addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s->addr[i]), out == s->val[i]);
    c = c || (a == s->addr[i]);
  }
  // add element in urt
  m._mem_table._urt.add(a, out);
  // add constrains on the element(a, out)
  f = f && urt_element_constrain(a, out, m._mem_table);

  // safety check
  // address "a" within the memory range that does not allow ur
  // if "a" cannot be found in memory WT, the result is false
  f = f && z3::implies(addr_in_range(a, m_info._stack.start, m_info._stack.end) &&
                       addr_not_in_wt(a, m._mem_table._wt),
                       string_to_expr("false"));
  return f;
}

// return the FOL formula that x.addr[idx] is the latest addr in x
// that is, for any i > idx, x.addr[idx] != x.addr[i]
z3::expr latest_write_addr(int idx, smt_wt& x) {
  z3::expr f = string_to_expr("true");
  for (int i = x.addr.size() - 1; i > idx; i--) {
    f = f && (x.addr[idx] != x.addr[i]);
  }
  return f;
}

// return the FOL formula that addr cannot be found in x
// that is, for any addr_x in x, addr != addr_x
z3::expr addr_not_in_wt(z3::expr& addr, smt_wt& x) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < x.addr.size(); i++) {
    f = f && (addr != x.addr[i]);
  }
  return f;
}

z3::expr stack_addr_in_one_wt(smt_wt& x, smt_wt& y, mem_range& r) {
  z3::expr f = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(addr_in_range(x.addr[i], r.start, r.end) &&
                         addr_not_in_wt(x.addr[i], y),
                         false);
  }
  return f;
}

// need memory WTs, stack memory range
z3::expr smt_stack_eq_chk(smt_wt& x, smt_wt& y, mem_range& r) {
  z3::expr f = string_to_expr("true");
  // case 1: addrs can be found in both WTs
  z3::expr f_addr_x = string_to_expr("true");
  z3::expr f_addr_y = string_to_expr("true");
  for (int i = x.addr.size() - 1; i >= 0; i--) {
    f_addr_x = addr_in_range(x.addr[i], r.start, r.end) &&
               latest_write_addr(i, x);
    for (int j = y.addr.size() - 1; j >= 0; j--) {
      f_addr_y = addr_in_range(y.addr[j], r.start, r.end) &&
                 latest_write_addr(j, y);
      f = f && z3::implies(f_addr_x && f_addr_y && (x.addr[i] == y.addr[j]),
                           x.val[i] == y.val[j]);
    }
  }
  // case 2: addrs can only be found in one WT.
  // If these addrs are found, the result is false.
  f = f && stack_addr_in_one_wt(x, y, r) && stack_addr_in_one_wt(y, x, r);
  return f;
}

// TODO: in order to test, stack is checked
// need to modify later, since stack does not need to be checked
z3::expr smt_mem_eq_chk(smt_mem& x, smt_mem& y, mem_info& m_info) {
  return smt_stack_eq_chk(x._mem_table._wt, y._mem_table._wt, m_info._stack);
}

z3::expr pgm_smt_mem_eq_chk(vector<z3::expr>& pc1, vector<smt_mem>& mem1,
                            vector<z3::expr>& pc2, vector<smt_mem>& mem2,
                            mem_info& m_info) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < pc1.size(); i++) {
    for (int j = 0; j < pc2.size(); j++) {
      f = f && z3::implies((pc1[i] && pc2[j]), smt_mem_eq_chk(mem1[i], mem2[j], m_info));
    }
  }
  return f;
}
