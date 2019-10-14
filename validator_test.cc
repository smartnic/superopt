#include <iostream>
#include "validator.h"
#include "inst.h"

using namespace std;

// instructions1 == instructions2 == instructions3 != instructions4
inst instructions1[5] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                         inst(ADDXY, 0, 1),     /* add r0, r1 */
                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(MAXX, 0, 1),      /* max r0, r1 */
                        };

inst instructions2[6] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                         inst(MOVXC, 2, 10),    /* mov r2, 10 */
                         inst(ADDXY, 0, 1),     /* add r0, r2 */
                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(MAXX, 0, 1),      /* max r0, r1 */
                        };

inst instructions3[5] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                         inst(ADDXY, 0, 1),     /* add r0, r1 */
                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(MAXX, 0, 2),      /* max r0, r2 */
                        };

inst instructions4[5] = {inst(MOVXC, 1, 4),     /* mov r1, 4  */
                         inst(ADDXY, 0, 1),     /* add r0, r1 */
                         inst(MOVXC, 2, 15),    /* mov r2, 15 */
                         inst(MAXC, 0, 15),     /* max r0, 15 */
                         inst(MAXX, 0, 3),      /* max r0, r3 */
                        };

void test1() {
	cout << "Check (instructions1 == instructions2) == true";
	if (isEqualProg(instructions1, 5, instructions2, 6)) {
		cout << " SUCCESS." << endl;
	}
	else {
		cout << " NOT SUCCESS." << endl;
	}
	cout << endl;

	cout << "Check (instructions1 == instructions3) == true";
	if (isEqualProg(instructions1, 5, instructions3, 5)) {
		cout << " SUCCESS." << endl;
	}
	else {
		cout << " NOT SUCCESS." << endl;
	}
	cout << endl;

	cout << "Check (instructions1 == instructions4) == false";
	if (!isEqualProg(instructions1, 5, instructions4, 5)) {
		cout << " SUCCESS." << endl;
	}
	else {
		cout << " NOT SUCCESS." << endl;
	}
	cout << endl;
}

int main(int argc, char *argv[]) {
	test1();
	return 0;
}
