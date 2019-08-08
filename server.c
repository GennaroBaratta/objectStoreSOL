#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <communication.h>
#include <fcntl.h>
#include <pthread.h>
#include <request.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// tutto ciò per poter usare uno switch (solo int)
#define REGISTER -1825436278
#define STORE 235691186
#define RETRIVE 1392069094
#define DELETE -1417066568
#define LEAVE 226835570

int unused;

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
  long fd = (long)arg;
  int detachResult = pthread_detach(pthread_self());
  assert(detachResult == 0);

  // tokenizzazione
  char* token;
  char *tokFull, *tokHeader;
  char* cmd;

  int ret;
  char response[BUFSIZE];
  char buf[BUFSIZE];

  char header[BUFSIZE];
  int leave = 0;

  char* block = NULL;

  char name[BUFSIZE];
  char nameFile[BUFSIZE];

  char* pathFile;
  do {
    memset(response, 0, BUFSIZE);
    memset(buf, 0, BUFSIZE);
    // sto meno uno lo devo capire ancora
    SYSCALL(ret, read(fd, buf, BUFSIZE - 1), "first read ");
    if (ret < 1)
      break;
    strcat(buf, "\0");
    token = strtok_r(buf, "\n", &tokFull);
    strcpy(header, token);

    cmd = strtok_r(token, " ", &tokHeader);
    switch (parseCmd(cmd)) {
      case REGISTER:

        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        strcpy(name, token);
        ret = handle_register(name);
        if (ret == 0) {
          strcpy(response, "OK \n");
        } else {
          strcpy(response, "KO non registrato\n");
        }
        break;
      case STORE:
        // può essere NULL
        block = strtok_r(NULL, "", &tokFull);

        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        strcpy(nameFile, token);
        char* lenStr;
        CHECKNULL(lenStr, strtok_r(NULL, " ", &tokHeader), "tokLen");
        size_t len;
        len = strtol(lenStr, NULL, 10);
        if (errno == EINVAL || errno == ERANGE) {
          strcpy(response, "KO error on data length\n");
        }
        char* file = calloc(len + 1, sizeof(char));
        if (block != NULL) {
          strcat(block, "\0");
          // len < BUFSIZE ? len : BUFSIZE - strlen(header)+1
          sprintf(file, "%s", block);
        }
        char* bufferino = calloc(len + 1, sizeof(char));
        SYSCALL(ret, read(fd, bufferino, len), "read file");
        if (ret > 0) {
          strcat(file, bufferino);
        }
        pathFile = calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
                          sizeof(char));
        strcpy(pathFile, "data/");
        strcat(pathFile, name);
        strcat(pathFile, "/");
        strcat(pathFile, nameFile);

        ret = handle_store(pathFile, file, len);
        if (ret == 0) {
          strcpy(response, "OK \n");
        } else {
          strcpy(response, "KO \n");
        }
        free(pathFile);
        free(file);
        free(bufferino);
        break;
      case RETRIVE:

        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        strcpy(nameFile, token);

        pathFile = calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
                          sizeof(char));
        strcpy(pathFile, "data/");
        strcat(pathFile, name);
        strcat(pathFile, "/");
        strcat(pathFile, nameFile);

        ret = handle_retrive(pathFile);
        if (ret == 0) {
          strcpy(response, "DATA 14 \nINIZIO-_-_FINE");
        } else {
          strcpy(response, "KO \n");
        }
        free(pathFile);
        break;
      case DELETE:
        break;
      case LEAVE:
        if (handle_disconnect(name) == 0) {
          strcpy(response, "OK \n");
        } else {
          strcpy(response, "KO removing user for connected users\n");
        }
        leave = 1;
        break;
      default:
        printf("Error! header [%s] is not correct\n", header);
    }
    SYSCALL(ret, writen(fd, response, strnlen(response, BUFSIZE) ), "write");
  } while (leave != 1);
  close(fd);
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
  initTable();

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
      // SYSCALL(unused, setNoBlocking(connfd), "setNoBlocking");

      pthread_create(&tid, NULL, &clientHandler, (void*)connfd);
    }
  }

  close(listenfd);
  return 0;
}
