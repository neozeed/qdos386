#include "aggregate.h"
#include <stdarg.h>
unsigned int sum1(struct S1 a) { unsigned int i,s=0;for(i=0;i<1;i++)s+=a.b[i];return s; }
struct S1 ret1(unsigned int seed) { struct S1 a;unsigned int i;for(i=0;i<1;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix1(struct S1 a,unsigned int l,struct S1 b,unsigned long r,const unsigned int *p) { return sum1(a)+sum1(b)+l+(unsigned int)r+*p; }
unsigned int rec1(struct S1 a,unsigned int d) { return d?sum1(a)+rec1(a,d-1u):sum1(a); }
unsigned int sum2(struct S2 a) { unsigned int i,s=0;for(i=0;i<2;i++)s+=a.b[i];return s; }
struct S2 ret2(unsigned int seed) { struct S2 a;unsigned int i;for(i=0;i<2;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix2(struct S2 a,unsigned int l,struct S2 b,unsigned long r,const unsigned int *p) { return sum2(a)+sum2(b)+l+(unsigned int)r+*p; }
unsigned int rec2(struct S2 a,unsigned int d) { return d?sum2(a)+rec2(a,d-1u):sum2(a); }
unsigned int sum3(struct S3 a) { unsigned int i,s=0;for(i=0;i<3;i++)s+=a.b[i];return s; }
struct S3 ret3(unsigned int seed) { struct S3 a;unsigned int i;for(i=0;i<3;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix3(struct S3 a,unsigned int l,struct S3 b,unsigned long r,const unsigned int *p) { return sum3(a)+sum3(b)+l+(unsigned int)r+*p; }
unsigned int rec3(struct S3 a,unsigned int d) { return d?sum3(a)+rec3(a,d-1u):sum3(a); }
unsigned int sum4(struct S4 a) { unsigned int i,s=0;for(i=0;i<4;i++)s+=a.b[i];return s; }
struct S4 ret4(unsigned int seed) { struct S4 a;unsigned int i;for(i=0;i<4;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix4(struct S4 a,unsigned int l,struct S4 b,unsigned long r,const unsigned int *p) { return sum4(a)+sum4(b)+l+(unsigned int)r+*p; }
unsigned int rec4(struct S4 a,unsigned int d) { return d?sum4(a)+rec4(a,d-1u):sum4(a); }
unsigned int sum5(struct S5 a) { unsigned int i,s=0;for(i=0;i<5;i++)s+=a.b[i];return s; }
struct S5 ret5(unsigned int seed) { struct S5 a;unsigned int i;for(i=0;i<5;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix5(struct S5 a,unsigned int l,struct S5 b,unsigned long r,const unsigned int *p) { return sum5(a)+sum5(b)+l+(unsigned int)r+*p; }
unsigned int rec5(struct S5 a,unsigned int d) { return d?sum5(a)+rec5(a,d-1u):sum5(a); }
unsigned int sum8(struct S8 a) { unsigned int i,s=0;for(i=0;i<8;i++)s+=a.b[i];return s; }
struct S8 ret8(unsigned int seed) { struct S8 a;unsigned int i;for(i=0;i<8;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix8(struct S8 a,unsigned int l,struct S8 b,unsigned long r,const unsigned int *p) { return sum8(a)+sum8(b)+l+(unsigned int)r+*p; }
unsigned int rec8(struct S8 a,unsigned int d) { return d?sum8(a)+rec8(a,d-1u):sum8(a); }
unsigned int sum12(struct S12 a) { unsigned int i,s=0;for(i=0;i<12;i++)s+=a.b[i];return s; }
struct S12 ret12(unsigned int seed) { struct S12 a;unsigned int i;for(i=0;i<12;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix12(struct S12 a,unsigned int l,struct S12 b,unsigned long r,const unsigned int *p) { return sum12(a)+sum12(b)+l+(unsigned int)r+*p; }
unsigned int rec12(struct S12 a,unsigned int d) { return d?sum12(a)+rec12(a,d-1u):sum12(a); }
unsigned int sum16(struct S16 a) { unsigned int i,s=0;for(i=0;i<16;i++)s+=a.b[i];return s; }
struct S16 ret16(unsigned int seed) { struct S16 a;unsigned int i;for(i=0;i<16;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix16(struct S16 a,unsigned int l,struct S16 b,unsigned long r,const unsigned int *p) { return sum16(a)+sum16(b)+l+(unsigned int)r+*p; }
unsigned int rec16(struct S16 a,unsigned int d) { return d?sum16(a)+rec16(a,d-1u):sum16(a); }
unsigned int sum20(struct S20 a) { unsigned int i,s=0;for(i=0;i<20;i++)s+=a.b[i];return s; }
struct S20 ret20(unsigned int seed) { struct S20 a;unsigned int i;for(i=0;i<20;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix20(struct S20 a,unsigned int l,struct S20 b,unsigned long r,const unsigned int *p) { return sum20(a)+sum20(b)+l+(unsigned int)r+*p; }
unsigned int rec20(struct S20 a,unsigned int d) { return d?sum20(a)+rec20(a,d-1u):sum20(a); }
unsigned int sum32(struct S32 a) { unsigned int i,s=0;for(i=0;i<32;i++)s+=a.b[i];return s; }
struct S32 ret32(unsigned int seed) { struct S32 a;unsigned int i;for(i=0;i<32;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix32(struct S32 a,unsigned int l,struct S32 b,unsigned long r,const unsigned int *p) { return sum32(a)+sum32(b)+l+(unsigned int)r+*p; }
unsigned int rec32(struct S32 a,unsigned int d) { return d?sum32(a)+rec32(a,d-1u):sum32(a); }
unsigned int sum64(struct S64 a) { unsigned int i,s=0;for(i=0;i<64;i++)s+=a.b[i];return s; }
struct S64 ret64(unsigned int seed) { struct S64 a;unsigned int i;for(i=0;i<64;i++)a.b[i]=(unsigned char)(seed+i*3u);return a; }
unsigned int mix64(struct S64 a,unsigned int l,struct S64 b,unsigned long r,const unsigned int *p) { return sum64(a)+sum64(b)+l+(unsigned int)r+*p; }
unsigned int rec64(struct S64 a,unsigned int d) { return d?sum64(a)+rec64(a,d-1u):sum64(a); }
struct Nested nested(struct Nested a,unsigned int x) { a.l+=x;a.s=(unsigned short)(a.s+x);a.z+=x;return a; }
unsigned long scalars(signed char a,unsigned char b,short c,unsigned short d,int e,unsigned int f,long g,unsigned long h,const unsigned int *p)
{ return (a==-3 && b==253 && c==-1234 && d==65000u && e==-12345 && f==0x87654321u && g==-123456L && h==0xfedcba98UL && *p==17u)?0xabcdef01UL:0UL; }
unsigned int variadic(unsigned int n,...) { va_list ap;unsigned int s=0,i;va_start(ap,n);for(i=0;i<n;i++)s+=va_arg(ap,unsigned int);va_end(ap);return s; }
