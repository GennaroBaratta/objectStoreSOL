#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <communication.h>
#include <fcntl.h>
#include <icl_hash.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// tutto ciò per poter usare uno switch (solo int)
#define REGISTER -1825436278
#define STORE 235691186
#define RETRIVE 1392069094
#define DELETE -1417066568
#define LEAVE 226835570

int unused;

icl_hash_t* usersTable;

int parseCmd(const char* cmd) {
  assert(cmd != NULL);

  int hash = 5381;
  int c;

  while ((c = *cmd++))
    hash = ((hash << 5) + hash) + c;

  // printf(":%d \n", hash);
  return hash;
}

void* clientHandler(void* arg) {
  int ret;
  char buf[BUFSIZE];
  char* saveprt;

  char* cmd;
  char response[BUFSIZE];
  char* token = NULL;

  char *name, *nameFile, *dirname = NULL, *pathFile;
  size_t lenFile;
  void* dataFile;
  FILE* file;

  long fd = (long)arg;

  int detachResult = pthread_detach(pthread_self());
  assert(detachResult == 0);
  do {
    memset(response, 0, BUFSIZE);
    SYSCALL(ret, read(fd, buf, BUFSIZE), "readn");
    if (ret <= 1)
      break;
    cmd = strtok_r(buf, " ", &saveprt);
    switch (parseCmd(cmd)) {
      case REGISTER:
        token = strtok_r(NULL, " ", &saveprt);

        name = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(name, token);

        dirname = malloc(sizeof(char) * (strlen(name) + strlen("data/")) + 1);
        strcpy(dirname, "data/");
        strncat(dirname, name, BUFSIZE);
        if (mkdir(dirname, 0777) && errno != EEXIST) {
          strncpy(response, "KO error while trying to create data directory \n",
                  BUFSIZE);
        } else if (icl_hash_insert(usersTable, name, arg) != NULL) {
          strcpy(response, "OK \n");
        } else {
          if (icl_hash_find(usersTable, name) != NULL)
            strcpy(response, "OK \n");
          else
            strcpy(response, "KO error on hash table\n");
        }
        
        //strcpy(response, "OK \n");
        break;
      case STORE:
        nameFile = strtok_r(NULL, " ", &saveprt);
        lenFile = strtol(strtok_r(NULL, " ", &saveprt), NULL, 10);
        if (errno == EINVAL || errno == ERANGE) {
          strcpy(response, "KO error on data length\n");
        } else {
          dataFile = malloc(lenFile);
          memset(dataFile, 0, lenFile);
          SYSCALL(ret, readn(fd, dataFile, lenFile), "readn file");

          pathFile =
              calloc(strlen(dirname) + strlen(nameFile) + 2, sizeof(char));
          strcpy(pathFile, dirname);
          strcat(pathFile, "/");
          strcat(pathFile, nameFile);
          file = fopen(pathFile, "w");
          if (file == NULL) {
            perror("fopen store");
            strcpy(response, "KO error on write file\n");
          } else {
            fwrite(dataFile, sizeof(char), lenFile, file);
            fclose(file);
            strcpy(response, "OK \n");
          }
          free(dataFile);
          free(pathFile);
        }
        break;
      case RETRIVE:
        break;
      case DELETE:
        break;
      case LEAVE:
        close(fd);
        fflush(stdout);
        free(dirname);
        return (NULL);
        break;
      default:
        printf("Error! operator is not correct");
    }
    SYSCALL(ret, writen(fd, response, strnlen(response, BUFSIZE) - 1), "write");
  } while (1);
  free(dirname);
  close(fd);
  printf("fd %li not reachable\n", fd);
  fflush(stdout);
  return (NULL);
}

int setNoBlocking(int fd) {
  int flags;
  SYSCALL(flags, fcntl(fd, F_GETFL), "fcntl");
  flags |= O_NONBLOCK;
  SYSCALL(unused, fcntl(fd, F_SETFL, flags), "fcntl");
  return unused;
}

void cleanup() {
  if ((unused = unlink(SOCKNAME)) == -1) {
    if (errno == ENOENT)
      return;
    perror("unlink");
  }
}
// int argc, char* argv[]
int main() {
  usersTable = icl_hash_create(1024, NULL, NULL);

  cleanup();
  atexit(cleanup);
  int listenfd;
  SYSCALL(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

  struct sockaddr_un serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

  int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) ==
      -1) {
    perror("setsockopt()");
    return 1;
  }

  SYSCALL(unused,
          bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)),
          "bind");
  SYSCALL(unused, listen(listenfd, MAXBACKLOG), "listen");

  if (mkdir("data", 0777) && errno != EEXIST)
    printf("error while trying to create data directory\n");

  long connfd;
  pthread_t tid;

  while (1) {
    if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("accept");
        exit(errno);
      }
    } else {
      //  SYSCALL(unused, setNoBlocking(connfd), "setNoBlocking");
      pthread_create(&tid, NULL, &clientHandler, (void*)connfd);
    }
  }

  close(listenfd);
  return 0;
}
