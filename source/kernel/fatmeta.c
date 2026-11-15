#include "fatmeta.h"
/* No host ABI types, packed structures, libc, casts through disk buffers,
 * unchecked additions, or writes from the read-only scan. */
static void zero(unsigned char *p, unsigned int n) { while(n--) *p++=0; }
static void copy(unsigned char *d,const unsigned char *s,unsigned int n) { while(n--) *d++=*s++; }
static int equal(const unsigned char *a,const unsigned char *b,unsigned int n) { while(n--) if(*a++!=*b++) return 0; return 1; }
unsigned int fm_u16(const unsigned char *p,unsigned int o) { return p[o]|((unsigned int)p[o+1]<<8); }
unsigned int fm_u32(const unsigned char *p,unsigned int o) { return fm_u16(p,o)|(fm_u16(p,o+2)<<16); }
void fm_p16(unsigned char *p,unsigned int o,unsigned int v) { p[o]=(unsigned char)v;p[o+1]=(unsigned char)(v>>8); }
void fm_p32(unsigned char *p,unsigned int o,unsigned int v) { fm_p16(p,o,v);fm_p16(p,o+2,v>>16); }
int fm_read(struct fm_device *d,unsigned int l,unsigned char *p) { return l<d->sectors && d->read ? d->read(d->ctx,l,p) : -1; }
int fm_write(struct fm_device *d,unsigned int l,const unsigned char *p) { return l<d->sectors && d->write ? d->write(d->ctx,l,p) : -1; }
static int range(unsigned int start,unsigned int n,unsigned int total) { return n && start<total && n<=total-start; }
int fm_table_valid(unsigned int total,const struct fm_partition *p)
{
 unsigned int i,j,active=0;
 for(i=0;i<4;i++) {
  if(!p[i].type) { if(p[i].start || p[i].count || p[i].active) return -1; continue; }
  if(p[i].type!=4 && p[i].type!=6 && p[i].type!=14 && p[i].type!=11 && p[i].type!=12) return -1;
  if(p[i].active!=0 && p[i].active!=128) return -1;
  if(p[i].active) active++;
  if(!p[i].start || !range(p[i].start,p[i].count,total)) return -1;
  for(j=0;j<i;j++) if(p[j].type && p[i].start<p[j].start+p[j].count && p[j].start<p[i].start+p[i].count) return -1;
 }
 return active<=1 ? 0 : -1;
}
int fm_mbr(const unsigned char *b,unsigned int total,struct fm_partition *p)
{
 unsigned int i,o;
 if(b[510]!=85 || b[511]!=170) return -1;
 for(i=0;i<4;i++) { o=446+16*i; p[i].active=b[o];p[i].type=b[o+4];p[i].start=fm_u32(b,o+8);p[i].count=fm_u32(b,o+12); }
 return fm_table_valid(total,p);
}
void fm_table_encode(unsigned char *b,const struct fm_partition *p)
{
 unsigned int i,o;
 for(i=0;i<4;i++) {
  o=446+16*i;zero(b+o,16);
  if(p[i].type) { b[o]=(unsigned char)p[i].active;b[o+1]=b[o+5]=254;b[o+2]=b[o+3]=b[o+6]=b[o+7]=255;b[o+4]=(unsigned char)p[i].type;fm_p32(b,o+8,p[i].start);fm_p32(b,o+12,p[i].count); }
 }
 b[510]=85;b[511]=170;
}
static int vr(struct fm_volume *v,unsigned int l,unsigned char *p)
{ return l<v->count ? fm_read(v->dev,v->start+l,p) : -1; }
static int vw(struct fm_volume *v,unsigned int l,const unsigned char *p)
{ return l<v->count ? fm_write(v->dev,v->start+l,p) : -1; }
int fm_mount(struct fm_volume *v,struct fm_device *d,unsigned int start,unsigned int count)
{
 unsigned int total,roots,fatend,flags,bits,spf,spc,res,nf,entries;
 unsigned char *b=v->boot;
 if(!range(start,count,d->sectors) || fm_read(d,start,b)) return -1;
 if(fm_u16(b,510)!=0xaa55 || fm_u16(b,11)!=512) return -1;
 spc=b[13];res=fm_u16(b,14);nf=b[16];entries=fm_u16(b,17);
 if(!spc || spc>128 || (spc&(spc-1)) || !res || (nf!=1 && nf!=2)) return -1;
 total=fm_u16(b,19);if(!total) total=fm_u32(b,32);
 if(!total || total>count) return -1;
 spf=fm_u16(b,22);bits=spf ? 16 : 32;if(!spf) spf=fm_u32(b,36);
 roots=(entries*32u+511)/512;
 if(!spf || res>=total || spf>(total-res)/nf) return -1;
 fatend=res+nf*spf;if(roots>=total-fatend) return -1;
 v->clusters=(total-fatend-roots)/spc;
 if(bits==16) { if(!entries || v->clusters<4085 || v->clusters>=65525 || spf<((v->clusters+2)*2u+511)/512) return -1; }
 else { if(entries || fm_u16(b,19) || fm_u16(b,42) || v->clusters<65525 || v->clusters>0x0fffffedu || spf<(v->clusters+129u)/128u) return -1; }
 v->fat_cached=0xffffffffu;v->dev=d;v->start=start;v->count=total;v->bits=bits;v->spc=spc;v->reserved=res;v->nfats=nf;v->spf=spf;v->data=fatend+roots;v->root_entries=entries;
 v->active=0;v->mirrored=1;v->info=0;v->backup=0;
 if(bits==16) v->root=fatend;
 else {
  flags=fm_u16(b,40);if(flags&0xff70u) return -1;
  if(flags&128u) { v->mirrored=0;v->active=flags&15u;if(v->active>=nf) return -1; }
  v->root=fm_u32(b,44);v->info=fm_u16(b,48);v->backup=fm_u16(b,50);
  if(v->root<2 || v->root>v->clusters+1 || !v->info || v->info>=res || !v->backup || v->backup>=res || v->info>=res-v->backup) return -1;
 }
 return 0;
}
static int fat(struct fm_volume *v,unsigned int c,unsigned int *out)
{
 unsigned int width=v->bits/8, off, sector;
 if(c>=v->clusters+2) return -1;
 off=c*width;sector=v->reserved+v->active*v->spf+off/512;
 if(v->fat_cached!=sector) { if(vr(v,sector,v->fat_cache))return -1;v->fat_cached=sector; }
 *out=v->bits==16 ? fm_u16(v->fat_cache,off%512) : fm_u32(v->fat_cache,off%512)&0x0fffffffu;return 0;
}
static int eoc(struct fm_volume *v,unsigned int c) { return c>=(v->bits==16 ? 0xfff8u : 0x0ffffff8u); }
static void issue(struct fm_check *k,unsigned int kind,unsigned int c,const char *text)
{ k->errors++;if(k->issue) k->issue(k->ctx,kind,c,text); }
/* Bounded ownership map catches cycles, repeated directory visits, and cross-links.
 * Directory recursion is limited; exceeding the supported limit is fatal. */
