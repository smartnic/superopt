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
#include "port.h"
#include "../../src/utils.h"

using namespace std;

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES (1 << 22)
#define RESULT_SIZE_BYTES (1 << 22)
#define SOLVER_RESPAWN_THRESOLD 1000


// int SERVER_PORT = 8020;
z3::context c;
pid_t child_pid_1 = -1;
pid_t child_pid_2 = -1;
pid_t pid;
int nsolve = 0;

char form_buffer[FORMULA_SIZE_BYTES + 1] = {0};
char res_buffer[RESULT_SIZE_BYTES + 1] = {0};

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
  cout << "z3client: Sending formula to server...\n";
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

string write_problem_to_z3server(string formula) {
  // cout << "z3client: Received a formula to solve\n";

  /* check if server process exists currently; if not, spawn it. */
  bool no_child_now = child_pid_1 <= 0;
  bool time_to_respawn = (! no_child_now) &&
                         nsolve > 0 && nsolve % SOLVER_RESPAWN_THRESOLD == 0;
  if (no_child_now || time_to_respawn) {
    if (time_to_respawn) /* kill the existing server. */ //FIX
      kill(child_pid_1, SIGKILL);

    child_pid_1 = spawn_server(SERVER_PORT);
    // cout << "Hello1";
    sleep(2);
    // cout << "Hello2";
    child_pid_2 = spawn_server(SERVER_PORT+1000);
    if (child_pid_1 <= 0) { /* unsuccessful spawn */
      cout << "z3client: spawning server 1 failed\n";
      return "";
    }
    if (child_pid_2 <= 0) { /* unsuccessful spawn */
      cout << "z3client: spawning server 2 failed\n";
      return "";
    }
    sleep(4); /* letting socket listen to be setup */
  }

  /* Make connection request to server */
  cout << "Connecting Server 1\n";
  int sock1 = create_and_connect_socket(SERVER_PORT);
  cout << "Connecting Server 2\n";
  int sock2 = create_and_connect_socket(SERVER_PORT + 1000);
  if (sock1 == -1 || sock2 == -1) { /* socket creation error */
    return "";
  }

  send_formula(sock1, formula);
  // sleep(15);
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
  // cout << "Reached Here\n";
  cout << server1_read << "\n";
  cout << server2_read << "\n";
  int status; 
  if (server1_read > 0 && server2_read > 0) { /* both sockets are readable */
    cout << "Both1 Servers returned\n";
    read_from_solver(sock1);
    read_from_solver(sock2);
    nsolve++;
  } else if (server1_read > 0 && server2_read == 0) { /* socket 1 is readable */
    read_from_solver(sock1);
    FD_ZERO (&fds);
    FD_SET (sock2, &fds);
    struct timeval tv1 = {2, 0};
    auto t1 = NOW;
    readSockets = select (FD_SETSIZE, &fds, NULL, NULL, &tv1);
    auto t2 = NOW;
    cout << "second select: " << DUR(t1, t2) << endl; 
    if (readSockets == 0){
      cout << "Timed out\n";
    } 
    server2_read = FD_ISSET (sock2, &fds);
    cout << server2_read << "\n";
    if (server2_read > 0){
      cout << "Both2 Servers returned\n";
      read_from_solver(sock2);
    } else {
      cout << "Only server 1 returned. ";
      cout << "Killing server 2\n";
      kill(child_pid_2, SIGKILL);
      t1 = NOW;
      waitpid(child_pid_2, &status, 0);
      t2 = NOW;
      cout << "Wait time: " << DUR(t1, t2) << endl;
      child_pid_2 = spawn_server(SERVER_PORT+1000);
    }
    nsolve++;
  } else if (server1_read == 0 && server2_read > 0) { /* socket 2 is readable */
    read_from_solver(sock2);
    FD_ZERO (&fds);
    FD_SET (sock1, &fds);
    struct timeval tv1 = {2, 0};
    auto t1 = NOW;
    readSockets = select (FD_SETSIZE, &fds, NULL, NULL, &tv1);
    auto t2 = NOW;
    cout << "second select: " << DUR(t1, t2) << endl; 
    if (readSockets == 0){
      cout << "Timed out\n";
    }   
    server1_read = FD_ISSET (sock1, &fds);
    cout << server1_read << "\n";
    if (server1_read > 0){
      cout << "Both2 Servers returned\n";
      read_from_solver(sock1);
    } else {
      cout << "Only server 2 returned. ";
      cout << "Killing server 1\n";
      kill(child_pid_1, SIGKILL);
      t1 = NOW;
      waitpid(child_pid_1, &status, 0);
      t2 = NOW;
      cout << "Wait time: " << DUR(t1, t2) << endl; 
      child_pid_1 = spawn_server(SERVER_PORT);
    }
    nsolve++;
  }
  //exit(0);
  /* Read back solver results. */
  // cout << "z3client: Waiting for solver results from server...\n";
  return string(res_buffer);
}
