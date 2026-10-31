/* Native QDOS helpers. GPL-3.0-or-later. */
#include "qlterm.h"
U16 be16(const U8 *p) {return (U16)((p[0]<<8)|p[1]);}
U32 be32(const U8 *p) {return ((U32)be16(p)<<16)|be16(p+2);}
void put16(U8 *p,U16 n) {p[0]=(U8)(n>>8);p[1]=(U8)n;}
void put32(U8 *p,U32 n) {put16(p,(U16)(n>>16));put16(p+2,(U16)n);}
int cfg_byte(int off) {return config_blob[off];}
int cfg_word(int off) {return be16(config_blob+off);}
void cfg_string(int off,char *out) {
    int n=be16(config_blob+off+2),max=be16(config_blob+off);
    if(n>max)n=max;memcpy(out,config_blob+off+4,n);out[n]=0;
}
void wait_frames(int n) {mt_susjb(-1,(short)n,NULL);}
/* TAS makes locks indivisible across the original's three QDOS jobs. */
int try_lock(volatile U8 *p) {
    U8 result;
    __asm__ volatile ("tas %1\n\tseq %0" : "=d"(result) : "m"(*p) : "cc","memory");
    return result!=0;
}
void lock(volatile U8 *p) {while(!try_lock(p))wait_frames(1);}
void unlock(volatile U8 *p) {*p=0;}
void aux(const char *s) {sd_clear(t.cmdwin,-1);io_sstrg(t.cmdwin,-1,(void *)s,(short)strlen(s));}
void file_error(int e) {aux("FILE ERROR: ");ut_err((long)e,t.cmdwin);wait_frames(150);}
void wait_media(const char *name) {
    char *sys;long v;
    if((name[0]=='R'||name[0]=='r')&&(name[1]=='A'||name[1]=='a')&&(name[2]=='M'||name[2]=='m'))return;
    wait_frames(100);mt_inf(&sys,&v);
    while(*(volatile U8 *)(sys+0xee))wait_frames(1);
}
static int has_device(const char *name) {
    char *sys,*link;long ver;
    U32 d;
    if(strlen(name)<5||name[4]!='_')return 0;
    mt_inf(&sys,&ver);
    /* SV_DDLST is $48; directory linkage CH_DRNAM is $24 from link. */
    link=*(char **)(sys+0x48);
    d=be32((const U8 *)name)&0xdfdfdf00UL;
    while(link) {
        if((be32((const U8 *)link+0x26)&0xdfdfdf00UL)==d)return 1;
        link=*(char **)link;
    }
    return 0;
}
chanid_t open_upload(char *name) {
    chanid_t ch,dir;char *sys,*def;long ver;
    U8 hdr[64];char full[QLT_NAME_MAX];int n,i=0,offset;
    ch=io_open(name,1);
    if(ch==-7&&!has_device(name)) {
        mt_inf(&sys,&ver);def=*(char **)(sys+0xb0);
        if(def) {
            n=be16((U8 *)def);
            if(n+(int)strlen(name)>42)return -12;
            memcpy(full,def+2,n);strcpy(full+n,name);strcpy(name,full);
            ch=io_open(name,1);
        }
    }
    if(ch<0)return ch;
    memset(hdr,0,sizeof(hdr));dir=io_open(name,4);
    if(dir>=0) {fs_headr(dir,-1,hdr,64);io_close(dir);}
    /* Network names n1_flp1_... have a second device prefix. */
    if(strlen(name)>3 && name[1]>='0'&&name[1]<='7'&&name[2]=='_')i=3;
    while(name[i]&&name[i]!='_')++i;
    if(name[i])++i;
    offset=i+be16(hdr+14);
    if(be16(hdr+14))++offset;
    if(offset>(int)strlen(name))offset=i;
    t.name_offset=offset;return ch;
}
chanid_t open_download(const char *name) {
    chanid_t ch;int n;char plain[QLT_NAME_MAX];
    if(strlen(name)>=QLT_NAME_MAX)return -12;
    strcpy(plain,name);name=plain;
    if(strlen(t.dldir)+strlen(name)>42)return -12;
    strcpy(t.filename,t.dldir);strcat(t.filename,name);t.name_offset=strlen(t.dldir);
    for(;;) {
        ch=io_open(t.filename,2);if(ch!=-8)break;
        n=strlen(t.filename);
        if(n>=5&&!memcmp(t.filename+n-5,"_dup",4))++t.filename[n-1];
        else {
            if(n+5>=QLT_NAME_MAX)return -12;
            strcat(t.filename,"_dup1");
        }
    }
    if(ch>=0)wait_media(t.filename);return ch;
}
