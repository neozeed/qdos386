/* Shared-engine qualification. C89; also compiled with the accepted GCC 1.40.
 * Host front-end below only opens explicitly disposable ordinary /tmp files. */
#include "fat16.h"
#include <stdio.h>
#include <string.h>
static unsigned char payload[10000], readback[10000];
static int ck(const char *what, int rc)
{
 printf("CHECK %s %d\n",what,rc);
 return rc;
}
#define TRY(label,expr) do { int result=(expr); if(ck(label,result)) return result; } while(0)
#define REQUIRE(label,expr) TRY(label,(expr)?0:-999)
static int open_new(struct fat16_vol *v,const char *name,struct fat16_file *f)
{ return fat16_open_path(v,0,name,FATP_CI,FAT_O_READ|FAT_O_WRITE|FAT_O_CREATE|FAT_O_EXCL,f); }
int mutation_test(struct fat16_vol *v, const char *test, unsigned int hint)
{
 struct fat16_file f,g; struct fat16_info info;
 unsigned int i,n,got,put,dir; unsigned long before,after;
 char name[32]; int rc;
 for(i=0;i<sizeof payload;i++)payload[i]=(unsigned char)(i*37u+19u);
 v->next_free=hint;
 if(!strcmp(test,"fragment")) {
  TRY("fragment-open",fat16_open_ex(v,"FRAG.BIN",FAT_O_READ|FAT_O_WRITE,&f));
  TRY("fragment-read",fat16_read(&f,readback,1703,&got));
  REQUIRE("fragment-content",got==1703&&!memcmp(payload,readback,1703));
  TRY("fragment-append",fat16_write(&f,payload+1703,512,&put));
  TRY("fragment-flush",fat16_flush(&f));
  TRY("fragment-shrink",fat16_truncate(&f,1000));
  TRY("fragment-extend",fat16_truncate(&f,2000));return 0;
 }
 if(!strcmp(test,"overflow")) {
  TRY("overflow-create",open_new(v,"/OVER.DAT",&f));
  f.pos=f.size=0xffffffffUL;
  rc=fat16_write(&f,payload,1,&put);
  REQUIRE("overflow-refused",rc==FAT_ERR_RANGE&&put==0);return 0;
 }
 if(!strcmp(test,"create")) {
  TRY("create",open_new(v,"/HIGH.DAT",&f));
  TRY("write",fat16_write(&f,payload,512,&put)); REQUIRE("count",put==512);
  REQUIRE("first",f.first==hint);
  TRY("flush",fat16_flush(&f));
  TRY("reopen",fat16_open_ex(v,"HIGH.DAT",FAT_O_READ,&g));
  TRY("read",fat16_read(&g,readback,512,&got));
  REQUIRE("contents",got==512&&!memcmp(payload,readback,512));
  return 0;
 }
 if(!strcmp(test,"full")) {
  TRY("create",open_new(v,"/FULL.DAT",&f));
  rc=fat16_write(&f,payload,512,&put);
  REQUIRE("full-error",rc==FAT_ERR_FULL&&put==0);
  TRY("full-flush",fat16_flush(&f));return 0;
 }
 if(!strcmp(test,"readonly")) {
  rc=open_new(v,"/RO.DAT",&f);
  REQUIRE("readonly",rc==FAT_ERR_WPROT);return 0;
 }
 TRY("free-before",fat16_free_clusters(v,&before));
 TRY("create",open_new(v,"/WORK.DAT",&f));
 TRY("write",fat16_write(&f,payload,9000,&put));REQUIRE("put9000",put==9000);
 TRY("flush",fat16_flush(&f));
 TRY("reopen",fat16_open_ex(v,"WORK.DAT",FAT_O_READ|FAT_O_WRITE,&g));
 TRY("read",fat16_read(&g,readback,9000,&got));REQUIRE("contents",got==9000&&!memcmp(payload,readback,9000));
 TRY("seek",fat16_seek(&g,123));
 TRY("overwrite",fat16_write(&g,payload+9000,17,&put));REQUIRE("overwrite-count",put==17);
 TRY("flush-overwrite",fat16_flush(&g));
 TRY("append-open",fat16_open_ex(v,"WORK.DAT",FAT_O_READ|FAT_O_WRITE|FAT_O_APPEND,&f));
 TRY("append",fat16_write(&f,payload+9000,137,&put));REQUIRE("append-count",put==137);
 TRY("flush-append",fat16_flush(&f));
 TRY("shrink",fat16_truncate(&f,700));
 TRY("extend",fat16_truncate(&f,2000));
 TRY("reopen-extended",fat16_open_ex(v,"WORK.DAT",FAT_O_READ,&g));
 TRY("read-extended",fat16_read(&g,readback,2000,&got));REQUIRE("extended-count",got==2000);
 for(i=0;i<2000;i++) {
  unsigned char expected=i>=700?0:(i>=123&&i<140?payload[9000+i-123]:payload[i]);
  if(readback[i]!=expected)return ck("extended-content",-999);
 }
 TRY("mkdir",fat16_mkdir(v,0,"/PARENT",FATP_CI));
 TRY("nested-mkdir",fat16_mkdir(v,0,"/PARENT/CHILD",FATP_CI));
 TRY("chdir",fat16_chdir_path(v,0,"/PARENT/CHILD",FATP_CI,&dir));
 rc=fat16_getcwd(v,dir,name,sizeof name);REQUIRE("getcwd",rc==13);
 REQUIRE("cwd",!strcmp(name,"/PARENT/CHILD"));
 TRY("move-file",fat16_rename_path(v,0,"/WORK.DAT","/PARENT/CHILD/FINAL.DAT",FATP_CI));
 TRY("stat-moved",fat16_stat_path(v,0,"/PARENT/CHILD/FINAL.DAT",FATP_CI,&info));REQUIRE("moved-size",info.size==2000);
 TRY("temporary-create",open_new(v,"/DELETE.DAT",&f));
 TRY("temporary-write",fat16_write(&f,payload,8193,&put));REQUIRE("temporary-count",put==8193);
 TRY("temporary-flush",fat16_flush(&f));
 TRY("truncate-zero",fat16_truncate(&f,0));
 TRY("rewrite-after-truncate",fat16_write(&f,payload,33,&put));
 TRY("temporary-flush2",fat16_flush(&f));
 TRY("rename-legacy",fat16_rename(v,"DELETE.DAT","REMOVE.DAT"));
 TRY("remove-legacy",fat16_remove(v,"REMOVE.DAT"));
 TRY("mkdir-empty",fat16_mkdir(v,0,"/VANISH",0));
 TRY("rmdir",fat16_rmdir(v,0,"/VANISH",0));
 /* Fill more than one cluster even at SPC=8, including root growth. */
 n=v->spc*16u+5u;
 for(i=0;i<n;i++) {
  sprintf(name,"F%07u",i);
  TRY("root-grow-create",fat16_open_ex(v,name,FAT_O_WRITE|FAT_O_CREATE|FAT_O_EXCL,&f));
  TRY("empty-flush",fat16_flush(&f));
 }
 TRY("free-after",fat16_free_clusters(v,&after));
 REQUIRE("allocation-consumed",after<before);
 printf("PASS exercise files=%u free-before=%lu free-after=%lu\n",n,before,after);
 return 0;
}
#ifndef QDOS_NATIVE_MUTATION
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
static int fd;static unsigned int writes,fail_at,partial,read_only;
static int rd(void *ctx,unsigned int l,unsigned char *b)
{ (void)ctx;return pread(fd,b,512,(off_t)l*512)==512?0:-1; }
static int wr(void *ctx,unsigned int l,const unsigned char *b)
{
 unsigned int take=512;int fail;
 (void)ctx;writes++;fail=read_only||writes==fail_at;
 if(fail)take=read_only?0:partial;
 printf("WRITE %u LBA %u BYTES %u RESULT %d\n",writes,l,take,fail?-1:0);
 if(take&&pwrite(fd,b,take,(off_t)l*512)!=(int)take)return -1;
 return read_only?-2:fail?-1:0;
}
int main(int argc,char **argv)
{
 struct stat st,lst;struct fat16_vol v;struct fat16_file f;
 char resolved[4096];const char *path;int rc,post=0;
 if(argc!=8||strcmp(argv[1],"--disposable"))return 2;
 path=argv[2];
 if(strncmp(path,"/tmp/",5)||!strstr(path,".disposable.img")||
    lstat(path,&lst)||!S_ISREG(lst.st_mode)||!realpath(path,resolved)||strcmp(path,resolved))return 2;
 fd=open(path,O_RDWR|O_NOFOLLOW);
 if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_dev!=lst.st_dev||st.st_ino!=lst.st_ino)return 2;
 fail_at=(unsigned int)strtoul(argv[5],0,0);partial=(unsigned int)strtoul(argv[6],0,0);
 read_only=(unsigned int)strtoul(argv[7],0,0);if(partial>512)return 2;
 memset(&v,0,sizeof v);rc=fat16_mount(&v,rd,0,(unsigned long)(st.st_size/512));
 printf("MOUNT %d\n",rc);if(rc){close(fd);return 3;}
 fat16_set_writer(&v,wr);
 rc=mutation_test(&v,argv[3],(unsigned int)strtoul(argv[4],0,0));
 if(v.wstate<0) {
  memset(&f,0,sizeof f);f.vol=&v;f.mode=FAT_O_WRITE;
  post=fat16_flush(&f);printf("LATCH flush=%d state=%d\n",post,v.wstate);
  if(post==0)rc=-999;
 }
 printf("RESULT rc=%d writes=%u\n",rc,writes);fsync(fd);close(fd);return rc?1:0;
}
#endif
