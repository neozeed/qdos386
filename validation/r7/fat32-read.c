/* Host qualification of the actual shared engine, not a reimplementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat16.h"
struct disk { FILE *fp; unsigned int reads, writes; };
static int read_sector(void *ctx, unsigned int lba, unsigned char *b)
{
 struct disk *d=(struct disk *)ctx;d->reads++;
 if(fseek(d->fp,(long)lba*512,SEEK_SET)) return -1;
 return fread(b,1,512,d->fp)==512 ? 0:-1;
}
static int refuse_write(void *ctx, unsigned int lba, const unsigned char *b)
{ struct disk *d=(struct disk *)ctx; (void)lba;(void)b;d->writes++;return -1; }
int main(int argc,char **argv)
{
 struct disk disk;struct fat16_vol v;struct fat16_file f;struct fat16_info info;
 struct fat16_dirent ent;unsigned char buf[257];unsigned int got,i,dir;unsigned long bytes=0,freecount;int rc,n=0;long sectors;FILE *out;
 if(argc!=4)return 2;
 memset(&disk,0,sizeof(disk));memset(&v,0,sizeof(v));disk.fp=fopen(argv[1],"rb");if(!disk.fp)return 2;
 fseek(disk.fp,0,SEEK_END);sectors=ftell(disk.fp)/512;
 rc=fat16_mount(&v,read_sector,&disk,(unsigned long)sectors);
 if(!strcmp(argv[2],"bad-mount")) {if(rc>=0||disk.writes)return 3;printf("PASS refusal mount=%d reads=%u writes=0\n",rc,disk.reads);return 0;}
 if(rc){printf("FAIL mount=%d\n",rc);return 4;}
 fat16_set_writer(&v,refuse_write);
 rc=fat16_open_path(&v,0,"/FRAG.BIN",FATP_CI,FAT_O_READ,&f);
 if(!strcmp(argv[2],"bad-file")){if(rc>=0||disk.writes)return 5;printf("PASS refusal file=%d reads=%u writes=0\n",rc,disk.reads);return 0;}
 if(rc)return 6;
 if(fat16_seek(&f,511)||fat16_read(&f,buf,257,&got)||got!=257)return 7;
 for(i=0;i<got;i++)if(buf[i]!=(unsigned char)((511+i)*37+19))return 8;
 if(fat16_seek(&f,0))return 9;
 out=fopen(argv[3],"wb");if(!out)return 10;
 do {rc=fat16_read(&f,buf,257,&got);if(rc)return 11;fwrite(buf,1,got,out);bytes+=got;}while(got);
 fclose(out);if(bytes!=1703)return 12;
 if(fat16_stat_path(&v,0,"/NEST/TINY.TXT",FATP_CI,&info)||info.size!=0)return 13;
 if(fat16_chdir_path(&v,0,"/NEST",FATP_CI,&dir)||dir!=17)return 14;
 if(fat16_getcwd(&v,dir,(char *)buf,sizeof(buf))<0||strcmp((char *)buf,"/NEST"))return 15;
 if(fat16_dir_list_open(&v,0,FATL_DIRS,&f))return 16;
 while((rc=fat16_dir_list_next(&f,&ent))>0)n++;
 if(rc||n!=3)return 17;
 if(fat16_dir_list_open(&v,19,FATL_DIRS,&f)||fat16_dir_list_next(&f,&ent)!=0)return 18;
 if(fat16_free_clusters(&v,&freecount))return 19;
 if(fat16_mkdir(&v,0,"/DENIED",FATP_CI)!=FAT_ERR_WPROT||disk.writes)return 20;
 printf("PASS FAT%u root=%u bytes=%lu free=%lu reads=%u writes=0\n",v.fat_bits,v.root_cluster,bytes,freecount,disk.reads);
 fclose(disk.fp);return 0;
}
