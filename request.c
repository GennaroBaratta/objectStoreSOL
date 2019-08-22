//#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <communication.h>
#include <dirent.h>
#include <ftw.h>
#include <icl_hash.h>
#include <request.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

icl_hash_t* usersTable;
static size_t dim = 0;
static size_t nfiles = 0;

void initTable() {
  CHECKNULL(usersTable, icl_hash_create(1024, NULL, NULL), "Create hashtable");
}

void freeTable(){
  int ret=0;
  SYSCALL(ret,icl_hash_destroy(usersTable,free,free),"destroy table");
}

int handle_register(char* name) {
  char* nameForTable;
  CHECKNULL(nameForTable, calloc(strlen(name) + 1, sizeof(char)),
            "Error on calloc name for table");
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
    perror(path);
    return -1;
  }
  size_t now = fwrite(block, sizeof(char), len, file);

  assert(now == len);
  fclose(file);

  return 0;
}

int handle_retrive(char* path, char** res) {
  FILE* file;
  file = fopen(path, "r");
  if (file == NULL) {
    perror(path);
    return -1;
  }
  size_t readn = 0;

  fseek(file, 0, SEEK_END);   // seek to end of file
  size_t size = ftell(file);  // get current file pointer
  assert(size > 0);
  fseek(file, 0, SEEK_SET);  // seek back to beginning of file

  char* buf = calloc(size + 1, sizeof(char));
  while (readn < size) {
    int tempReadn = 0;
    SYSCALL(tempReadn, fread(buf + readn, sizeof(char), size - readn, file),
            "fread retrive");
    if (tempReadn == 0) {
      perror("Error retriving file");
      break;
    }
    readn += tempReadn;
  }
  assert(readn == size);
  *res = calloc(size + sizeof(size) + 1 + strlen("DATA  \n "), sizeof(char));

  sprintf(*res, "DATA %zu \n %s", size, buf);

  free(buf);
  fclose(file);
  return 0;
}

int handle_delete(char* path) {
  int ret;
  SYSCALL(ret, remove(path), "remove");
  return 0;
}

int handle_disconnect(char* name) {
  return icl_hash_delete(usersTable, name, free, NULL);
}

static int internal_stats(const char* fpath,
                          const struct stat* sb,
                          int typeflag,
                          struct FTW* ftwbuf) {
  if (typeflag == FTW_F) {
    dim += sb->st_size;
    nfiles++;
  }
  return 0;
}

void handle_print_stats() {
  int err;
  SYSCALL(err, nftw("data", &internal_stats, 32, 0), "nftw on data");

  pthread_mutex_lock(&(usersTable->fieldMutex));
  printf("Clients connected: %zu\n", usersTable->nentries);
  pthread_mutex_unlock(&(usersTable->fieldMutex));
  printf("Total size: %zu\n", dim);
  printf("Total files: %zu\n", nfiles);
}