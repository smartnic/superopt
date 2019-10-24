#include <iostream>
#include "validator.h"
#include "inst.h"

using namespace z3;

void test1() {
	validator vld;
	std::cout << "\nno branch program equivalence check starts...\n\n";
	// instructions1 == instructions2 == instructions3 != instructions4
	inst instructions1[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
	                         inst(ADDXY, 0, 1),     /* add r0, r1 */
	                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
	                         inst(MAXC, 0, 15),     /* max r0, 15 */
	                         inst(MAXX, 0, 1),      /* max r0, r1 */
	                         inst(RETX, 0),
	                        };

	inst instructions2[7] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
	                         inst(MOVXC, 2, 10),    /* mov r2, 10 */
	                         inst(ADDXY, 0, 1),     /* add r0, r1 */
	                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
	                         inst(MAXC, 0, 15),     /* max r0, 15 */
	                         inst(MAXX, 0, 1),      /* max r0, r1 */
	                         inst(RETX, 0),
	                        };

	inst instructions3[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
	                         inst(ADDXY, 0, 1),     /* add r0, r1 */
	                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
	                         inst(MAXC, 0, 15),     /* max r0, 15 */
	                         inst(MAXX, 0, 2),      /* max r0, r2 */
	                         inst(RETX, 0),
	                        };

	inst instructions4[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
	                         inst(ADDXY, 0, 1),     /* add r0, r1 */
	                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
	                         inst(MAXC, 0, -1),     /* max r0, 15 */
	                         inst(MAXX, 0, 3),      /* max r0, r3 */
	                         inst(RETX, 0),
	                        };
	if (vld.equalCheck(instructions1, 6, instructions2, 7)) {
		std::cout << "check instructions1 == instructions2 SUCCESS\n";
	}
	else {
		std::cout << "check instructions1 == instructions2 NOT SUCCESS\n";
	}
	if (vld.equalCheck(instructions1, 6, instructions3, 6)) {
		std::cout << "check instructions1 == instructions3 SUCCESS\n";
	}
	else {
		std::cout << "check instructions1 == instructions3 NOT SUCCESS\n";
	}
	if (!vld.equalCheck(instructions1, 6, instructions4, 6)) {
		std::cout << "check instructions1 != instructions4 SUCCESS\n";
	}
	else {
		std::cout << "check instructions1 != instructions4 NOT SUCCESS\n";
	}
	std::cout << "\nno branch program equivalence check ends...\n\n";
}


void test2() {
	validator vld;
	std::cout << "\nbranch program equivalence check starts...\n\n";
	// instructions1 == instructions2
	inst instructions1[3] = {inst(JMPGT, 0, 2, 1),  // if r0 <= r2:
	                         inst(RETX, 0),         // ret r0
	                         inst(RETX, 2),     // ret r2;
	                        };
	inst instructions2[3] = {inst(JMPLT, 0, 2, 1),  // if r0 >= r2
	                         inst(RETX, 2),         // ret r2
	                         inst(RETX, 0),         // ret r0
	                        };
	if (vld.equalCheck(instructions1, 3, instructions2, 3)) {
		std::cout << "check instructions1 == instructions2 SUCCESS\n";
	}
	else {
		std::cout << "check instructions1 == instructions2 NOT SUCCESS\n";
	}

	// instructions3 == instructions4 != instructions5
	inst instructions3[3] = {inst(JMPGT, 0, 2, 1),  // return max(r0, r2)
	                         inst(RETX, 2),
	                         inst(RETX, 0),
	                        };
	inst instructions4[2] = {inst(MAXX, 0, 2),  // return r0=max(r0, r2)
	                         inst(RETX, 0),
	                        };
	inst instructions5[3] = {inst(JMPGT, 2, 0, 1),  // return min(r0, r2)
	                         inst(RETX, 2),
	                         inst(RETX, 0),
	                        };
	if (vld.equalCheck(instructions3, 3, instructions4, 2)) {
		std::cout << "check instructions3 == instructions4 SUCCESS\n";
	}
	else {
		std::cout << "check instructions3 == instructions4 NOT SUCCESS\n";
	}

	if (!vld.equalCheck(instructions3, 3, instructions5, 3)) {
		std::cout << "check instructions3 != instructions5 SUCCESS\n";
	}
	else {
		std::cout << "check instructions3 != instructions5 NOT SUCCESS\n";
	}

	// std::cout << "\ncheck f(x) = max(x, r1, r2, 10)\n";
	// f(x) = max(x, r1, r2, 10)
	// p11 == p12
	inst p11[5] = {inst(MAXX, 0, 1),
	               inst(MAXX, 0, 2),
	               inst(MOVXC, 1, 10),
	               inst(MAXX, 0, 1),
	               inst(RETX, 0),
	              };
	inst p12[11] = {inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
	                inst(MOVXC, 0, 0),
	                inst(ADDXY, 0, 1),
	                inst(JMPGT, 0, 2, 2), // skip r0 <- r2, if r0 > r2
	                inst(MOVXC, 0, 0),
	                inst(ADDXY, 0, 2),
	                inst(MOVXC, 1, 10),   // r1 <- 10
	                inst(JMPGT, 0, 1, 2), // skip r0 <- r1, if r0 > r1
	                inst(MOVXC, 0, 0),
	                inst(ADDXY, 0, 1),
	                inst(RETX, 0),        // ret r0
	               };
	if (vld.equalCheck(p11, 5, p12, 11)) {
		std::cout << "check f(x)_p1 == f(x)_p2 SUCCESS\n";
	}
	else {
		std::cout << "check f(x)_p1 == f(x)_p2 NOT SUCCESS\n";
	}
	std::cout << "\nbranch program equivalence check ends...\n\n";
}

//
void test3() {
	validator vld;
	expr x = stringToExpr("x");
	expr y = stringToExpr("y");
	expr fx = implies(x > 10, y == x) && implies(x <= 10, y == 10);
	inst pFx[4] = {inst(MOVXC, 1, 10),
	               inst(JMPLT, 0, 1, 1),
	               inst(RETX, 0),
	               inst(RETX, 1),
	              };
	if (vld.equalCheck(pFx, 4, fx, x, y)) {
		std::cout << "check Program_f(x) == (f(x)=max(x, 10)) SUCCESS\n";
	}
	else {
		std::cout << "check Program_f(x) == (f(x)=max(x, 10)) NOT SUCCESS\n";
	}
}

int main(int argc, char *argv[]) {
	test1(); // no branch
	test2(); // with branch
	test3();
	return 0;
}
