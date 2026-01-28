#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *fname){
  int fd;
  struct dirent de;
  struct stat st;
  char buf[512], *p;
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open directory %s\n", path);
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
    printf("find: path too long\n");
    close(fd);
    return;
  }
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      printf("find: cannot stat %s\n", buf);
      continue;
    }
    if (st.type == T_DIR){
      if (strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
        find(buf, fname);
    } else {
      if (strcmp(de.name, fname) == 0){
        printf("%s\n", buf);
      }
    }
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3){
    printf("usage: find [PATH] [FILE]\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}