static int chain(struct fm_volume *v,struct fm_check *k,unsigned int first,unsigned int id,unsigned int *length)
{
 unsigned int c=first,n=0,next;
 if(first<2 || first>v->clusters+1) { issue(k,1,first,"invalid first cluster");*length=0;return 0; }
 while(!eoc(v,c)) {
  if(c<2 || c>v->clusters+1) { issue(k,1,c,"invalid/free/reserved cluster in chain");break; }
  if(k->owners[c]) { issue(k,k->owners[c]==id?1:2,c,"cycle or cross-linked cluster");break; }
  k->owners[c]=id;n++;
  if(fat(v,c,&next)) return -1;
  c=next;
 }
 *length=n;return 0;
}
static unsigned int name_sum(const unsigned char *p)
{ unsigned int s=0,i;for(i=0;i<11;i++) s=(((s&1u)<<7)+(s>>1)+p[i])&255u;return s; }
static int directory(struct fm_volume *v,struct fm_check *k,unsigned int first,unsigned int parent,unsigned int depth);
static int entries(struct fm_volume *v,struct fm_check *k,unsigned char *b,unsigned int dir,unsigned int parent,unsigned int *idx,unsigned int *lfn,unsigned int *sum,unsigned int depth)
{
 unsigned int off,attr,c,size,n,id,ord,i;
 for(off=0;off<512;off+=32,(*idx)++) {
  unsigned char *p=b+off;
  if(!p[0]) { if(dir && *idx<2) issue(k,4,dir,"missing dot/dotdot entries");if(*lfn) issue(k,5,dir,"orphan long-name record");*lfn=0;return 1; }
  if(p[0]==229) { if(*lfn) issue(k,5,dir,"orphan long-name record");*lfn=0;continue; }
  attr=p[11];
  if(attr==15) {
   ord=p[0];if(ord&64u) { if(*lfn) issue(k,5,dir,"overlapping long-name sequences");*lfn=ord&31u;*sum=p[13]; }
   if(!*lfn || (ord&31u)!=*lfn || (ord&160u) || *lfn>20 || p[12] || fm_u16(p,26) || p[13]!=*sum) { issue(k,5,dir,"malformed long-name sequence");*lfn=0; }
   else { (*lfn)--;if(!*lfn) *lfn=32; }
   continue;
  }
  if(*lfn && (*lfn!=32 || *sum!=name_sum(p))) issue(k,5,dir,"long-name checksum/sequence mismatch");
  *lfn=0;
  if(attr&8u) { if(attr&16u) issue(k,4,dir,"invalid volume/directory attribute");continue; }
  c=fm_u16(p,26);if(v->bits==32)c|=fm_u16(p,20)<<16;
  size=fm_u32(p,28);
  if(p[0]=='.') {
   i=p[1]=='.'?2:1;
   if(!dir || (*idx!=(i==1?0u:1u)) || attr!=16 || size || c!=(i==1?dir:parent)) issue(k,4,dir,"invalid dot/dotdot entry");
   for(;i<11;i++)if(p[i]!=' ') { issue(k,4,dir,"malformed dot name");break; }
   continue;
  }
  if(dir && *idx<2) issue(k,4,dir,"missing dot/dotdot entry");
  if(attr&16u) {
   if(size || c<2) { issue(k,4,c,"invalid directory cluster or size");continue; }
   if(directory(v,k,c,dir,depth+1))return -1;
  } else {
   k->files++;id=k->files+k->dirs+1;n=0;
   if(c && chain(v,k,c,id,&n))return -1;
   if(n!=size/(v->spc*512)+(size%(v->spc*512)!=0))issue(k,7,c,"file size/chain mismatch");
  }
 }
 return 0;
}
static int directory(struct fm_volume *v,struct fm_check *k,unsigned int first,unsigned int parent,unsigned int depth)
{
 unsigned int c=first,n=0,id,sec,idx=0,lfn=0,sum=0,i,next;int rc;
 unsigned char b[512];
 if(depth>FM_MAX_DEPTH)return -1;
 k->dirs++;id=k->files+k->dirs+1;
 if(first) {
  if(first<2 || first>v->clusters+1) { issue(k,4,first,"directory outside volume");return 0; }
  if(k->owners[first]) { issue(k,2,first,"directory cross-link/loop");return 0; }
  if(chain(v,k,first,id,&n))return -1;
 } else n=(v->root_entries*32u+511)/512;
 for(i=0;i<n;i++) {
  if(first) {
   if(c<2 || c>v->clusters+1)return -1;
   sec=v->data+(c-2)*v->spc;
  } else sec=v->root+i;
  for(next=0;next<(first?v->spc:1u);next++) {
   if(vr(v,sec+next,b))return -1;
   rc=entries(v,k,b,depth?first:0,parent,&idx,&lfn,&sum,depth);if(rc)return rc<0?-1:0;
  }
  if(first && fat(v,c,&c))return -1;
 }
 if(lfn)issue(k,5,first,"unterminated long-name sequence");
 return 0;
}
int fm_scan(struct fm_volume *v,struct fm_check *k)
{
 unsigned int i,j,c,n,firstfree=0,declared;unsigned char b[512],other[512];
 if(v->clusters>FM_MAX_CLUSTERS || k->capacity<v->clusters+2 || !k->owners)return -1;
 v->fat_cached=0xffffffffu;k->errors=k->files=k->dirs=k->free_count=k->lost=0;
 for(i=0;i<v->clusters+2;i++) k->owners[i]=0;
 if(v->mirrored)for(j=1;j<v->nfats;j++)for(i=0;i<v->spf;i++) {
  if(vr(v,v->reserved+i,b)||vr(v,v->reserved+j*v->spf+i,other))return -1;
  if(!equal(b,other,512))issue(k,6,i,"FAT copies differ; authority ambiguous");
 }
 if(fat(v,0,&n))return -1;
 if((n&255u)!=v->boot[21] || !eoc(v,n))issue(k,10,0,"invalid FAT media entry");
 if(fat(v,1,&n))return -1;
 if((n&(v->bits==16?0x3fffu:0x03ffffffu))!=(v->bits==16?0x3fffu:0x03ffffffu))issue(k,10,1,"invalid reserved FAT entry");
 if(directory(v,k,v->bits==32?v->root:0,0,0))return -1;
 for(c=2;c<v->clusters+2;c++) {
  if(fat(v,c,&n))return -1;
  if(!n) { k->free_count++;if(!firstfree)firstfree=c; }
  else if(!k->owners[c] && n!=(v->bits==16?0xfff7u:0x0ffffff7u)) { k->lost++;issue(k,3,c,"lost allocated cluster; no automatic repair"); }
 }
 if(v->bits==32) {
  if(vr(v,v->backup,b))return -1;
  if(!equal(v->boot,b,512))issue(k,9,v->backup,"backup boot sector differs");
  for(j=0;j<2;j++) {
   i=v->info+(j?v->backup:0);
   if(vr(v,i,b))return -1;
   if(fm_u32(b,0)!=0x41615252u || fm_u32(b,484)!=0x61417272u || fm_u32(b,508)!=0xaa550000u)issue(k,8,i,"invalid FSInfo signatures");
   else {
    declared=fm_u32(b,488);
    if(declared!=0xffffffffu && declared!=k->free_count)issue(k,8,i,"FSInfo free count differs from scanned FAT");
    declared=fm_u32(b,492);
    if(declared!=0xffffffffu) {
     if(declared<2 || declared>v->clusters+1)issue(k,8,i,"FSInfo next-free hint outside volume");
     /* A valid-range hint is a search starting point, not a promise that
      * the hinted cluster is free (mkfs.fat may point at the root). */
    }
   }
  }
 }
 return 0;
}
int fm_fix_info(struct fm_volume *v,unsigned int free_count)
{
 unsigned char b[512];unsigned int i;
 if(v->bits!=32 || free_count>v->clusters || !v->dev->flush)return -1;
 for(i=0;i<2;i++) {
  zero(b,512);fm_p32(b,0,0x41615252u);fm_p32(b,484,0x61417272u);fm_p32(b,488,free_count);fm_p32(b,492,0xffffffffu);fm_p32(b,508,0xaa550000u);
  if(vw(v,v->info+(i?v->backup:0),b))return -1;
 }
 return v->dev->flush(v->dev->ctx);
}
int fm_format(struct fm_device *d,unsigned int start,unsigned int count,unsigned int bits)
{
 unsigned int spc,spf,res,roots,clusters,meta,i,j,need;unsigned char b[512],old[512];
 if(d->sectors>0x0fffffffu || !range(start,count,d->sectors) || !start || (bits!=16 && bits!=32) || count<8192 || !d->write || !d->flush)return -1;
 res=bits==32?32u:1u;roots=bits==32?0u:32u;spf=0;clusters=0;
 for(spc=1;spc<=128;spc*=2) {
  /* Conservative monotone FAT sizing; never shrink across a boundary. */
  spf=1;
  for(i=0;i<32;i++) {
   if(count<=res+roots || spf>(count-res-roots)/2)return -1;
   clusters=(count-res-roots-2*spf)/spc;
   need=(clusters+2+(512/(bits/8))-1)/(512/(bits/8));
   if(need<=spf)break;
   spf=need;
  }
  if(i==32)return -1;
  if((bits==16 && clusters>=4085 && clusters<65525) || (bits==32 && clusters>=65525 && clusters<=FM_MAX_CLUSTERS))break;
 }
 if(spc>128)return -1;
 meta=res+2*spf+roots;
 if(fm_read(d,start,old))return -1;
 /* Preserve existing same-type VBR jump/OEM and executable region. Refuse
  * unknown nonzero VBRs or type conversion, where code/BPB layouts conflict. */
 for(i=0;i<512;i++)if(old[i])break;
 if(i<512) {
  struct fm_volume previous;
  if(fm_mount(&previous,d,start,count) || previous.bits!=bits || previous.reserved!=res)return -1;
  if(bits==32)for(j=1;j<res;j++) {
   if(j==1 || j==6 || j==7)continue;
   if(fm_read(d,start+j,b))return -1;
   for(i=0;i<512;i++)if(b[i])return -1;
  }
 }
 zero(b,512);
 /* Clear VBR first: interrupted formatting is visibly unformatted, not clean.
  * No boot code is installed. All reserved/FAT/root metadata is initialized. */
 for(i=0;i<meta+(bits==32?spc:0u);i++)if(fm_write(d,start+i,b))return -1;
 for(j=0;j<2;j++) {
  zero(b,512);
  if(bits==16) { fm_p16(b,0,0xfff8u);fm_p16(b,2,0xffffu); }
  else { fm_p32(b,0,0x0ffffff8u);fm_p32(b,4,0x0fffffffu);fm_p32(b,8,0x0fffffffu); }
  if(fm_write(d,start+res+j*spf,b))return -1;
 }
 if(bits==32) {
  zero(b,512);fm_p32(b,0,0x41615252u);fm_p32(b,484,0x61417272u);fm_p32(b,488,clusters-1);fm_p32(b,492,3);fm_p32(b,508,0xaa550000u);
  if(fm_write(d,start+1,b)||fm_write(d,start+7,b))return -1;
 }
 if(d->flush(d->ctx))return -1;
 zero(b,512);b[0]=0xeb;b[1]=0xfe;b[2]=0x90;copy(b+3,(const unsigned char *)"QDOSR7  ",8);
 fm_p16(b,11,512);b[13]=(unsigned char)spc;fm_p16(b,14,res);b[16]=2;fm_p16(b,17,bits==16?512:0);
 if(count<65536 && bits==16)fm_p16(b,19,count);else fm_p32(b,32,count);
 b[21]=248;fm_p16(b,24,63);fm_p16(b,26,255);fm_p32(b,28,start);
 if(bits==16)fm_p16(b,22,spf);
 else { fm_p32(b,36,spf);fm_p32(b,44,2);fm_p16(b,48,1);fm_p16(b,50,6); }
 i=bits==16?36u:64u;b[i]=128;b[i+2]=41;fm_p32(b,i+3,d->identity);copy(b+i+7,(const unsigned char *)"NO NAME    ",11);copy(b+i+18,(const unsigned char *)(bits==16?"FAT16   ":"FAT32   "),8);fm_p16(b,510,0xaa55);
 if(fm_u16(old,510)==0xaa55) {
  copy(b,old,11);i=bits==16?62u:90u;copy(b+i,old+i,510u-i);
 }
 if(bits==32 && fm_write(d,start+6,b))return -1;
 if(fm_write(d,start,b))return -1;
 return d->flush(d->ctx);
}
