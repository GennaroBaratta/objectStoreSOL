#include <clientLibrary.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char* testStr =
    "INIZIO-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_"
    "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_FINE";
int successes = 0;

void store(char* clientName) {
  char* nameFile = calloc(strlen(clientName) + 4, sizeof(char));
  strcpy(nameFile, clientName);
  strcat(nameFile, "_aa");
  if (os_store(nameFile, testStr, strlen(testStr)) == -1) {
    perror("store");
    free(nameFile);
    exit(errno);
  }
  free(nameFile);
}

void retrive(char* clientName) {
  char* nameFile = calloc(strlen(clientName) + 4, sizeof(char));
  strcpy(nameFile, clientName);
  strcat(nameFile, "_aa");
  char** data = os_retrive(nameFile);
  if (data == NULL) {
    perror("retrive");
    free(nameFile);
    exit(errno);
  }
  printf("%s\n",*data);
  free(*data);
  free(nameFile);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <client's name> <n>\n", argv[0]);
    return -1;
  }
  if (os_connect(argv[1]) == 0) {
    switch (atoi(argv[2])) {
      case 1:
        store(argv[1]);
        break;
      case 2:
        retrive(argv[1]);
        break;
      case 3:
        // delete
        break;
      default:
        printf("n must be between 1 and 3\n");
        break;
    }
    os_disconnect();
  } else {
    return -1;
  }
  return 0;
}
