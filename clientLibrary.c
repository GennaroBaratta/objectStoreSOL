#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <clientLibrary.h>
#include <string.h>
#include <sys/time.h>

int sockfd;
int notused, n;

int os_connect(char* name) {
  struct sockaddr_un serv_addr;
  SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

  SYSCALL(notused,
          connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)),
          "connect");

  char buf[BUFSIZE] = "";
  sprintf(buf, "REGISTER %s \n", name);

  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen client");
  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read client");
  return strncmp(buf, "OK", 2);
}

int os_disconnect() {
  char buf[BUFSIZE];
  memset(buf, 0, BUFSIZE);
  SYSCALL(notused, writen(sockfd, "LEAVE \n", 8), "writen");
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read client");
  close(sockfd);
  return strncmp(buf, "OK", 2);
}

int os_store(char* name, void* block, size_t len) {
  assert(block);
  char buf[BUFSIZE] = "";

  sprintf(buf, "STORE %s %zu \n", name, len);

  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen client");
  SYSCALL(notused, writen(sockfd, block, len), "writen client");
  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read client");
  return strncmp(buf, "OK", 2);
}

void* os_retrive(char* name) {
  assert(name);
  char buf[BUFSIZE];
  char* data;
  char *token1, *token2, *token3, *tokptr;
  int ret;

  sprintf(buf, "RETRIVE %s \n", name);

  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen client");

  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE - 1), "read client");

  CHECKNULL(token1, strtok_r(buf, " ", &tokptr), "strtok_r retrive");

  if (strncmp(token1, "KO", 2) == 0) {
    return NULL;
  }
  token2 = strtok_r(NULL, "\n", &tokptr);
  if (!token2) {
    return NULL;
  }
  size_t lenNum = strlen(token2) - 1;

  size_t len = strtol(token2, NULL, 10);
  CHECKNULL(token3, strtok_r(NULL, " ", &tokptr), "strtok_r3 retrive");

  size_t maxLenBlock = BUFSIZE - strlen("DATA  \n ") - lenNum - 1;
  CHECKNULL(data, calloc((len + 1), sizeof(char)), "malloc");
  memcpy(data, token3, len < maxLenBlock ? len : maxLenBlock);
  if (len > maxLenBlock) {
    void* bufferino = calloc(len - maxLenBlock + 1, sizeof(char));
    SYSCALL(ret, readn(sockfd, bufferino, len - maxLenBlock), "read file");
    if (ret > 0) {
      memcpy(data + maxLenBlock, bufferino, ret);
    }
    free(bufferino);
  }

  return data;
}

int os_delete(char* name) {
  assert(name);
  char buf[BUFSIZE] = "";
  sprintf(buf, "DELETE %s \n", name);
  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen client");

  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read client");
  return strncmp(buf, "OK", 2);
}