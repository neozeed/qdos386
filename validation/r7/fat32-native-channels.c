/* Native public filesystem ABI and physical-sector cache regression.
 * Runs unchanged on the FAT16 recovery/system baseline and FAT32 system. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"
#include "r20w.h"
static unsigned char data[256],out[256];
#define CHECK(s,e) do { if(!(e)){printf("F32IO FAIL %s\n",s);return 1;} } while(0)
int main(int argc,char **argv)
{
 int ch,other,i,k,child;unsigned int pos;unsigned long invalid;
 struct qd_stat info;
 if(argc>1&&!strcmp(argv[1],"leave")) {
  ch=qd_open_ex("WIN1:/OWNEXIT.DAT",QD_O_WRITE|QD_O_CREATE|QD_O_EXCL);
  CHECK("owner-open",ch>=0);memset(data,0x6d,sizeof data);
  CHECK("owner-write1",qd_write(ch,data,256)==256);
  CHECK("owner-write2",qd_write(ch,data,256)==256);
  CHECK("owner-write3",qd_write(ch,data,188)==188);
  puts("F32IO owner leaves writer channel open");return 0;
 }
 invalid=(unsigned int)qtrap(QT_R20_STAT,R20_STAT_INVALIDATE,0,0);
 for(k=0;k<16;k++) {
  ch=qd_open_ex("WIN1:/CACHE.DAT",QD_O_READ|QD_O_WRITE|QD_O_CREATE|QD_O_TRUNC);CHECK("create",ch>=0);
  other=qd_open("WIN1:/CACHE.DAT");CHECK("exclusion",other==-84);
  for(i=0;i<256;i++)data[i]=(unsigned char)(i*19+k);
  CHECK("write",qd_write(ch,data,256)==256);CHECK("close",qd_close(ch)==0);
  ch=qd_open("WIN1:/CACHE.DAT");CHECK("reopen",ch>=0);
  CHECK("read",qd_read(ch,out,256)==256&&!memcmp(data,out,256));CHECK("read-close",qd_close(ch)==0);
 }
 ch=qd_open_ex("WIN1:/CACHE.DAT",QD_O_READ|QD_O_WRITE|QD_O_APPEND);CHECK("append-open",ch>=0);
 CHECK("append",qd_write(ch,data,256)==256);CHECK("shrink",qd_truncate(ch,123)==0);
 CHECK("extend",qd_truncate(ch,700)==0);CHECK("seek",qd_seek(ch,0)==0);
 for(pos=0;pos<700;) {
  unsigned int want=700-pos;if(want>256)want=256;
  CHECK("extended-read",qd_read(ch,out,want)==(int)want);
  for(i=0;i<(int)want;i++)CHECK("extended-content",out[i]==(pos+(unsigned int)i<123?data[pos+i]:0));
  pos+=want;
 }
 CHECK("close-extended",qd_close(ch)==0);
 CHECK("mkdir",qd_mkdir("WIN1:/F32DIR")==0);
 CHECK("rename",qd_rename("WIN1:/CACHE.DAT","WIN1:/F32DIR/FINAL.DAT")==0);
 CHECK("remove",qd_remove("WIN1:/F32DIR/FINAL.DAT")==0);CHECK("rmdir",qd_rmdir("WIN1:/F32DIR")==0);
 child=qd_spawn("/F32IO.AOU","leave",6,0,QD_SPAWN_CON_OWN);CHECK("spawn-owner",child>=0);
 CHECK("wait-owner",qd_wait(child,3000)==0);qd_yield(3);
 CHECK("owner-stat",qd_stat("WIN1:/OWNEXIT.DAT",&info)==0&&info.size==700);
 ch=qd_open("WIN1:/OWNEXIT.DAT");CHECK("owner-reopen",ch>=0);
 for(pos=0;pos<700;) {
  unsigned int want=700-pos;if(want>256)want=256;
  CHECK("owner-read",qd_read(ch,out,want)==(int)want);
  for(i=0;i<(int)want;i++)CHECK("owner-content",out[i]==0x6d);
  pos+=want;
 }
 CHECK("owner-close",qd_close(ch)==0);
 CHECK("invalidation",(unsigned int)qtrap(QT_R20_STAT,R20_STAT_INVALIDATE,0,0)>invalid);
 CHECK("heap",qd_heap_ok());qtrap(QT_R20_STAT,R20_STAT_DUMP,0,0);
 puts("F32IO PASS public channels, close, owner cleanup, cache invalidation");return 0;
}
