#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <communication.h>
#include <fcntl.h>
#include <pthread.h>
#include <request.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

// tutto ciò per poter usare uno switch (solo int)
// i valori hash dei vari comandi
#define REGISTER -1825436278
#define STORE 235691186
#define RETRIVE 1392069094
#define DELETE -1417066568
#define LEAVE 226835570

#define NUM_THREADS 12

int unused;

int listenfd;

static volatile sig_atomic_t stopFlag = 0;
pthread_mutex_t worker_stop_mtx = PTHREAD_MUTEX_INITIALIZER;

volatile int worker_num = 1;
pthread_mutex_t worker_num_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t worker_num_cond = PTHREAD_COND_INITIALIZER;

static void* sig_thread(void* arg) {
  sigset_t* set = arg;
  int s, sig;
  while (1) {
    s = sigwait(set, &sig);
    if (s != 0) {
      perror("sigwait");
    }
    switch (sig) {
      case SIGUSR1:
        printf("Print statistics...\n");
        handle_print_stats();
        break;
      default:
        pthread_mutex_lock(&worker_stop_mtx);
        stopFlag = 1;
        pthread_mutex_unlock(&worker_stop_mtx);

        SYSCALL(s, shutdown(listenfd, SHUT_RD), "Error shutdowning");

        pthread_mutex_lock(&worker_num_mtx);
        worker_num--;
        pthread_mutex_unlock(&worker_num_mtx);
    }
  }
  return (NULL);
}

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

  // tokenizzazione
  char* token;
  char *tokFull, *tokHeader;
  char* cmd;

  int ret;
  char* response = NULL;
  char buf[BUFSIZE];

  char header[BUFSIZE];
  int leave = 0;

  char* block = NULL;

  char name[BUFSIZE];
  char nameFile[BUFSIZE];

  char* pathFile;

  char* res = NULL;
  do {
    pthread_mutex_lock(&worker_stop_mtx);
    if (stopFlag) {
      pthread_mutex_unlock(&worker_stop_mtx);
      break;
    }
    pthread_mutex_unlock(&worker_stop_mtx);
    CHECKNULL(response, calloc(BUFSIZE, sizeof(char)),
              "Error on calloc memory");

    memset(buf, 0, BUFSIZE);
    // sto meno uno lo devo capire ancora
    SYSCALL(ret, read(fd, buf, BUFSIZE - 1), "first read ");
    if (ret < 1)
      break;
    strcat(buf, "\0");
    CHECKNULL(token, strtok_r(buf, "\n", &tokFull), "Error tokening header");
    sprintf(header, "%s", token);

    CHECKNULL(cmd, strtok_r(token, " ", &tokHeader), "Error tokening command");
    switch (parseCmd(cmd)) {
      case REGISTER:

        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        sprintf(name, "%s", token);
        ret = handle_register(name);
        if (ret == 0) {
          sprintf(response, "OK \n");
        } else {
          sprintf(response, "KO not registred\n");
        }
        break;
      case STORE:
        // può essere NULL
        block = strtok_r(NULL, "", &tokFull);

        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        sprintf(nameFile, "%s", token);
        char* lenStr;
        CHECKNULL(lenStr, strtok_r(NULL, " ", &tokHeader), "tokLen");
        size_t len;
        len = strtol(lenStr, NULL, 10);
        if (errno == EINVAL || errno == ERANGE) {
          strcpy(response, "KO error on data length\n");
          break;
        }
        size_t lenLeft = len;
        char* file;
        CHECKNULL(file, calloc(len + 1, sizeof(char)), "Error on calloc file");
        if (block != NULL) {
          lenLeft -= strlen(block);
          strcat(block, "\0");
          // len < BUFSIZE ? len : BUFSIZE - strlen(header)+1
          sprintf(file, "%s", block);
        }

        char* bufferino;
        CHECKNULL(bufferino, calloc(lenLeft + 1, sizeof(char)),
                  "Error on calloc bufferino");
        if (lenLeft > 0) {
          SYSCALL(ret, readn(fd, bufferino, lenLeft), "read file");
          if (ret > 0) {
            strcat(file, bufferino);
          }
        }

        CHECKNULL(pathFile,
                  calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
                         sizeof(char)),
                  "Error on calloc pathFile");

        sprintf(pathFile, "data/%s/%s", name, nameFile);

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
        sprintf(nameFile, "%s", token);

        CHECKNULL(pathFile,
                  calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
                         sizeof(char)),
                  "Error on calloc pathFile");

        sprintf(pathFile, "data/%s/%s", name, nameFile);

        ret = handle_retrive(pathFile, &res);
        if (ret == 0) {
          response = res;
        } else {
          sprintf(response, "KO \n");
        }

        free(pathFile);
        break;
      case DELETE:
        CHECKNULL(token, strtok_r(NULL, " ", &tokHeader), "tokName");
        sprintf(nameFile, "%s", token);
        CHECKNULL(pathFile,
                  calloc(strlen("data/") + strlen(name) + strlen(nameFile) + 2,
                         sizeof(char)),
                  "Error on calloc pathFile");

        sprintf(pathFile, "data/%s/%s", name, nameFile);

        ret = handle_delete(pathFile);
        if (ret == 0) {
          sprintf(response, "OK \n");
        } else {
          sprintf(response, "KO \n");
        }

        free(pathFile);
        break;
      case LEAVE:
        if (handle_disconnect(name) == 0) {
          sprintf(response, "OK \n");
        } else {
          sprintf(response, "KO removing user for connected users\n");
        }
        leave = 1;
        break;
      default:
        printf("Error! header [%s] is not correct\n", header);
    }
    if (strncmp(response, "|", 1) == 0)
      printf("\tcazz\t\n");
    SYSCALL(ret, writen(fd, response, strlen(response)), "write");
    free(response);
  } while (leave != 1);

  pthread_mutex_lock(&worker_num_mtx);
  worker_num--;
  pthread_mutex_unlock(&worker_num_mtx);
  pthread_cond_signal(&worker_num_cond);
  SYSCALL(unused, close(fd), "Error on close");
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
  pthread_t tid[NUM_THREADS];
  pthread_attr_t attr;

  ISZERO(unused, pthread_attr_init(&attr), "Error init pthread_attr");

  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  sigset_t set;

  // block signal
  SYSCALL(unused, sigemptyset(&set), "Error on sigemptyset");
  SYSCALL(unused, sigaddset(&set, SIGQUIT), "Error on sigaddset");
  SYSCALL(unused, sigaddset(&set, SIGUSR1), "Error on sigaddset");
  SYSCALL(unused, sigaddset(&set, SIGTSTP), "Error on sigaddset");
  ISZERO(unused, pthread_sigmask(SIG_BLOCK, &set, NULL), "pthread_sigmask");
  ISZERO(unused, pthread_create(&tid[0], &attr, &sig_thread, (void*)&set),
         "pthread_create");
  initTable();

  cleanup();
  ISZERO(unused, atexit(cleanup), "Atexit");
  SYSCALL(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

  struct sockaddr_un serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

  int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) ==
      -1) {
    perror("setsockopt()");
    exit(errno);
  }

  SYSCALL(unused,
          bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)),
          "bind");
  SYSCALL(unused, listen(listenfd, MAXBACKLOG), "listen");

  if (mkdir("data", 0777) && errno != EEXIST) {
    printf("error while trying to create data directory\n");
    exit(errno);
  }

  long connfd;

  pthread_mutex_lock(&worker_stop_mtx);
  while (!stopFlag) {
    pthread_mutex_unlock(&worker_stop_mtx);
    if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("accept");
      }
    } else {
      pthread_mutex_lock(&worker_num_mtx);
      if (worker_num >= NUM_THREADS) {
        pthread_cond_wait(&worker_num_cond, &worker_num_mtx);
      }
      pthread_create(&tid[worker_num], &attr, &clientHandler, (void*)connfd);
      worker_num++;
      pthread_mutex_unlock(&worker_num_mtx);
    }

    pthread_mutex_lock(&worker_stop_mtx);
  }
  pthread_mutex_unlock(&worker_stop_mtx);
  pthread_attr_destroy(&attr);

  pthread_mutex_lock(&worker_num_mtx);
  while (worker_num > 0) {
    pthread_cond_wait(&worker_num_cond, &worker_num_mtx);
  }
  pthread_mutex_unlock(&worker_num_mtx);
  SYSCALL(unused, close(listenfd), "Error on close");
  return 0;
}
