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
#include <vector>
#include "../../src/utils.h"

using namespace std;

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES (1 << 22)
#define RESULT_SIZE_BYTES (1 << 22)
#define SOLVER_RESPAWN_THRESOLD 1000


int SERVER_PORT = 8002; /* default port */
z3::context c;
pid_t child_pid_1 = -1;
pid_t child_pid_2 = -1;
pid_t pid;
int nsolve1 = 0;
int nsolve2 = 0;

char form_buffer[FORMULA_SIZE_BYTES + 1] = {0};
char res_buffer[RESULT_SIZE_BYTES + 1] = {0};

// server_id starts from 0
int get_server_port(int server_id) {
  assert(server_id >= 0);
  return (SERVER_PORT + server_id);
}

int spawn_server(int port) {
  // cout << "Hello before sleep\n";
  // sleep(10);
  // cout << "after sleep\n";
  cout << "Spawining Server with port: " << port << "\n";
  pid = fork();
  if (pid == -1) {
    cout << "Fork error occurred. Can't spawn a z3 solver server.";
    return -1;
  } else if (pid == 0) { /* in the child process; exec to z3server */
    std::string NEWPORT = std::to_string(port);
    char *argv_list[] = {(char *)"./z3server.out ", const_cast<char *>(NEWPORT.c_str()), (char *)NULL};
    execv("./z3server.out", argv_list);
    exit(-1); /* never supposed to get here until the exec fails. */
  } else {
    /* in the parent process; record and return the child pid for later. */
    return pid;
  }
}

void kill_server() {
  if (pid == 0) return;
  string cmd = "kill -9 " + to_string(pid);
  int status = system(cmd.c_str());
  if ((status != -1) && WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
    cout << "kill the z3 solver server successfully" << endl;
  } else {
    cout << "kill the z3 solver server failed" << endl;
  }
}

int create_and_connect_socket(int port) {
  int sock = 0;
  struct sockaddr_in serv_addr;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("z3client: socket creation failed");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    perror("z3client: Invalid localhost network address");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
      < 0) {
    perror("z3client: connect() to z3server failed");
    return -1;
  }
  return sock;
}

/* Send the formula to the server */
void send_formula(int sock, string formula) {
  int nchars;
  //cout << "z3client: Sending formula to server...\n";
  nchars = std::min(FORMULA_SIZE_BYTES, (int)formula.length());
  strncpy(form_buffer, formula.c_str(), nchars);
  form_buffer[nchars] = '\0';
  send(sock, form_buffer, nchars + 1, 0);
}

/* Send the formula to the server */
void read_from_solver(int sock) {
  int nread, total_read;
  total_read = 0;
  do {
    nread = read(sock, res_buffer + total_read, RESULT_SIZE_BYTES - total_read);
    total_read += nread;
  } while (res_buffer[total_read - 1] != '\0' &&
           total_read < RESULT_SIZE_BYTES);
  if (total_read >= RESULT_SIZE_BYTES)
    cout << "Exhausted result read buffer\n";
  close(sock);
}

/* Poll Server Status non-blocking */
int poll_servers(int sock, int timeout) {
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (sock, &fds);
  struct timeval tv1 = {timeout, 0};
  int readSockets = select (FD_SETSIZE, &fds, NULL, NULL, &tv1);
  return FD_ISSET (sock, &fds);
}
string write_problem_to_z3server(string formula) {
  // cout << "z3client: Received a formula to solve\n";

  /* Server One */
  bool no_child_now = child_pid_1 <= 0;
  bool time_to_respawn = (! no_child_now) &&
                         nsolve1 > 0 && nsolve1 % SOLVER_RESPAWN_THRESOLD == 0;
  if (no_child_now || time_to_respawn) {
    if (time_to_respawn) /* kill the existing server. */
      kill(child_pid_1, SIGKILL);

    child_pid_1 = spawn_server(get_server_port(0));
    if (child_pid_1 <= 0) { /* unsuccessful spawn */
      cout << "z3client: spawning server 1 failed\n";
      return "";
    }
    sleep(2); /* letting socket listen to be setup */
  }
  /* Server Two */
  no_child_now = child_pid_2 <= 0;
  time_to_respawn = (! no_child_now) &&
                    nsolve2 > 0 && nsolve2 % SOLVER_RESPAWN_THRESOLD == 0;
  if (no_child_now || time_to_respawn) {
    if (time_to_respawn) /* kill the existing server. */
      kill(child_pid_2, SIGKILL);

    child_pid_2 = spawn_server(get_server_port(1));
    if (child_pid_2 <= 0) { /* unsuccessful spawn */
      cout << "z3client: spawning server 1 failed\n";
      return "";
    }
    sleep(2); /* letting socket listen to be setup */
  }

  /* Make connection request to server */
  //cout << "Connecting Server 1\n";
  int sock1 = create_and_connect_socket(get_server_port(0));
  //cout << "Connecting Server 2\n";
  int sock2 = create_and_connect_socket(get_server_port(1));
  if (sock1 == -1 || sock2 == -1) { /* socket creation error */
    return "";
  }

  send_formula(sock1, formula);
  send_formula(sock2, formula);

  /* Block until one socket returns data */
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (sock1, &fds);
  FD_SET (sock2, &fds);
  struct timeval tv = {86400, 0}; /* set timeout for 24 hours */
  int readSockets = select (FD_SETSIZE, &fds, NULL, NULL, &tv);
  if (readSockets < 0) {
    perror("z3client: neither server returned anything");
    return "";
  }
  if (readSockets == 0) {
    perror("z3client: timeout");
    return "";
  }
  int server1_read = FD_ISSET (sock1, &fds);
  int server2_read = FD_ISSET (sock2, &fds);
  int status;
  if (server1_read > 0 && server2_read > 0) { /* both sockets are readable */
    // cout << "z3Client: both servers returned\n";
    read_from_solver(sock1);
    read_from_solver(sock2);
    nsolve1++;
    nsolve2++;
  } else if (server1_read > 0 && server2_read == 0) { /* socket 1 is readable */
    read_from_solver(sock1);
    server2_read = poll_servers(sock2, 2);
    if (server2_read > 0) {
      // cout << "z3client: both servers returned\n";
      read_from_solver(sock2);
      nsolve2++;
    } else {
      cout << "z3client: Only server 1 returned. Killing server 2\n";
      kill(child_pid_2, SIGKILL);
      waitpid(child_pid_2, &status, 0);
      child_pid_2 = spawn_server(get_server_port(1));
    }
    nsolve1++;
  } else if (server1_read == 0 && server2_read > 0) { /* socket 2 is readable */
    read_from_solver(sock2);
    server1_read = poll_servers(sock1, 2);
    if (server1_read > 0) {
      // cout << "z3client: both servers returned\n";
      read_from_solver(sock1);
      nsolve1++;
    } else {
      cout << "z3client: Only server 2 returned. Killing server 1\n";
      kill(child_pid_1, SIGKILL);
      waitpid(child_pid_1, &status, 0);
      child_pid_1 = spawn_server(get_server_port(0));
    }
    nsolve2++;
  }
  /* Read back solver results. */
  // cout << "z3client: Waiting for solver results from server...\n";
  return string(res_buffer);
}
