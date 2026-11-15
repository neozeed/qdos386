/* Eight actual file-channel readers and a public-channel writer on FAT32 WIN1.
 * Reference FNV-1a is independently computed from the preserved PAT2.BIN. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"
#include "r20w.h"
static unsigned char buf[256];
static int writer(void)
{
 int ch,i,k,rc;unsigned char out[256];
 for(k=0;k<32;k++) {
  ch=qd_open_ex("WIN1:/LIVE.DAT",QD_O_READ|QD_O_WRITE|QD_O_CREATE|QD_O_TRUNC);
  if(ch<0)return 1;
  for(i=0;i<256;i++)buf[i]=(unsigned char)(i*23+k);
  for(i=0;i<8;i++)if(qd_write(ch,buf,256)!=256)return 1;
  if(qd_close(ch))return 1;
  ch=qd_open("WIN1:/LIVE.DAT");if(ch<0)return 1;
  for(i=0;i<8;i++)if(qd_read(ch,out,256)!=256||memcmp(buf,out,256))return 1;
  if(qd_close(ch))return 1;
 }
 rc=qd_remove("WIN1:/LIVE.DAT");return rc?1:0;
}
static int reader(void)
{
 unsigned long h,n;int ch,got,i,round;
 for(round=0;round<2;round++) {
  ch=qd_open("WIN1:/PAT2.BIN");if(ch<0)return 1;
  h=2166136261UL;n=0;
  while((got=qd_read(ch,buf,sizeof buf))>0) {
   for(i=0;i<got;i++)h=(h^buf[i])*16777619UL;
   n+=(unsigned long)got;
  }
  if(qd_close(ch)||got<0||n!=1572864UL||h!=0xe5895dc5UL) {
   printf("F32LIVE reader FAIL got=%d bytes=%lu hash=%lx\n",got,n,h);return 1;
  }
 }
 puts("F32LIVE reader PASS");return 0;
}
int main(int argc,char **argv)
{
 struct qd_procinfo before,after;
 int kids[8],i,rc,bad=0;unsigned int heap;
 unsigned long start,elapsed;
 if(argc>1&&!strcmp(argv[1],"reader"))return reader();
 /* WIN0 is the preserved FAT16 recovery namespace on the combined disk. */
 rc=qd_open("WIN0:/HOME/HI.C");
 if(rc<0||qd_read(rc,buf,sizeof buf)<=0||qd_close(rc))return 1;
 rc=qd_open_ex("WIN0:/MUSTNOT.DAT",QD_O_WRITE|QD_O_CREATE);
 if(rc!=-80){printf("F32LIVE recovery protection FAIL rc=%d\n",rc);return 1;}
 puts("F32LIVE recovery read/protection PASS");
 puts("F32LIVE begin eight readers plus public FAT32 WIN1 writer");
 for(i=0;i<8;i++)kids[i]=-1;
 qd_yield(4);if(qd_proc_info(0,&before))return 1;heap=qd_free_bytes();
 qtrap(QT_R20_STAT,R20_STAT_DUMP,0,0);
 for(i=0;i<8;i++) {
  kids[i]=qd_spawn("/F32LIVE.AOU","reader",7,QD_SPAWN_HELD,QD_SPAWN_CON_OWN);
  if(kids[i]<0){printf("F32LIVE spawn reader %d rc=%d\n",i,kids[i]);bad=1;break;}
 }
 /* The public ABI permits eight unreaped children per parent. The parent
  * itself performs writes, so eight readers do not require a ninth child. */
 if(bad){for(i=0;i<8;i++)if(kids[i]>=0)qd_kill(kids[i]);puts("F32LIVE spawn FAIL");return 1;}
 start=qd_ticks();
 for(i=0;i<8;i++)if(qd_job_activate(kids[i],2))bad=1;
 rc=writer();if(rc)bad=1;
 for(i=0;i<8;i++) {
  rc=qd_wait(kids[i],12000);printf("F32LIVE child %d exit=%d\n",i,rc);
  if(rc){bad=1;if(rc<0)qd_kill(kids[i]);}
 }
 qd_yield(10);elapsed=qd_ticks()-start;
 if(qd_proc_info(0,&after))bad=1;
 printf("F32LIVE resource frames=%lu/%lu heap=%u/%u live=%lu/%lu elapsed=%lu\n",before.pool_free,after.pool_free,heap,qd_free_bytes(),before.procs_live,after.procs_live,elapsed);
 if(before.pool_free!=after.pool_free||heap!=qd_free_bytes()||before.procs_live!=after.procs_live||!qd_heap_ok())bad=1;
 qtrap(QT_R20_STAT,R20_STAT_DUMP,0,0);
 printf("F32LIVE %s\n",bad?"FAIL":"PASS");return bad?1:0;
}
