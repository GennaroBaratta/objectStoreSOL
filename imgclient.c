#include <clientLibrary.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int fdinput = open("img.jpg", O_RDWR);
  if (fdinput < 0)
    perror("fopen");

  struct stat st;
  if (stat("img.jpg", &st) == -1)
    perror("stat");

  void* img = malloc(st.st_size);

  int r;
  SYSCALL(r, readn(fdinput, img, st.st_size), "read");

  if (os_connect("ImgClient") == 0) {
    if (os_store("img1.jpg", img, st.st_size) == -1) {
      fprintf(stderr, "Error on os_store()");
    }
  }
  os_disconnect();
  /*

  int ret = remove("data/img1.jpg");
  if (ret < 0 && errno != ENOENT) {
    perror("remove in store");
    return -1;
  }
  int fd;
  SYSCALL(fd,
          open("data/img1.jpg", O_CREAT | O_RDWR | O_APPEND,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
          "open in store");
  int written = 0;
  SYSCALL(written, writen(fd, img, st.st_size), "write in store");
  close(fd);

  */
  if (img)
    free(img);
  return 0;
}