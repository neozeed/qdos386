/* Native disk tools and host test adapter. C89, same metadata engine in both.
 * Host adapter is for disposable /tmp ordinary files only, never real disks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fatmeta.h"
#include "disktool.h"
#ifndef TOOL_MODE
#define TOOL_MODE 0
#endif
#ifdef DISKTOOLS_HOST
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
static int hostfd=-1;
static unsigned int host_writes,fail_after=0xffffffffu;
static int rd(void *ctx,unsigned int l,unsigned char *b)
{ (void)ctx;return pread(hostfd,b,512,(off_t)l*512)==512 ? 0:-1; }
static int wr(void *ctx,unsigned int l,const unsigned char *b)
{ (void)ctx;if(host_writes++>=fail_after)return -1;return pwrite(hostfd,b,512,(off_t)l*512)==512 ? 0:-1; }
static int flush(void *ctx) { (void)ctx;return fsync(hostfd); }
#else
#include "qdos386.h"
static struct dt_request req;
static int trap(unsigned int op) { return (int)qtrap(QT_DISKTOOL,op,(unsigned int)&req,0); }
static int rd(void *ctx,unsigned int l,unsigned char *b)
{ int rc;(void)ctx;req.lba=l;rc=trap(DT_READ);if(!rc)memcpy(b,req.data,512);return rc; }
static int wr(void *ctx,unsigned int l,const unsigned char *b)
{ (void)ctx;req.lba=l;memcpy(req.data,b,512);return trap(DT_WRITE); }
static int flush(void *ctx) { (void)ctx;return trap(DT_FLUSH); }
#endif
static struct fm_device dev;
static unsigned int live,claimed,testauth,partno=1,repair;
static char identity[80];
static int claim(void)
{
 if(live) { puts("REFUSED: live mounted boot/system disk");return -1; }
#ifndef DISKTOOLS_HOST
 if(trap(DT_CLAIM)) { puts("REFUSED: device busy or identity changed");return -1; }
#endif
 claimed=1;return 0;
}
static void release(void)
{
#ifndef DISKTOOLS_HOST
 if(claimed)trap(DT_RELEASE);
#endif
 claimed=0;
}
static int line(char *b,unsigned int n)
{
 unsigned int k=0;int c,overflow=0;
 for(;;) {
  c=getchar();
  /* The QDOS raw console returns Ctrl+D/Z as bytes, not stdio EOF. */
  if(c==EOF || c==4 || c==26) {
#ifndef DISKTOOLS_HOST
   putchar('\n');fflush(stdout);
#endif
   b[0]=0;return 0;
  }
  if(c=='\r' || c=='\n')break;
  if(c==8 || c==127) {
   if(k) {
    k--;
#ifndef DISKTOOLS_HOST
    fputs("\b \b",stdout);fflush(stdout);
#endif
   }
   continue;
  }
  if(c<32 || c>126)continue;
  if(k+1<n) {
   b[k++]=(char)c;
#ifndef DISKTOOLS_HOST
   putchar(c);fflush(stdout);
#endif
  } else overflow=1;
 }
#ifndef DISKTOOLS_HOST
 putchar('\n');fflush(stdout);
