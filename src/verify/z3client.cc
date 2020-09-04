#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>
#include <cstring>

using namespace std;

#define FORMULA_SHM_KEY 224
#define RESULTS_SHM_KEY 46
#define FORMULA_SIZE_BYTES 65536

void write_to_z3server(string formula) {
  key_t key = ftok("shm_z3input", FORMULA_SHM_KEY);
  int shmid = shmget(key, FORMULA_SIZE_BYTES, 0666 | IPC_CREAT);
  char *str = (char*) shmat(shmid, NULL, 0);
  /* Copy the incoming formula into the memory segment. */
  formula.copy(str, formula.size() + 1);
  str[formula.size() + 1] = '\0';
  shmdt(str);
}

