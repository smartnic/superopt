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
z3::expr addrs_in_one_wt_not_allow_uw(mem_wt& x, mem_wt& y) {
  z3::expr f = string_to_expr("true");
  for (int i = x._wt.addr.size() - 1; i >= 0; i--) {
    z3::expr f1 = latest_write_addr(i, x._wt);
    f1 = f1 && addr_not_in_wt(x._wt.addr[i], y._wt);
    f = f && z3::implies(f1, false);
  }
  return f;
}

// case: addrs in x._wt but not in y._wt and allow read before write
// return the FOL formula that if these addrs are found, they should be
// found in the x._uwt and the values corresponding to the same addrs found in
// the x._wt and x._uwt should be the same.
z3::expr addrs_in_one_wt_allow_uw(mem_wt& x, mem_wt& y) {
  z3::expr f = string_to_expr("true");
  for (int i = x._wt.addr.size() - 1; i >= 0; i--) {
    z3::expr addr = x._wt.addr[i];
    z3::expr val = x._wt.val[i];
    // f1: FOL formula that addr(latest_write_addr) \in x._wt.addr, \notin y.wt.addr
    z3::expr f1 = latest_write_addr(i, x._wt) && addr_not_in_wt(addr, y._wt);
    // f2: FOL formula that addr must in x._uwt
    z3::expr f2 = !(addr_not_in_wt(addr, x._uwt)); // addr in x._uwt
    // f3: FOL formula that if addr is found in x._uwt (searched from the latest),
    // the values are equal
    z3::expr f3 = string_to_expr("true");
    for (int j = x._uwt.addr.size() - 1; j >= 0; j--) {
      f3 = f3 && z3::implies(latest_write_addr(j, x._uwt) && (addr == x._uwt.addr[j]),
                             val == x._uwt.val[j]);
    }
    f = f && z3::implies(f1, f2 && f3);
  }
  return f;
}

// make sure the elements in unintialized write table are unique,
// i.e., for two elements <a1, v1> and <a2, v2> in write table,
// a1 == a2 => v1 == v2
z3::expr property_of_uwt(smt_wt& x) {
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
  if ((! x._allow_uw) || (! y._allow_uw)) {
    f = f && addrs_in_one_wt_not_allow_uw(x, y)
        && addrs_in_one_wt_not_allow_uw(y, x);
    return f;
  }
  // case2.2 addr(latest_write_addr) \in x._wt.addr, \notin y._wt.addr and
  // allow read before write
  f = f && (addrs_in_one_wt_allow_uw(x, y) && addrs_in_one_wt_allow_uw(y, x));
  // add the property of unintialized write table
  f = z3::implies(property_of_uwt(x._uwt) && property_of_uwt(y._uwt), f);
  return f;
}
