#include <clientLibrary.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int nstores = 0;
int nretrives = 0;
int ndeletes = 0;
int successes = 0;
int failures = 0;
size_t bytesStores = 0;
size_t bytesRetrives = 0;

int main() {
  int fdinput = open("img.jpg", O_RDWR);
  if (fdinput < 0)
    perror("fopen");

  struct stat st, stOut;
  if (stat("img.jpg", &st) == -1)
    perror("stat");

  void* img = malloc(st.st_size);

  int r;
  SYSCALL(r, readn(fdinput, img, st.st_size), "read");

  void* retrivedImg = NULL;
  if (os_connect("ImgClient") == 0) {
    nstores++;
    if (os_store("img1.jpg", img, st.st_size) == -1) {
      fprintf(stderr, "Error on os_store()");
      failures++;
    } else {
      successes++;
      bytesStores += st.st_size;
      nretrives++;
      if ((retrivedImg = os_retrive("img1.jpg")) == NULL) {
        fprintf(stderr, "Error on os_retrive()");
        failures++;
      } else {
        int fdoutput = open("imgOut.jpg", O_TRUNC | O_CREAT | O_RDWR | O_APPEND,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fdoutput < 0)
          perror("fopen");

        SYSCALL(r, writen(fdoutput, retrivedImg, st.st_size), "writen");
        if (stat("imgOut.jpg", &stOut) == -1) {
          perror("stat");
          failures++;
        } else {
          successes++;
          bytesRetrives += stOut.st_size;
          ndeletes++;
          if (os_delete("img1.jpg") == -1) {
            perror("delete");
            failures++;
          } else {
            successes++;
          }
        }
      }
    }
  }
  os_disconnect();

  if (img)
    free(img);

  if (retrivedImg)
    free(retrivedImg);

  printf(
      "---Client: %s\n---Stores: %d\n---Retrives: %d\n---Deletes: %"
      "d\n---Successes: %d\n---Failures: %d\n---Stored "
      "bytes: %zu\n---Retrived bytes: %zu\n\n",
      "ImgClient", nstores, nretrives, ndeletes, successes, failures,
      bytesStores, bytesRetrives);

  fflush(stdout);
  return 0;
}