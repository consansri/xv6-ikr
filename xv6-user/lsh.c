#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "xv6-user/user.h"

char*
fmtname(char *name)
{
  static char buf[STAT_MAX_NAME+1];
  int len = strlen(name);

  // Return blank-padded name.
  if(len >= STAT_MAX_NAME)
    return name;
  memmove(buf, name, len);
  memset(buf + len, ' ', STAT_MAX_NAME - len);
  buf[STAT_MAX_NAME] = '\0';
  return buf;
}

void
ls(char *path)
{
  int fd;
  struct stat st;
  char *types[] = {
    [T_DIR]   "DIR ",
    [T_FILE]  "FILE",
  };

  char *sizes[] = {"", "kB", "MB", "GB"};

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type == T_DIR){
    while(readdir(fd, &st) == 1){
      printf("%s %s\t", fmtname(st.name), types[st.type]);
      uint64 dsize = st.size;
      int lvl;
      for(lvl = 0; dsize > 1024 && lvl < 4; lvl++){
        dsize = dsize >> 10;
      }
      printf("%l %s\n", dsize, sizes[lvl]);
    }
  } else {
    printf("%s %s\t", fmtname(st.name), types[st.type]);
    uint64 dsize = st.size;
    int lvl;
    for(lvl = 0; dsize > 1024 && lvl < 4; lvl++){
      dsize = dsize >> 10;
    }
    printf("%l %s\n", dsize, sizes[lvl]);

  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}


