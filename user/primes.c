#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sol(int input_fd){
  int n;
  if (!read(input_fd, &n, 4)) return;
  printf("prime %d\n", n);
  int p[2];
  pipe(p);
  if (fork() == 0){
    // child
    close(p[1]);
    sol(p[0]);
    close(p[0]);
    exit(0);
  } else {
    // parent
    close(p[0]);
    int k;
    while (read(input_fd, &k, 4)){
      if (k%n != 0){
        write(p[1], &k, 4);
      }
    }
    close(p[1]);
    wait(0);
  }
}

int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  if (fork() == 0){
    close(p[1]);
    sol(p[0]);
    close(p[0]);
    exit(0);
  } else {
    close(p[0]);
    for (int i=2;i<=35;i++){
      write(p[1], &i, 4);
    }
    close(p[1]);
    wait(0);
    exit(0);
  }
}
