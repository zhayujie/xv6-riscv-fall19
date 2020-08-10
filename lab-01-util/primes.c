#include "kernel/types.h"
#include "user/user.h"

void
close_pipe(int *p) {
  close(p[0]);
  close(p[1]);
}

void
primes() {
  int n, p, len;
  int fd[2];

  // read from prev progress 
  if ((len = read(0, &n, sizeof(int))) <= 0 || n <= 0) {
    close(1);
    exit();
  }
  // write first prime to console
  printf("prime %d\n", n);
  
  pipe(fd);
  if (fork() == 0) {
    close(0);
    dup(fd[0]);
    close_pipe(fd);
    primes();
  } else {
    close(1);
    dup(fd[1]);
    close_pipe(fd);
    while ((len = read(0, &p, sizeof(int))) > 0 && p > 0) {
      if (p % n != 0) {
        write(1, &p, sizeof(int));
      }
    }
    if (len <= 0 || p <= 0) {
      close(1);
      exit();
    }
  } 
}

int
main(void) {
  int i;
  int fd[2];
  
  pipe(fd);
  if (fork() == 0) {
    close(0);
    dup(fd[0]);
    close_pipe(fd);
    primes();
  } else {
    close(1);
    dup(fd[1]);
    close_pipe(fd);
    for (i = 2; i <= 35; i++) {
      write(1, &i, sizeof(int));
    }
    close(1);
    wait();
  }
  exit();
}
