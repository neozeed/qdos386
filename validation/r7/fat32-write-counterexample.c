/* Host-only negative experiment. Never a guest utility or supported writer.
 * An explicitly disposable image demonstrates why enabling the historical
 * QDOS_APOLLO_FAT32_WRITE macro is unsafe without a complete writer port. */
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
 if(argc!=2 || strncmp(argv[1],"/tmp/",5))return 2;
 fp=fopen(argv[1],"r+b");if(!fp)return 2;fseek(fp,0,SEEK_END);size=ftell(fp);memset(&v,0,sizeof(v));
 rc=fat16_mount(&v,rd,0,(unsigned long)size/512);printf("mount=%d\n",rc);if(rc)return 3;
 fat16_set_writer(&v,wr);v.next_free=70010;
 rc=fat16_open_path(&v,0,"/UNSAFE.DAT",FATP_CI,FAT_O_READ|FAT_O_WRITE|FAT_O_CREATE|FAT_O_EXCL,&f);printf("open=%d\n",rc);if(rc)return 4;
 memset(b,0x5a,sizeof(b));rc=fat16_write(&f,b,sizeof(b),&put);printf("write=%d bytes=%u first=%u\n",rc,put,f.first);
 rc=fat16_flush(&f);printf("flush=%d sector-writes=%u\n",rc,writes);fclose(fp);return 0;
}
