#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  if (argc < 2){
    printf("usage: xargs [COMMAND [INITIAL-ARGS]]");
    exit(0);
  }
  char *argv2[MAXARG];
  char buf[512];
  char *p = buf;
  for (int i=1;i<argc;i++){
    argv2[i-1] = argv[i];
  }
  argv2[argc] = 0;
  argv2[argc-1] = buf;
  while (read(0, p++, 1) > 0 && (p < buf + sizeof(buf))){
    if (*(p-1) == '\n'){
      *(p-1) = 0;
      p = buf;
      if (fork() == 0){
        exec(argv2[0], argv2);
      } else {
        wait(0);
      }
    }
  }
  if (p >= buf + sizeof(buf)){
    printf("argument too long\n");
    exit(0);
  }
}
