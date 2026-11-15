/* Host qualification of the REAL WIN1 open-file registry, over the FAT32 engine.
 * Only hardware/job context is supplied by the harness; no replacement fs policy. */
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../source/kernel/win1.c"
static int image_fd;
unsigned int job_current_id(void) { return 1; }
int job_lookup(unsigned int id) { return id==1?1:-1; }
static int image_read(void *ctx,unsigned int sector,unsigned char *b)
{ (void)ctx;return pread(image_fd,b,512,(off_t)sector*512)==512?0:-1; }
static int image_write(void *ctx,unsigned int sector,const unsigned char *b)
{ (void)ctx;return pwrite(image_fd,b,512,(off_t)sector*512)==512?0:-1; }
#define CHECK(s,e) do { if(!(e)){printf("FAIL %s\n",s);return 1;}printf("PASS %s\n",s); } while(0)
int main(int argc,char **argv)
{
 struct chan_object a,b,c;struct stat st,old;char resolved[4096];unsigned char buf[512];
 if(argc!=3||strcmp(argv[1],"--disposable")||strncmp(argv[2],"/tmp/",5)||!strstr(argv[2],".disposable.img"))return 2;
 if(lstat(argv[2],&old)||!S_ISREG(old.st_mode)||!realpath(argv[2],resolved)||strcmp(argv[2],resolved))return 2;
 image_fd=open(argv[2],O_RDWR|O_NOFOLLOW);
 if(image_fd<0||fstat(image_fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||old.st_ino!=st.st_ino||old.st_dev!=st.st_dev)return 2;
 CHECK("mount",fat16_mount(&vol,image_read,0,(unsigned long)(st.st_size/512))==0);
 mounted=1;fat16_set_writer(&vol,image_write);vol.next_free=70010;
 memset(&a,0,sizeof a);memset(&b,0,sizeof b);memset(&c,0,sizeof c);
 a.mode=CHAN_O_READ|CHAN_O_WRITE|CHAN_O_CREATE|CHAN_O_EXCL;b.mode=CHAN_O_READ;c.mode=CHAN_O_READ|CHAN_O_WRITE|CHAN_O_TRUNC;
 CHECK("writer-open",win1_open(&a,"WIN1:/LOCK.DAT")==0);
 memset(buf,0x5a,sizeof buf);CHECK("writer-write",win1_write(&a,buf,sizeof buf)==512);
 CHECK("reject-reader",win1_open(&b,"WIN1:/LOCK.DAT")==FAT_ERR_BUSY);
 CHECK("reject-truncator",win1_open(&c,"WIN1:/LOCK.DAT")==FAT_ERR_BUSY);
 CHECK("reject-delete",win1_fsop(CHAN_FS_REMOVE,"WIN1:/LOCK.DAT",0)==FAT_ERR_BUSY);
 CHECK("reject-rename",win1_fsop(CHAN_FS_RENAME,"WIN1:/LOCK.DAT","WIN1:/NEW.DAT")==FAT_ERR_BUSY);
 CHECK("flush",win1_ctl(&a,CHAN_CTL_FLUSH,0)==0);win1_close(&a);
 CHECK("first-reader",win1_open(&b,"WIN1:/LOCK.DAT")==0);a.mode=CHAN_O_READ;
 CHECK("second-reader",win1_open(&a,"WIN1:/LOCK.DAT")==0);
 CHECK("reject-writer-with-readers",win1_open(&c,"WIN1:/LOCK.DAT")==FAT_ERR_BUSY);
 CHECK("read-unchanged",win1_read(&b,buf,sizeof buf)==512&&buf[0]==0x5a&&buf[511]==0x5a);
 win1_close(&a);win1_close(&b);
 CHECK("writer-after-release",win1_open(&c,"WIN1:/LOCK.DAT")==0);
 CHECK("truncated",c.ff.size==0);CHECK("flush-truncated",win1_ctl(&c,CHAN_CTL_FLUSH,0)==0);win1_close(&c);
 CHECK("delete-after-release",win1_fsop(CHAN_FS_REMOVE,"WIN1:/LOCK.DAT",0)==0);
 fsync(image_fd);close(image_fd);return 0;
}
