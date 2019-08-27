#include <communication.h>

int readn(long fd, void* buf, size_t size) {
  size_t left = size;
  char* bufptr = (char*)buf;
  while (left > 0) {
    int r;
    if ((r = read((int)fd, bufptr, left)) == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("ciao");
        return size - left;
      }
      perror("read");
      return -1;
    }
    if (r == 0)
      return 0;  // gestione chiusura socket
    left -= r;
    bufptr += r;
  }
  return size;
}

int writen(long fd, void* buf, size_t size) {
  size_t left = size;
  int r;
  char* bufptr = (char*)buf;
  while (left > 0) {
    if ((r = write((int)fd, bufptr, left)) == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("ciaone");
        return size - left;
      }
      perror("write");
      return -1;
    }
    if (r == 0)
      return 0;
    left -= r;
    bufptr += r;
  }
  return size;
}