#endif
 b[k]=0;return !overflow;
}
static int authorize(const char *action)
{
 char answer[120],expected[120];
 printf("%s: target %s, %u sectors (512 bytes each).\n",action,identity,dev.sectors);
 puts("DESTRUCTIVE metadata update. No secure erase, journaling or power-loss atomicity.");
 if(live) { puts("REFUSED: live mounted boot/system disk");return 0; }
 if(testauth) { puts("Explicit disposable-image test authorization");return claim()==0; }
 sprintf(expected,"WRITE %s",identity);printf("Type exactly '%s' to commit (Enter cancels): ",expected);fflush(stdout);
 if(!line(answer,sizeof(answer)) || strcmp(answer,expected)) { puts("Cancelled; no writes");return 0; }
 return claim()==0;
}
static int number(const char *s,unsigned int *n)
{
 unsigned int x=0,d;if(!*s)return -1;
 while(*s) { if(*s<'0'||*s>'9')return -1;d=(unsigned int)(*s++-'0');if(x>(0xffffffffu-d)/10)return -1;x=x*10+d; }
 *n=x;return 0;
}
static void inventory(const struct fm_partition *p)
{
 unsigned int i,used=1;
 printf("Disk %s: %u sectors, %u MiB, %s\n",identity,dev.sectors,dev.sectors/2048,live?"LIVE: writes refused":"unmounted maintenance target");
 puts("Part  Start       Sectors     Type Active Alignment(2048)");
 for(i=0;i<4;i++) { printf("%u     %-11u %-11u %02x   %s    %s\n",i+1,p[i].start,p[i].count,p[i].type,p[i].active?"yes":"no",p[i].start%2048?"unaligned":"aligned");used+=p[i].count; }
 printf("Unpartitioned: %u sectors (includes alignment/loader gaps; not automatically usable)\n",dev.sectors-used);
}
static int fdisk_run(int argc,char **argv,int at)
{
 unsigned char original[512],proposed[512],check[512];struct fm_partition p[4];
 unsigned int n,start,count,bits,i;int dirty=0;char input[160],cmd[24],a[24],b[24],c[24],d[24];
 if(fm_read(&dev,0,original))return 3;
 if(fm_mbr(original,dev.sectors,p)) {
  /* Only an all-zero new MBR can be initialized; malformed/GPT/extended
   * existing tables are not interpreted as free space. */
  for(i=0;i<512;i++)if(original[i]) { puts("Unsupported/malformed MBR; inspect only, no changes");return 3; }
  memset(p,0,sizeof(p));puts("Blank disk: no partition table");
 }
 inventory(p);
 if(at==argc)return 0;
 if(strcmp(argv[at],"--edit"))return 3;
 puts("Stage commands: create N START COUNT 16|32; delete N; active N; show; write; quit");
 for(;;) {
  printf("FDISK> ");fflush(stdout);if(!line(input,sizeof(input)))break;
  cmd[0]=a[0]=b[0]=c[0]=d[0]=0;
  if(sscanf(input,"%23s %23s %23s %23s %23s",cmd,a,b,c,d)<1)continue;
  if(!strcmp(cmd,"quit"))break;
  if(!strcmp(cmd,"show")) { inventory(p);continue; }
  if(!strcmp(cmd,"write")) {
   if(!dirty) { puts("No staged changes");continue; }
   if(fm_table_valid(dev.sectors,p)) { puts("REFUSED: invalid/overlapping/out-of-range partitions or conflicting active flags");continue; }
   inventory(p);
   if(!authorize("Write partition table"))continue;
   if(fm_read(&dev,0,check)||memcmp(check,original,512)) { release();puts("REFUSED: MBR changed since inspection");return 3; }
   memcpy(proposed,original,512);fm_table_encode(proposed,p);
   if(fm_write(&dev,0,proposed)||flush(0)) { release();puts("I/O FAILURE: possible partial update; external recovery may be needed");return 3; }
   release();puts("Partition table written; boot code and disk signature preserved");return 0;
  }
  if(number(a,&n)||n<1||n>4) { puts("Invalid partition number");continue; }n--;
  if(!strcmp(cmd,"delete")) { memset(&p[n],0,sizeof(p[n]));dirty=1; }
  else if(!strcmp(cmd,"active")) { if(!p[n].type) { puts("Empty partition");continue; }for(i=0;i<4;i++)p[i].active=0;p[n].active=128;dirty=1; }
  else if(!strcmp(cmd,"create")) {
   if(p[n].type || number(b,&start)||number(c,&count)||number(d,&bits)||(bits!=16&&bits!=32)) { puts("Invalid create arguments or occupied slot");continue; }
   p[n].start=start;p[n].count=count;p[n].type=bits==16?14:12;p[n].active=0;
   if(fm_table_valid(dev.sectors,p)) { memset(&p[n],0,sizeof(p[n]));puts("REFUSED: overlap, overflow or out-of-disk extent");continue; }dirty=1;
  } else puts("Unknown command");
 }
 puts("Staged changes discarded; no writes");return 0;
}
static unsigned int classes[11];
static void finding(void *ctx,unsigned int kind,unsigned int cluster,const char *why)
{ (void)ctx;if(kind<11)classes[kind]++;if(kind<11 && classes[kind]>16)return;printf("Issue class %u at %u: %s\n",kind,cluster,why); }
static int check_run(struct fm_partition *p)
{
 struct fm_volume v;struct fm_check k;unsigned int i,unsafe=0;char answer[32];int rc;
 if(repair && claim())return 3;
 if(fm_mount(&v,&dev,p->start,p->count)) { puts("Unsupported or malformed FAT BPB");return 3; }
 printf("CHKDSK FAT%u: %u clusters, %u sectors/cluster; %s\n",v.bits,v.clusters,v.spc,repair?"interactive repair requested":"READ ONLY");
 if(v.clusters>FM_MAX_CLUSTERS) { puts("Volume exceeds bounded checker capacity");return 3; }
 memset(&k,0,sizeof(k));k.capacity=v.clusters+2;k.owners=(unsigned int *)malloc(k.capacity*sizeof(unsigned int));k.issue=finding;
 if(!k.owners) { puts("Insufficient memory; scan not performed");return 3; }
 rc=fm_scan(&v,&k);free(k.owners);
 if(rc) { puts("Fatal I/O/unsupported traversal; no repair attempted");return 3; }
 printf("Scan: files=%u directories=%u free=%u lost=%u errors=%u\n",k.files,k.dirs,k.free_count,k.lost,k.errors);
 if(!k.errors)return 0;
 if(!repair)return 1;
 for(i=1;i<11;i++)if(i!=8)unsafe+=classes[i];
 if(unsafe) { puts("Ambiguous/structural errors: automatic repair refused; no writes");return 1; }
 /* Both advisory copies form ONE repair finding of class FSInfo. A applies
  * only to that class. Never infer repair authority from program launch. */
 puts("Validated FAT scan: replace invalid advisory FSInfo with scanned free count and unknown next-free hint (both copies).");
 printf("Fix (Y/N/A/Q)? [N] ");fflush(stdout);
 if(!line(answer,sizeof(answer)) || (strcmp(answer,"Y") && strcmp(answer,"y") && strcmp(answer,"A") && strcmp(answer,"a"))) { puts("No further repairs; errors remain");return 1; }
 if(!authorize("Repair advisory FSInfo"))return 1;
 if(fm_fix_info(&v,k.free_count)) { release();puts("I/O FAILURE during repair; possible partial update; external recovery may be needed");return 3; }
 release();puts("Repaired FSInfo class only; structural metadata unchanged");return 2;
}
static void help(void)
{
 puts("FDISK --image TARGET [--edit] [--test-disposable]\nFORMAT --image TARGET --partition N --fat 16|32 [--test-disposable]\nCHKDSK --image TARGET --partition N [--repair] [--test-disposable]");
 puts("Native TARGET is ide0 or ide1. Host test TARGET must be a regular /tmp file.\nFDISK and CHKDSK default read-only. FORMAT and FDISK write require typed disk identity.\nCHKDSK repair prompts Fix (Y/N/A/Q)? default No; A means this FSInfo class only.\nExit: 0 clean/success, 1 errors unfixed/cancelled, 2 repaired, 3 fatal/unsupported.\nNo live boot/system disk writes; primary MBR only; 512-byte sectors; no GPT/extended.\nChecker limit: 262144 clusters, directory depth 32. No ambiguous repairs or secure erase.");
}
int main(int argc,char **argv)
{
 const char *image=0;int i,edit=-1,rc;unsigned int bits=0;unsigned char mbr[512];struct fm_partition p[4];
 for(i=1;i<argc;i++) {
  if(!strcmp(argv[i],"--help")) { help();return 0; }
  else if(!strcmp(argv[i],"--image") && i+1<argc)image=argv[++i];
  else if(!strcmp(argv[i],"--partition") && i+1<argc) { if(number(argv[++i],&partno)||partno<1||partno>4)return 3; }
  else if(!strcmp(argv[i],"--fat") && i+1<argc) { if(number(argv[++i],&bits))return 3; }
  else if(!strcmp(argv[i],"--repair"))repair=1;
  else if(!strcmp(argv[i],"--test-disposable"))testauth=1;
  else if(!strcmp(argv[i],"--edit"))edit=i;
  else { help();return 3; }
 }
 if(!image) { help();return 3; }
#ifdef DISKTOOLS_HOST
 {
  struct stat st,lst;const char *f;char resolved[4096];int writing=(TOOL_MODE==1||edit>=0||repair);
  if(strncmp(image,"/tmp/",5) || lstat(image,&lst) || !S_ISREG(lst.st_mode) || !realpath(image,resolved) || strcmp(image,resolved)) { puts("REFUSED: canonical ordinary /tmp path required; no symlinks");return 3; }
  if(writing && (!testauth || strlen(image)<18 || strcmp(image+strlen(image)-18,".r7-disposable.img"))) { puts("Host writes require --test-disposable and .r7-disposable.img suffix");return 3; }
  hostfd=open(image,(writing?O_RDWR:O_RDONLY)|O_NOFOLLOW);
  if(hostfd<0 || fstat(hostfd,&st) || st.st_dev!=lst.st_dev || st.st_ino!=lst.st_ino || st.st_nlink!=1 || st.st_size<=0 || st.st_size%512 || st.st_size/512>0x0fffffffu) { puts("REFUSED: invalid image identity/geometry");return 3; }
  dev.sectors=(unsigned int)(st.st_size/512);dev.identity=(unsigned int)st.st_ino;
  sprintf(identity,"HOST-%08x-%u",dev.identity,dev.sectors);
  f=getenv("QDOS_DISK_TEST_FAIL_AFTER");if(f && number(f,&fail_after))return 3;
 }
#else
 if(strcmp(image,"ide0") && strcmp(image,"ide1")) { puts("Native target must be ide0 or ide1");return 3; }
 req.unit=(unsigned int)(image[3]-'0');if(trap(DT_INFO)) { puts("Cannot identify supported disk");return 3; }
 dev.identity=req.identity;dev.sectors=req.sectors;live=req.flags&DT_BOOT;
 sprintf(identity,"IDE%u-%08x-%u",req.unit,dev.identity,dev.sectors);
#endif
 dev.read=rd;dev.write=wr;dev.flush=flush;
 if(TOOL_MODE==0)rc=fdisk_run(argc,argv,edit>=0?edit:argc);
 else {
  if(fm_read(&dev,0,mbr)||fm_mbr(mbr,dev.sectors,p)||!p[partno-1].type) { puts("Invalid/unsupported MBR or empty partition");return 3; }
  if(TOOL_MODE==2)rc=check_run(&p[partno-1]);
  else {
   printf("FORMAT partition %u: start=%u sectors=%u FAT%u\n",partno,p[partno-1].start,p[partno-1].count,bits);
   if(bits!=16 && bits!=32) { puts("Specify --fat 16 or 32");return 3; }
   if((bits==16 && p[partno-1].type!=4 && p[partno-1].type!=6 && p[partno-1].type!=14) || (bits==32 && p[partno-1].type!=11 && p[partno-1].type!=12)) { puts("Partition type does not match requested FAT");return 3; }
   if(!authorize("Format partition"))return 1;
   { unsigned char check[512];
    if(fm_read(&dev,0,check) || memcmp(check,mbr,512)) {
     release();puts("REFUSED: MBR changed since inspection");return 3;
    }
   }
   rc=fm_format(&dev,p[partno-1].start,p[partno-1].count,bits);release();
   puts(rc?"FORMAT FAILED: unsupported geometry or I/O; possible partial update, external recovery may be needed":"FORMAT completed; data partition, no boot loader installed");rc=rc?3:0;
  }
 }
 release();
#ifdef DISKTOOLS_HOST
 close(hostfd);
#endif
 return rc;
}
