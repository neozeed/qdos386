/* Native secondary-volume qualification through the existing R7 ownership ABI.
 * No raw port I/O, no boot-disk writes, no public filesystem ABI changes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"
#include "disktool.h"
#include "fat16.h"
static struct dt_request req;
static unsigned int writes,fail_at,partial;
static int dt(unsigned int op) { return (int)qtrap(QT_DISKTOOL,op,(unsigned int)&req,0); }
static int read_sector(void *ctx,unsigned int sector,unsigned char *b)
{
 int rc;(void)ctx;if(sector>=req.sectors)return -1;
 req.lba=sector;rc=dt(DT_READ);if(rc)return rc;
 memcpy(b,req.data,512);return 0;
}
static int write_sector(void *ctx,unsigned int sector,const unsigned char *b)
{
 unsigned int take=512;int rc,fail;
 (void)ctx;if(sector>=req.sectors)return -1;
 writes++;fail=fail_at==writes;if(fail)take=partial;
 printf("NWRITE %u LBA %u BYTES %u ERROR %u\n",writes,sector,take,fail);
 req.lba=sector;
 if(take<512) {rc=dt(DT_READ);if(rc)return rc;}
 if(take) {
  memcpy(req.data,b,take);rc=dt(DT_WRITE);if(rc)return rc;
 }
 return fail?-1:0;
}
extern int mutation_test(struct fat16_vol *,const char *,unsigned int);
#ifdef QDOS_F32_EMBED
int native_mutation_run(int argc,char **argv)
#else
int main(int argc,char **argv)
#endif
{
 struct fat16_vol v;struct fat16_file f;unsigned int hint=70010;
 const char *scenario="exercise";int rc,release,flush,post;
 if(argc>1)scenario=argv[1];if(argc>2)hint=(unsigned int)atoi(argv[2]);
 if(argc>3)fail_at=(unsigned int)atoi(argv[3]);if(argc>4)partial=(unsigned int)atoi(argv[4]);
 if(partial>512)return 2;
 memset(&req,0,sizeof req);req.unit=1;rc=dt(DT_INFO);
 if(rc||req.flags&DT_BOOT||req.sectors<65536u) {printf("F32N refused disk rc=%d\n",rc);return 2;}
 rc=dt(DT_CLAIM);if(rc){printf("F32N claim=%d\n",rc);return 2;}
 if(!strcmp(scenario,"leaveclaim")){puts("F32N PASS leaving claim for process-owner cleanup");return 0;}
 memset(&v,0,sizeof v);rc=fat16_mount(&v,read_sector,0,req.sectors);
 printf("F32N mount=%d sectors=%u\n",rc,req.sectors);
 if(!rc&&v.fat_bits!=32u)rc=FAT_ERR_TYPE;
 if(!rc) {
  fat16_set_writer(&v,write_sector);
  rc=mutation_test(&v,scenario,hint);
  if(fail_at) {
   memset(&f,0,sizeof f);f.vol=&v;f.mode=FAT_O_WRITE;
   post=fat16_flush(&f);
   printf("F32N injected rc=%d flush=%d writes=%u state=%d\n",rc,post,writes,v.wstate);
   rc=(rc==FAT_ERR_WIO&&post==FAT_ERR_WIO&&writes==fail_at&&v.wstate==-2)?0:1;
  }
 }
 flush=dt(DT_FLUSH);release=dt(DT_RELEASE);
 printf("F32N %s rc=%d flush=%d release=%d writes=%u\n",(rc||flush||release)?"FAIL":"PASS",rc,flush,release,writes);
 return (rc||flush||release)?1:0;
}
