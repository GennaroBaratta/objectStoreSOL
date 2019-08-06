#if !defined(COMM_H)
#define COMM_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKNAME "objstore.sock"
#define MAXBACKLOG 25
#define SYSCALL(r, c, e) \
  if ((r = c) == -1) {   \
    perror(e);           \
    exit(errno);         \
  }
#define CHECKNULL(r, c, e) \
  if ((r = c) == NULL) {   \
    perror(e);             \
    exit(errno);           \
  }

#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

typedef struct msg_t* msg_t;

extern int readn(long fd, void* buf, size_t size);

extern int writen(long fd, void* buf, size_t size);

#endif