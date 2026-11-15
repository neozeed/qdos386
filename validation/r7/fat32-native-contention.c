/* Eight actual file-channel readers while the secondary FAT32 writer owns IDE1.
 * Reference FNV-1a is independently computed from the preserved PAT2.BIN. */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"
#include "r20w.h"
static unsigned char buf[256];
extern int native_mutation_run(int argc,char **argv);
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
   printf("F32SYS reader FAIL got=%d bytes=%lu hash=%lx\n",got,n,h);return 1;
  }
 }
 puts("F32SYS reader PASS");return 0;
}
int main(int argc,char **argv)
{
 struct qd_procinfo before,after;
 int kids[8],i,rc,bad=0;unsigned int heap;
 unsigned long start,elapsed;
 char *writer_args[3];
 if(argc>1&&!strcmp(argv[1],"reader"))return reader();
 puts("F32SYS begin eight readers plus native secondary FAT32 writer");
 for(i=0;i<8;i++)kids[i]=-1;
 writer_args[0]="F32SYS";writer_args[1]="exercise";writer_args[2]="70010";
 qd_yield(4);if(qd_proc_info(0,&before))return 1;heap=qd_free_bytes();
 qtrap(QT_R20_STAT,R20_STAT_DUMP,0,0);
 for(i=0;i<8;i++) {
  kids[i]=qd_spawn("/F32SYS.AOU","reader",7,QD_SPAWN_HELD,QD_SPAWN_CON_OWN);
  if(kids[i]<0){printf("F32SYS spawn reader %d rc=%d\n",i,kids[i]);bad=1;break;}
 }
 /* The public ABI permits eight unreaped children per parent. The parent
  * itself performs writes, so eight readers do not require a ninth child. */
 if(bad){for(i=0;i<8;i++)if(kids[i]>=0)qd_kill(kids[i]);puts("F32SYS spawn FAIL");return 1;}
 start=qd_ticks();
 for(i=0;i<8;i++)if(qd_job_activate(kids[i],2))bad=1;
 rc=native_mutation_run(3,writer_args);if(rc)bad=1;
 for(i=0;i<8;i++) {
  rc=qd_wait(kids[i],12000);printf("F32SYS child %d exit=%d\n",i,rc);
  if(rc){bad=1;if(rc<0)qd_kill(kids[i]);}
 }
 qd_yield(10);elapsed=qd_ticks()-start;
 if(qd_proc_info(0,&after))bad=1;
 printf("F32SYS resource frames=%lu/%lu heap=%u/%u live=%lu/%lu elapsed=%lu\n",before.pool_free,after.pool_free,heap,qd_free_bytes(),before.procs_live,after.procs_live,elapsed);
 if(before.pool_free!=after.pool_free||heap!=qd_free_bytes()||before.procs_live!=after.procs_live||!qd_heap_ok())bad=1;
 qtrap(QT_R20_STAT,R20_STAT_DUMP,0,0);
 printf("F32SYS %s\n",bad?"FAIL":"PASS");return bad?1:0;
}
