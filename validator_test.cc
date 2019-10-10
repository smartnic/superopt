#include <iostream>
#include "validator.h"
#include "inst.h"

using namespace std;

// instructions1 == instructions2 == instructions3 != instructions4
inst instructions1[5] = {inst(MOVXC, 2, 4),     /* mov r2, 4  */
                         inst(ADDXY, 1, 2),     /* add r1, r2 */
                         inst(MOVXC, 3, 15),    /* mov r3, 15 */
                         inst(MAXC, 1, 15),     /* max r1, 15 */
                         inst(MAXX, 1, 2),      /* max r1, r2 */
                        };

inst instructions2[6] = {inst(MOVXC, 2, 4),     /* mov r2, 4  */
                         inst(MOVXC, 3, 10),    /* mov r3, 10 */
                         inst(ADDXY, 1, 2),     /* add r1, r2 */
                         inst(MOVXC, 3, 15),    /* mov r3, 15 */
                         inst(MAXC, 1, 15),     /* max r1, 15 */
                         inst(MAXX, 1, 2),      /* max r1, r2 */
                        };

inst instructions3[5] = {inst(MOVXC, 2, 4),     /* mov r2, 4  */
                         inst(ADDXY, 1, 2),     /* add r1, r2 */
                         inst(MOVXC, 3, 15),    /* mov r3, 15 */
                         inst(MAXC, 1, 15),     /* max r1, 15 */
                         inst(MAXX, 1, 3),      /* max r1, r3 */
                        };

inst instructions4[5] = {inst(MOVXC, 2, 4),     /* mov r2, 4  */
                         inst(ADDXY, 1, 2),     /* add r1, r2 */
                         inst(MOVXC, 3, 15),    /* mov r3, 15 */
                         inst(MAXC, 1, 15),     /* max r1, 15 */
                         inst(MAXX, 1, 4),      /* max r1, r4 */
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
