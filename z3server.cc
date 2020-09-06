#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include "z3++.h"

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES 65536
#define RESULT_SIZE_BYTES  4096

#define PORT 8001

using namespace std;

z3::context c;
int read_problem_from_z3client();

string run_solver(char* formula) {
  z3::solver s(c);
  s.from_string(formula);
  cout << "Checking... result:\n";
  switch (s.check()) {
    case z3::unsat: {
      cout <<  "unsat\n";
      return "unsat";
    }
    case z3::sat: {
      /* TODO: extract a model and pass that as the result instead */
      cout << "sat\n";
      return "sat";
    }
    case z3::unknown: {
      cout << "unknown\n";
      return "unknown";
    }
  }
}

int read_problem_from_z3client() {
  int server_fd, acc_socket, nread, total_read, nchars;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[FORMULA_SIZE_BYTES+1] = {0};
  char res_buffer[RESULT_SIZE_BYTES+1]  = {0};
  string result;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("z3server: socket creation failed");
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

    /* Read the full formula into buffer. */
    total_read = 0;
    do {
      nread = read(acc_socket, buffer, FORMULA_SIZE_BYTES - total_read);
      total_read += nread;
    } while (buffer[total_read] != '\0');

    cout << "Formula from client:\n";
    cout << buffer << endl;

    /* Run the solver. */
    result = run_solver(buffer);
    nchars = min((int)result.length(), RESULT_SIZE_BYTES);
    strncpy(res_buffer, result.c_str(), nchars);
    res_buffer[nchars] = '\0';

    /* Send result. */
    send(acc_socket, res_buffer, strlen(res_buffer), 0);
    cout << "Result sent from server -> client: " << res_buffer << "\n";
  }
  return 0;
}

int main() {
  /* Receive a z3 smtlib2 formula in a shared memory segment, and
     return sat or unsat in another one. */
  read_problem_from_z3client();
  return 0;
}
