#pragma once

#include "z3++.h"
using namespace std;
extern int SERVER_PORT;
int spawn_server();
string write_problem_to_z3server(string formula);
