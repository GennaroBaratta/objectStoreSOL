#include <clientLibrary.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
// incremento di 4995 bytes 20 volte per raggiungere 100.000bytes
// 4995 = 5 * 999 = 5 * 9 * 111
char* str111 =
    "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
    "|"
    "||||||||||||||||||||||||||||||||||||";
char* strBegin =
    "INIZIO-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_";
int nstores = 0;
int nretrives = 0;
int ndeletes = 0;
int successes = 0;
int failures = 0;
size_t kbytesStores = 0;
size_t kbytesRetrives = 0;

void store(char* file) {
  char incStr[4996] = "";
  char* testStr = NULL;
  for (int k = 0; k < 45; k++) {
    strncat(incStr, str111, 4995);
  }

  for (int i = 0; i <= 20; i++) {
    char* incStrTot = NULL;
    char nameFile[BUFSIZE];
    sprintf(nameFile, "%s%d", file, i);

    CHECKNULL(incStrTot, calloc((i * strlen(incStr) + 1), sizeof(char)),
              "calloc");

    for (int j = 0; j < i; j++) {
      strcat(incStrTot, incStr);
    }

    CHECKNULL(testStr,
              calloc((strlen(incStrTot) + 5 + strlen(strBegin)), sizeof(char)),
              "calloc");
    sprintf(testStr, "%s%sFINE", strBegin, incStrTot);
    nstores++;
    if (os_store(nameFile, testStr, strlen(testStr)) == -1) {
      perror("store");
      failures++;
    } else {
      kbytesStores += strlen(testStr);
      successes++;
    }
    free(testStr);
    free(incStrTot);
  }
}

void retrive(char* file) {
  char nameFile[BUFSIZE];
  for (int i = 0; i <= 20; i++) {
    memset(nameFile, 0, BUFSIZE);
    sprintf(nameFile, "%s%d", file, i);
    nretrives++;
    char* data = os_retrive(nameFile);
    if (data == NULL) {
      perror("retrive");
      failures++;
    } else {
      size_t lendata = strlen(data);
      kbytesRetrives += lendata;
      lendata == (i * 4995 + 100) ? successes++ : failures++;
    }
    free(data);
  }
}

void delete (char* file) {
  for (int i = 0; i <= 20; i++) {
    char nameFile[BUFSIZE];
    sprintf(nameFile, "%s%d", file, i);
    ndeletes++;
    if (os_delete(nameFile) == -1) {
      perror("delete");
      failures++;
    } else {
      successes++;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <client's name> <n>\n", argv[0]);
    return -1;
  }
  char* nameFile = calloc(strlen(argv[1]) + 5, sizeof(char));
  strcpy(nameFile, argv[1]);
  strcat(nameFile, "_aa");

  if (os_connect(argv[1]) == 0) {
    switch (atoi(argv[2])) {
      case 1:
        store(nameFile);
        break;
      case 2:
        retrive(nameFile);
        break;
      case 3:
        delete (nameFile);
        break;
      default:
        printf("n must be between 1 and 3\n");
        break;
    }
    os_disconnect();
  } else {
    free(nameFile);
    return -1;
  }

  kbytesStores /= 1000;
  kbytesRetrives /= 1000;

  printf(
      "---Client: %s\n---Stores: %d\n---Retrives: %d\n---Deletes: %"
      "d\n---Successes: %d\n---Failures: %d\n---Stored "
      "KB: %zu\n---Retrived KB: %zu\n\n",
      argv[1], nstores, nretrives, ndeletes, successes, failures, kbytesStores,
      kbytesRetrives);
  free(nameFile);
  fflush(stdout);
  return 0;
}
