#include <stdio.h>
#include "qdos386.h"
#include "disktool.h"
static struct dt_request r;
static int bad;
static int call(unsigned int op) { return (int)qtrap(QT_DISKTOOL,op,(unsigned int)&r,0); }
static void check(const char *s,int ok) { printf("DTABI %s %s\n",s,ok?"PASS":"FAIL");if(!ok)bad++; }
int main(void)
{
 unsigned int identity,count;
 r.unit=0;check("info-boot",call(DT_INFO)==0 && (r.flags&DT_BOOT));
 check("refuse-boot-claim",call(DT_CLAIM)==DT_ERR_LIVE);
 r.unit=1;check("info-secondary",call(DT_INFO)==0 && !r.flags);identity=r.identity;count=r.sectors;
 r.lba=count;check("bounds",call(DT_READ)==DT_ERR_RANGE);r.lba=0;
 r.identity^=1;check("identity",call(DT_READ)==DT_ERR_ID);r.identity=identity;
 check("no-unclaimed-write",call(DT_WRITE)==DT_ERR_BUSY);
 check("claim",call(DT_CLAIM)==0);check("read",call(DT_READ)==0 && r.data[510]==85 && r.data[511]==170);
 check("release",call(DT_RELEASE)==0);r.unit=2;check("unit",call(DT_INFO)==DT_ERR_ARG);
 check("bad-pointer",(int)qtrap(QT_DISKTOOL,DT_INFO,0,0)==DT_ERR_ARG);
 printf("DTABI %s bad=%d\n",bad?"FAIL":"PASS",bad);return bad?1:0;
}
