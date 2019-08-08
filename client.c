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
  char* nameFile = malloc(strlen(clientName) + 4);
  strcpy(nameFile, clientName);
  strcat(nameFile, "_aa");
  if (os_store(nameFile, testStr, strlen(testStr)) == -1) {
    perror("store");
    free(nameFile);
    exit(errno);
  }
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
        // retrive
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
