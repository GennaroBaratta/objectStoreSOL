#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700  // for nftw (POSIX.1-2008 marks ftw() as obsolete)
#include <assert.h>
#include <communication.h>
#include <dirent.h>
#include <fcntl.h>
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

void freeTable() {
  int ret = 0;
  SYSCALL(ret, icl_hash_destroy(usersTable, free, free), "destroy table");
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

int handle_store(char* path,
                 void* block,
                 size_t len,
                 size_t maxLenBlock,
                 int clientfd) {
  int ret = remove(path);
  if (ret < 0 && errno != ENOENT) {
    perror("remove in store");
    return -1;
  }
  int fd;
  SYSCALL(fd,
          open(path, O_CREAT | O_RDWR | O_APPEND,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
          "open in store");
  size_t written = 0;
  if (block != NULL) {
    written = write(fd, block, len < maxLenBlock ? len : maxLenBlock);
  }
  if (written < len) {
    size_t lenLeft = len - written;
    void* bufferino;
    CHECKNULL(bufferino, calloc(lenLeft, sizeof(char)),
              "Error on calloc bufferino");
    SYSCALL(ret, readn(clientfd, bufferino, lenLeft), "read file");
    if (ret > 0) {
      SYSCALL(ret, writen(fd, bufferino, lenLeft), "write2 in store");
    } else {
      perror("read 2 in store");
    }
    free(bufferino);
  }
  close(fd);
  return 0;
}

int handle_retrive(char* path, char** res) {
  int fd;
  SYSCALL(fd,
          open(path, O_CREAT | O_RDWR | O_APPEND,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
          "open in store");

  int readbytes = 0;

  // seek to end of file
  size_t size = lseek(fd, 0, SEEK_END);
  assert(size > 0);
  lseek(fd, 0, SEEK_SET);  // seek back to beginning of file

  void* buf = malloc(size + 1);

  SYSCALL(readbytes, readn(fd, buf, size), "readn in retrive");

  char strSize[25];
  sprintf(strSize, "%zu", size);
  *res = calloc(size + strlen(strSize) + 1 + strlen("DATA  \n "), sizeof(char));
  sprintf(*res, "DATA %s \n ", strSize);
  size_t sizeHeader = strlen(*res);

  memcpy(*res + sizeHeader, buf, size);
  // strncat(*res, buf, size);

  free(buf);
  close(fd);
  return readbytes + sizeHeader;
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
  dim = 0;
  nfiles = 0;
  SYSCALL(err, nftw("data", &internal_stats, 32, 0), "nftw on data");

  pthread_mutex_lock(&(usersTable->fieldMutex));
  printf("Clients connected: %zu\n", usersTable->nentries);
  pthread_mutex_unlock(&(usersTable->fieldMutex));
  printf("Total size: %zu\n", dim);
  printf("Total files: %zu\n", nfiles);
}