#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include "z3client.h"
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES (1 << 22)
#define RESULT_SIZE_BYTES (1 << 22)
#define SOLVER_RESPAWN_THRESOLD 1000


int SERVER_PORT = 8002; /* default port */ 
z3::context c;
pid_t child_pid = -1;
pid_t pid;
int nsolve = 0;

char form_buffer[FORMULA_SIZE_BYTES + 1] = {0};
char res_buffer[RESULT_SIZE_BYTES + 1] = {0};

int spawn_server() {
  pid = fork();
  if (pid == -1) {
    cout << "Fork error occurred. Can't spawn a z3 solver server.";
    return -1;
  } else if (pid == 0) { /* in the child process; exec to z3server */
    std::string NEWPORT = std::to_string(SERVER_PORT);
    char *argv_list[] = {(char *)"./z3server.out ", const_cast<char *>(NEWPORT.c_str()), (char *)NULL};
    execv("./z3server.out", argv_list);
    exit(-1); /* never supposed to get here until the exec fails. */
  } else {
    /* in the parent process; record and return the child pid for later. */
    return pid;
  }
}

string write_problem_to_z3server(string formula) {
  int sock = 0, nread, nchars, total_read;
  struct sockaddr_in serv_addr;

  // cout << "z3client: Received a formula to solve\n";

  /* check if server process exists currently; if not, spawn it. */
  bool no_child_now = child_pid <= 0;
  bool time_to_respawn = (! no_child_now) &&
                         nsolve > 0 && nsolve % SOLVER_RESPAWN_THRESOLD == 0;
  if (no_child_now || time_to_respawn) {
    if (time_to_respawn) /* kill the existing server. */
      kill(child_pid, SIGKILL);
    child_pid = spawn_server();
    if (child_pid <= 0) { /* unsuccessful spawn */
      cout << "z3client: spawning server failed\n";
      return "";
    }
    sleep(2); /* letting socket listen to be setup */
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("z3client: socket creation failed");
    return "";
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    perror("z3client: Invalid localhost network address");
    return "";
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
      < 0) {
    perror("z3client: connect() to z3server failed");
    return "";
  }

  /* Send the formula to the server */
  // cout << "z3client: Sending formula to server...\n";
  nchars = std::min(FORMULA_SIZE_BYTES, (int)formula.length());
  strncpy(form_buffer, formula.c_str(), nchars);
  form_buffer[nchars] = '\0';
  send(sock, form_buffer, nchars + 1, 0);

  /* Read back solver results. */
  // cout << "z3client: Waiting for solver results from server...\n";
  total_read = 0;
  do {
    nread = read(sock, res_buffer + total_read, RESULT_SIZE_BYTES - total_read);
    total_read += nread;
  } while (res_buffer[total_read - 1] != '\0' &&
           total_read < RESULT_SIZE_BYTES);
  if (total_read >= RESULT_SIZE_BYTES)
    cout << "Exhausted result read buffer\n";
  close(sock);

  nsolve++;

  return string(res_buffer);
}
