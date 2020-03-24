#include "inst_codegen.h"

using namespace std;

void predicate_st_byte(z3::expr in, z3::expr addr, z3::expr off, smt_mem& m) {
  m._stack._wt.add(addr + off, in.extract(7, 0));
}

// Assume the input is safe, eg. addr+off can be found in s.addr
z3::expr predicate_ld_byte(z3::expr addr, z3::expr off, smt_mem& m, z3::expr out) {
  smt_wt *s = &m._stack._wt;
  z3::expr a = addr + off;
  z3::expr c = string_to_expr("false");
  z3::expr f = string_to_expr("true");
  for (int i = s->addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies((!c) && (a == s->addr[i]), out == s->val[i]);
    c = c || (a == s->addr[i]);
  }
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

// case: addrs in x._wt and y._wt
// return the FOL formula that if those addrs are found and
// they are the latest write of those addrs in both x._wt and y._wt,
// the corresponding values in both both x._wt and y._wt should be the same.
z3::expr addrs_in_both_wts(mem_wt& x, mem_wt& y) {
  z3::expr f = string_to_expr("true");
  for (int i = x._wt.addr.size() - 1; i >= 0; i--) {
    for (int j = y._wt.addr.size() - 1; j >= 0; j--) {
      f = f && z3::implies(latest_write_addr(i, x._wt) &&
                           latest_write_addr(j, y._wt) &&
                           (x._wt.addr[i] == y._wt.addr[j]),
                           x._wt.val[i] == y._wt.val[j]);
    }
  }
  return f;
}

// case: addrs in x._wt but not in y._wt and not allow read before write
// return the FOL formula that if these addrs are found, the result is false
z3::expr addrs_in_one_wt_not_allow_ur(mem_wt& x, mem_wt& y) {
  z3::expr f = string_to_expr("true");
  for (int i = x._wt.addr.size() - 1; i >= 0; i--) {
    f = f && z3::implies(addr_not_in_wt(x._wt.addr[i], y._wt), false);
  }
  return f;
}

// case: addrs in x._wt but not in y._wt and allow read before write
// return the FOL formula that if these addrs are found, they should be
// found in the x._urt and the values corresponding to the same addrs found in
// the x._wt and x._urt should be the same.
z3::expr addrs_in_one_wt_allow_ur(mem_wt& x, mem_wt& y) {
  z3::expr f = string_to_expr("true");
  for (int i = x._wt.addr.size() - 1; i >= 0; i--) {
    z3::expr addr = x._wt.addr[i];
    z3::expr val = x._wt.val[i];
    // f1: FOL formula that addr(latest_write_addr) \in x._wt.addr, \notin y.wt.addr
    z3::expr f1 = latest_write_addr(i, x._wt) && addr_not_in_wt(addr, y._wt);
    // f2: FOL formula that addr must in x._urt
    z3::expr f2 = !(addr_not_in_wt(addr, x._urt)); // addr in x._urt
    // f3: FOL formula that if addr is found in x._urt, the values are equal
    z3::expr f3 = string_to_expr("true");
    for (int j = x._urt.addr.size() - 1; j >= 0; j--) {
      f3 = f3 && z3::implies(addr == x._urt.addr[j], val == x._urt.val[j]);
    }
    f = f && z3::implies(f1, f2 && f3);
  }
  return f;
}

// make sure the elements in unintialized read table are unique,
// i.e., for two elements <a1, v1> and <a2, v2> in write table,
// a1 == a2 => v1 == v2
// TODO: may need add the logic into read
z3::expr property_of_urt(smt_wt& x) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < x.addr.size(); i++) {
    for (int j = x.addr.size() - 1; j > i; j--) {
      f = f && z3::implies(x.addr[i] == x.addr[j], x.val[i] == x.val[j]);
    }
  }
  return f;
}

z3::expr predicate_mem_eq_chk(mem_wt& x, mem_wt& y) {
  // case1 addr(latest_write_addr) \in x._wt.addr, \in y._wt.addr
  z3::expr f = addrs_in_both_wts(x, y);
  // case 2.1 addr(latest_write_addr) \in x._wt.addr, \notin y._wt.addr and
  // do not allow read before write
  if ((! x._allow_ur) || (! y._allow_ur)) {
    f = f && addrs_in_one_wt_not_allow_ur(x, y)
        && addrs_in_one_wt_not_allow_ur(y, x);
    return f;
  }
  // case2.2 addr(latest_write_addr) \in x._wt.addr, \notin y._wt.addr and
  // allow read before write
  f = f && (addrs_in_one_wt_allow_ur(x, y) && addrs_in_one_wt_allow_ur(y, x));
  // add the property of unintialized read table
  f = z3::implies(property_of_urt(x._urt) && property_of_urt(y._urt), f);
  return f;
}

// TODO: in order to test, stack is checked
// need to modify later, since stack does not need to be checked
z3::expr smt_mem_eq_chk(smt_mem& x, smt_mem& y) {
  return predicate_mem_eq_chk(x._stack, y._stack);
}

z3::expr pgm_smt_mem_eq_chk(vector<z3::expr>& pc1, vector<smt_mem>& mem1,
                            vector<z3::expr>& pc2, vector<smt_mem>& mem2) {
  z3::expr f = string_to_expr("true");
  for (int i = 0; i < pc1.size(); i++) {
    for (int j = 0; j < pc2.size(); j++) {
      f = f && z3::implies((pc1[i] && pc2[j]), smt_mem_eq_chk(mem1[i], mem2[j]));
    }
  }
  return f;
}
