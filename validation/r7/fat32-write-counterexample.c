/* Host-only negative experiment. Never a guest utility or supported writer.
 * An explicitly disposable image demonstrates why enabling the historical
 * QDOS_APOLLO_FAT32_WRITE macro is unsafe without a complete writer port. */
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fat16.h"
static FILE *fp;
static unsigned int writes;
static int rd(void *ctx,unsigned int l,unsigned char *b)
{ (void)ctx;if(fseek(fp,(long)l*512,SEEK_SET))return -1;return fread(b,1,512,fp)==512?0:-1; }
static int wr(void *ctx,unsigned int l,const unsigned char *b)
{ (void)ctx;writes++;if(fseek(fp,(long)l*512,SEEK_SET))return -1;return fwrite(b,1,512,fp)==512?0:-1; }
int main(int argc,char **argv)
{
 struct fat16_vol v;struct fat16_file f;unsigned char b[512];unsigned int put;int rc;long size;
 struct stat st,lst;int fd;char resolved[4096];const char *path;
 if(argc!=4 || strcmp(argv[1],"--image") || strcmp(argv[3],"--test-disposable"))return 2;
 path=argv[2];
 if(strncmp(path,"/tmp/",5) || strlen(path)<18 || strcmp(path+strlen(path)-18,".r7-disposable.img") ||
    lstat(path,&lst) || !S_ISREG(lst.st_mode) || !realpath(path,resolved) || strcmp(path,resolved))return 2;
 fd=open(path,O_RDWR|O_NOFOLLOW);
 if(fd<0 || fstat(fd,&st) || !S_ISREG(st.st_mode) || st.st_nlink!=1 || st.st_dev!=lst.st_dev || st.st_ino!=lst.st_ino)return 2;
 fp=fdopen(fd,"r+b");if(!fp)return 2;fseek(fp,0,SEEK_END);size=ftell(fp);memset(&v,0,sizeof(v));
 rc=fat16_mount(&v,rd,0,(unsigned long)size/512);printf("mount=%d\n",rc);if(rc)return 3;
 fat16_set_writer(&v,wr);v.next_free=70010;
 rc=fat16_open_path(&v,0,"/UNSAFE.DAT",FATP_CI,FAT_O_READ|FAT_O_WRITE|FAT_O_CREATE|FAT_O_EXCL,&f);printf("open=%d\n",rc);if(rc)return 4;
 memset(b,0x5a,sizeof(b));rc=fat16_write(&f,b,sizeof(b),&put);printf("write=%d bytes=%u first=%u\n",rc,put,f.first);
 rc=fat16_flush(&f);printf("flush=%d sector-writes=%u\n",rc,writes);fclose(fp);return 0;
}
