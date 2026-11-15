/* Private disposable image population through the qualified shared engine.
 * Manifest: D<TAB>/directory or F<TAB>/path<TAB>/tmp/input-file, one per line. */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "fat16.h"
static int fd;
static int rd(void *ctx,unsigned int l,unsigned char *b)
{(void)ctx;return pread(fd,b,512,(off_t)l*512)==512?0:-1;}
static int wr(void *ctx,unsigned int l,const unsigned char *b)
{(void)ctx;return pwrite(fd,b,512,(off_t)l*512)==512?0:-1;}
int main(int argc,char **argv)
{
 struct stat st,old;struct fat16_vol v;struct fat16_file file;
 char real[4096],line[4096];unsigned char b[8192];FILE *manifest,*input;
 unsigned int put;size_t n;char *path,*source,*end;int rc,files=0,dirs=0;
 if(argc!=5||strcmp(argv[1],"--disposable")||strcmp(argv[3],"--manifest")||strncmp(argv[2],"/tmp/",5)||!strstr(argv[2],".disposable.img")||strncmp(argv[4],"/tmp/",5))return 2;
 if(lstat(argv[2],&old)||!S_ISREG(old.st_mode)||!realpath(argv[2],real)||strcmp(argv[2],real))return 2;
 fd=open(argv[2],O_RDWR|O_NOFOLLOW);
 if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||old.st_ino!=st.st_ino||old.st_dev!=st.st_dev)return 2;
 memset(&v,0,sizeof v);rc=fat16_mount(&v,rd,0,(unsigned long)(st.st_size/512));if(rc||v.fat_bits!=32)return 3;
 fat16_set_writer(&v,wr);v.next_free=70010;
 manifest=fopen(argv[4],"r");if(!manifest)return 2;
 while(fgets(line,sizeof line,manifest)) {
  end=strchr(line,'\n');if(!end)return 2;*end=0;
  if(line[1]!='\t'||line[2]!='/')return 2;
  path=line+2;source=strchr(path,'\t');if(source)*source++=0;
  if(line[0]=='D') {rc=fat16_mkdir(&v,0,path,FATP_CI);dirs++;}
  else if(line[0]=='F'&&source&&strncmp(source,"/tmp/",5)==0) {
   rc=fat16_open_path(&v,0,path,FATP_CI,FAT_O_READ|FAT_O_WRITE|FAT_O_CREATE|FAT_O_EXCL,&file);if(rc)return 4;
   input=fopen(source,"rb");if(!input)return 2;
   while((n=fread(b,1,sizeof b,input))!=0) {
    rc=fat16_write(&file,b,(unsigned int)n,&put);
    if(rc||put!=n){printf("FAIL write %s rc=%d put=%u\n",path,rc,put);return 4;}
   }
   if(ferror(input)||fclose(input))return 4;
   rc=fat16_flush(&file);files++;
  } else return 2;
  if(rc){printf("FAIL %s rc=%d\n",path,rc);return 4;}
 }
 if(ferror(manifest)||fclose(manifest))return 4;
 fsync(fd);close(fd);printf("PASS populate files=%d directories=%d\n",files,dirs);return 0;
}
