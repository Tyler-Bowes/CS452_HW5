#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define ERR(s) err(s,__FILE__,__LINE__)

static void err(char *s, char *file, int line) {
  fprintf(stderr,"%s:%d: %s\n",file,line,s);
  exit(1);
}

//have it work with 0 len tokens buflen tokens etc
// userspace representation

int main() {
  enum {max=100};
  char buf[max+1];
  int len;

  printf("PRINTED in tryscanner 1\n");

  int scanner=open("/dev/Scanner",O_RDWR);
  if (scanner<0) {
    printf("%d scanner value", scanner);
    ERR("open() failed");
  }
  printf("PRINTED in tryscanner 2\n");

  if (ioctl(scanner,0,0))
    ERR("ioctl() for separators failed");
  printf("PRINTED in tryscanner 3\n");

  if (write(scanner,":",1)!=1)
    ERR("write() of separators failed");
  printf("PRINTED in tryscanner 4\n");

  for (char *line; scanf("%m[^\n]\n",&line)!=EOF;) {
    len=strlen(line);
    if (len!=write(scanner,line,len))
      ERR("write() of data failed");
    free(line);
    while ((len=read(scanner,buf,max))>=0) {
      buf[len]=0;
      printf("%s%s",buf,(len ? "" : "\n"));
    }
  }

  close(scanner);
  return 0;
}
