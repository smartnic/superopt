#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <string.h>
#include "z3++.h"
#include <math.h>

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES (1 << 22)
#define RESULT_SIZE_BYTES (1 << 22)

using namespace std;

z3::context c;
int read_problem_from_z3client(int PORT);

char buffer[FORMULA_SIZE_BYTES + 1] = {0};
char res_buffer[RESULT_SIZE_BYTES + 1]  = {0};

string run_solver(char* formula) {
  z3::tactic t = z3::tactic(c, "bv");
  z3::solver s = t.mk_solver();

  Z3_set_ast_print_mode(s.ctx(), Z3_PRINT_SMTLIB2_COMPLIANT);
  string res;
  s.from_string(formula);
  // cout << "Running the solver..." << endl;
  switch (s.check()) {
    case z3::unsat: {
      return "unsat";
    }
    case z3::sat: {
      ostringstream strm;
      z3::model mdl = s.get_model();
      strm << mdl;
      res = strm.str();
      return res;
    }
    case z3::unknown: {
      return "unknown";
    }
  }
}
void set_seed(){
  srand (time(NULL));
  int iSecret = rand() % ((int) pow(2,8)) + 1;
  // iSecret = rand() % 4 + 1;
  cout << "z3server: seed = " << iSecret << endl;
  z3::set_param("sls.random_seed", iSecret);
  z3::set_param("smt.random_seed", iSecret);
  z3::set_param("sat.random_seed", iSecret);
  z3::set_param("fp.spacer.random_seed", iSecret);
}

int read_problem_from_z3client(int PORT) {
  int server_fd, acc_socket, nread, total_read, nchars;
  int opt = 1;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  string result;
  set_seed();
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("z3server: socket creation failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT,
                 (char*)&opt, sizeof(opt)) < 0) {
    perror("z3server: setsockopt to reuse addr/port failed");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (::bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
    perror("z3server: socket bind to local address/port failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 1) < 0) {
    perror("z3server: can't listen to bound socket");
    exit(EXIT_FAILURE);
  }

  /* Main server + solver loop */
  while ((acc_socket = accept(server_fd, (struct sockaddr *)&address,
                              (socklen_t*)&addrlen)) ) {
    if (acc_socket < 0) {
      perror("z3server: failed to accept incoming connection");
      exit(EXIT_FAILURE);
    }

    //cout << "z3server: Received a new connection. Reading formula on port: " << PORT << endl;
    /* Read the full formula into buffer. */
    total_read = 0;
    do {
      nread = read(acc_socket, buffer + total_read, FORMULA_SIZE_BYTES - total_read);
      total_read += nread;
    } while (buffer[total_read - 1] != '\0' &&
             total_read < FORMULA_SIZE_BYTES);
    if (total_read >= FORMULA_SIZE_BYTES)
      cout << "Exhausted formula read buffer\n";
    
    //cout << "z3server: Recieved Formula from client on port: " << PORT << endl;

    /* Run the solver. */
    result = run_solver(buffer);
    nchars = min((int)result.length(), RESULT_SIZE_BYTES);
    strncpy(res_buffer, result.c_str(), nchars);
    res_buffer[nchars] = '\0';

    //cout << "z3server: Sending formula to Client...\n";
    /* Send result. */
    send(acc_socket, res_buffer, nchars + 1, 0);
    close(acc_socket);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2)
  {
    cout << "No port argument" << endl;
    return 1;
  }
  int PORT = std::stoi(argv[1]);
  cout << "z3server: Port is  " << argv[1] << endl;
  /* Receive a z3 smtlib2 formula in a shared memory segment, and
     return sat or unsat in another one. */
  read_problem_from_z3client(PORT);
  return 0;
}
