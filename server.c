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
        char* pathFile =
            calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
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
        free(bufferino);
        break;
      case RETRIVE:
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
    SYSCALL(ret, writen(fd, response, strnlen(response, BUFSIZE) - 1), "write");
  } while (leave != 1);
  close(fd);
  return (NULL);
}

/*
void* clientHandler(void* arg) {
  int ret;
  char buf[BUFSIZE];
  char* saveprt;
  char* saveprt1;
  char* cmd;
  char response[BUFSIZE];
  char *token = NULL, *token1 = NULL;
  char tmpData[BUFSIZE];

  char header[BUFSIZE];

  char *name, *nameFile, *dirname = NULL, *pathFile;
  size_t lenFile;
  char* dataFile;
  FILE* file;

  long fd = (long)arg;

  int detachResult = pthread_detach(pthread_self());
  assert(detachResult == 0);
  do {
    memset(response, 0, BUFSIZE);
    memset(buf, 0, BUFSIZE);
    SYSCALL(ret, read(fd, buf, BUFSIZE), "first read ");
    if (ret < 1)
      break;

    token = strtok_r(buf, "\n", &saveprt1);
    strcpy(header, token);

    token1 = strtok_r(NULL, "", &saveprt1);
    if (token1 != NULL) {
      strcpy(tmpData, token1);
    }
    cmd = strtok_r(token, " ", &saveprt);
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
        strcpy(response, "OK \n");
        break;
      case STORE:
        CHECKNULL(token, strtok_r(NULL, " ", &saveprt), "strtok_r");
        CHECKNULL(nameFile, malloc(sizeof(char*) * (strlen(token) + 1)),
                  "malloc nameFile");
        strcpy(nameFile, token);
        lenFile = strtol(strtok_r(NULL, " ", &saveprt), NULL, 10);

        if (errno == EINVAL || errno == ERANGE) {
          strcpy(response, "KO error on data length\n");
        } else {
          CHECKNULL(dataFile, malloc((lenFile + 1) * sizeof(char)),
                    "malloc datafile");
          memset(dataFile, 0, lenFile);
          int tmpDataLen = 0;

          strcat(dataFile, tmpData);

          char bufferino[lenFile - tmpDataLen + 1];
          SYSCALL(ret, readn(fd, bufferino, lenFile - tmpDataLen),
                  "readn file");
          strncat(dataFile, bufferino, BUFSIZE);
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
          free(nameFile);
          free(dataFile);
          free(pathFile);
        }
        break;
      case RETRIVE:
        break;
      case DELETE:
        break;
      case LEAVE:
        fflush(stdout);
        free(dirname);
        close(fd);
        return (NULL);
        break;
      default:
        printf("Error! operator [%s] with header [%s] is not correct\n", cmd,
               header);
    }
    SYSCALL(ret, writen(fd, response, strnlen(response, BUFSIZE) - 1), "write");
  } while (1);

  if (dirname)
    free(dirname);
  printf("fd %li not reachable\n", fd);
  fflush(stdout);
  close(fd);
  return (NULL);
}*/

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
