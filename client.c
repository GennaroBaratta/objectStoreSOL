#include <clientLibrary.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char* testStr = "Ciao objectstore, il mio nome è Gennaro";
int successes = 0;


void store() {
  if (os_store("a", testStr, strlen(testStr)))
    perror("store");
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <client's name> <n>\n", argv[0]);
    return -1;
  }

  if (os_connect(argv[1]) == 0) {
    switch (atoi(argv[2])) {
      case 1:
        store();
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
