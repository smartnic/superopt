#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "z3++.h"

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES 65536

using namespace std;

z3::context c;

void read_from_z3client() {
  key_t key = ftok("shm_z3input", FORMULA_SHM_KEY);
  int shmid = shmget(key, FORMULA_SIZE_BYTES, 0666 | IPC_CREAT);
  char *str = (char*) shmat(shmid, NULL, 0);
  cout << "Read formula:\n";
  cout << str;
  cout << "\n";
  shmdt(str);
  shmctl(shmid, IPC_RMID, NULL);
}

int main() {
  /* Receive a z3 smtlib2 formula in a shared memory segment, and
     return sat or unsat in another one. */
  read_from_z3client();
  return 0;
}
