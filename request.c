#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <communication.h>
#include <icl_hash.h>
#include <request.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

icl_hash_t* usersTable;

void initTable() {
  usersTable = icl_hash_create(1024, NULL, NULL);
}

int handle_register(char* name) {
  char* nameForTable = calloc(strlen(name) + 1, sizeof(char));
  strcpy(nameForTable, name);

  if (icl_hash_find(usersTable, nameForTable) != NULL) {  // exists
    printf("esiste già\n");
    return -1;
  } else {
    if (icl_hash_insert(usersTable, nameForTable, nameForTable) !=
        NULL) {  // no error

      char* dirname =
          malloc(sizeof(char) * (strlen(name) + strlen("data/")) + 1);
      strcpy(dirname, "data/");
      strncat(dirname, name, BUFSIZE);
      if (mkdir(dirname, 0777) && errno != EEXIST) {
        free(dirname);
        return -1;
      }
      free(dirname);
      return 0;
    }
  }
  return -1;
}

int handle_store(char* path, void* block, size_t len) {
  assert(len == strlen(block));
  FILE* file;
  file = fopen(path, "w");
  if (file == NULL) {
    perror("fopen store");
    return -1;
  }
  fwrite(block, sizeof(char), len, file);
  fclose(file);

  return 0;
}

int handle_retrive(char* path) {
  printf("pathFile: %s\n", path);
  return 0;
}

int handle_delete(char* name);

int handle_disconnect(char* name) {
  return icl_hash_delete(usersTable, name, free, NULL);
}