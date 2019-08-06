#include <clientLibrary.h>

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
  strcat(buf, "REGISTER ");
  strcat(buf, name);
  strcat(buf, " \n");

  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen");
  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read");
  printf("result: %s\n", buf);
  return strncmp(buf, "OK", 2);
}

int os_disconnect() {
  SYSCALL(notused, writen(sockfd, "LEAVE \n", 8), "writen");
  close(sockfd);
  return 0;
}

int os_store(char* name, void* block, size_t len) {
  char buf[BUFSIZE] = "";
  char strNum[20];

  sprintf(strNum, " %lu", len);
  strcat(buf, "STORE ");
  strcat(buf, name);
  strcat(buf, strNum);
  strcat(buf, " \n");

  SYSCALL(notused, writen(sockfd, buf, strlen(buf)), "writen");
  SYSCALL(notused, writen(sockfd, block, len), "writen");
  memset(buf, 0, BUFSIZE);
  SYSCALL(n, read(sockfd, buf, BUFSIZE), "read");
  printf("result: %s\n", buf);
  return strncmp(buf, "OK", 2);
}
