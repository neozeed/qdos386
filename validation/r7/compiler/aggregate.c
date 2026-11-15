#include "aggregate.h"

static unsigned int checks=0,failures=0;
static void out(const char *s) { unsigned int n=0;while(s[n])n++;probe_write(s,n); }
static void check(int b,const char *s) { checks++;if(!b){out("FAIL ");out(s);out("\n");failures++;} }
static unsigned int expected(unsigned int n,unsigned int seed) { unsigned int i,s=0;for(i=0;i<n;i++)s+=(seed+i*3u)&255u;return s; }
static void hex(unsigned int x) { char b[9];unsigned int i;for(i=0;i<8;i++)b[7-i]="0123456789abcdef"[(x>>(4*i))&15u];b[8]=0;out(b); }
static void test1(void) {
 struct S1 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S1)=sum1;struct S1 (*rp)(unsigned int)=ret1;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<1;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==1u,"size");check(sum1(a)==expected(1,3),"pass");
 check(fp(b)==expected(1,9),"function pointer");c=a;check(sum1(c)==expected(1,3),"assignment");
 { struct S1 *q=&c;*q=c; }check(sum1(c)==expected(1,3),"self assignment");
 c=ret1(9);check(sum1(c)==expected(1,9),"struct return");
 c=rp(3);check(sum1(c)==expected(1,3),"return function pointer");
 check(mix1(a,7,b,11UL,&p)==expected(1,3)+expected(1,9)+31u,"mixed argument positions");
 check(mix1(ret1(3),7,ret1(9),11UL,&p)==expected(1,3)+expected(1,9)+31u,"nested returns");
 check(rec1(a,3)==4u*expected(1,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 1 done\n");
}
static void test2(void) {
 struct S2 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S2)=sum2;struct S2 (*rp)(unsigned int)=ret2;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<2;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==2u,"size");check(sum2(a)==expected(2,3),"pass");
 check(fp(b)==expected(2,9),"function pointer");c=a;check(sum2(c)==expected(2,3),"assignment");
 { struct S2 *q=&c;*q=c; }check(sum2(c)==expected(2,3),"self assignment");
 c=ret2(9);check(sum2(c)==expected(2,9),"struct return");
 c=rp(3);check(sum2(c)==expected(2,3),"return function pointer");
 check(mix2(a,7,b,11UL,&p)==expected(2,3)+expected(2,9)+31u,"mixed argument positions");
 check(mix2(ret2(3),7,ret2(9),11UL,&p)==expected(2,3)+expected(2,9)+31u,"nested returns");
 check(rec2(a,3)==4u*expected(2,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 2 done\n");
}
static void test3(void) {
 struct S3 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S3)=sum3;struct S3 (*rp)(unsigned int)=ret3;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<3;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==3u,"size");check(sum3(a)==expected(3,3),"pass");
 check(fp(b)==expected(3,9),"function pointer");c=a;check(sum3(c)==expected(3,3),"assignment");
 { struct S3 *q=&c;*q=c; }check(sum3(c)==expected(3,3),"self assignment");
 c=ret3(9);check(sum3(c)==expected(3,9),"struct return");
 c=rp(3);check(sum3(c)==expected(3,3),"return function pointer");
 check(mix3(a,7,b,11UL,&p)==expected(3,3)+expected(3,9)+31u,"mixed argument positions");
 check(mix3(ret3(3),7,ret3(9),11UL,&p)==expected(3,3)+expected(3,9)+31u,"nested returns");
 check(rec3(a,3)==4u*expected(3,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 3 done\n");
}
static void test4(void) {
 struct S4 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S4)=sum4;struct S4 (*rp)(unsigned int)=ret4;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<4;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==4u,"size");check(sum4(a)==expected(4,3),"pass");
 check(fp(b)==expected(4,9),"function pointer");c=a;check(sum4(c)==expected(4,3),"assignment");
 { struct S4 *q=&c;*q=c; }check(sum4(c)==expected(4,3),"self assignment");
 c=ret4(9);check(sum4(c)==expected(4,9),"struct return");
 c=rp(3);check(sum4(c)==expected(4,3),"return function pointer");
 check(mix4(a,7,b,11UL,&p)==expected(4,3)+expected(4,9)+31u,"mixed argument positions");
 check(mix4(ret4(3),7,ret4(9),11UL,&p)==expected(4,3)+expected(4,9)+31u,"nested returns");
 check(rec4(a,3)==4u*expected(4,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 4 done\n");
}
static void test5(void) {
 struct S5 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S5)=sum5;struct S5 (*rp)(unsigned int)=ret5;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<5;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==5u,"size");check(sum5(a)==expected(5,3),"pass");
 check(fp(b)==expected(5,9),"function pointer");c=a;check(sum5(c)==expected(5,3),"assignment");
 { struct S5 *q=&c;*q=c; }check(sum5(c)==expected(5,3),"self assignment");
 c=ret5(9);check(sum5(c)==expected(5,9),"struct return");
 c=rp(3);check(sum5(c)==expected(5,3),"return function pointer");
 check(mix5(a,7,b,11UL,&p)==expected(5,3)+expected(5,9)+31u,"mixed argument positions");
 check(mix5(ret5(3),7,ret5(9),11UL,&p)==expected(5,3)+expected(5,9)+31u,"nested returns");
 check(rec5(a,3)==4u*expected(5,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 5 done\n");
}
static void test8(void) {
 struct S8 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S8)=sum8;struct S8 (*rp)(unsigned int)=ret8;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<8;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==8u,"size");check(sum8(a)==expected(8,3),"pass");
 check(fp(b)==expected(8,9),"function pointer");c=a;check(sum8(c)==expected(8,3),"assignment");
 { struct S8 *q=&c;*q=c; }check(sum8(c)==expected(8,3),"self assignment");
 c=ret8(9);check(sum8(c)==expected(8,9),"struct return");
 c=rp(3);check(sum8(c)==expected(8,3),"return function pointer");
 check(mix8(a,7,b,11UL,&p)==expected(8,3)+expected(8,9)+31u,"mixed argument positions");
 check(mix8(ret8(3),7,ret8(9),11UL,&p)==expected(8,3)+expected(8,9)+31u,"nested returns");
 check(rec8(a,3)==4u*expected(8,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 8 done\n");
}
static void test12(void) {
 struct S12 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S12)=sum12;struct S12 (*rp)(unsigned int)=ret12;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<12;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==12u,"size");check(sum12(a)==expected(12,3),"pass");
 check(fp(b)==expected(12,9),"function pointer");c=a;check(sum12(c)==expected(12,3),"assignment");
 { struct S12 *q=&c;*q=c; }check(sum12(c)==expected(12,3),"self assignment");
 c=ret12(9);check(sum12(c)==expected(12,9),"struct return");
 c=rp(3);check(sum12(c)==expected(12,3),"return function pointer");
 check(mix12(a,7,b,11UL,&p)==expected(12,3)+expected(12,9)+31u,"mixed argument positions");
 check(mix12(ret12(3),7,ret12(9),11UL,&p)==expected(12,3)+expected(12,9)+31u,"nested returns");
 check(rec12(a,3)==4u*expected(12,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 12 done\n");
}
static void test16(void) {
 struct S16 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S16)=sum16;struct S16 (*rp)(unsigned int)=ret16;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<16;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==16u,"size");check(sum16(a)==expected(16,3),"pass");
 check(fp(b)==expected(16,9),"function pointer");c=a;check(sum16(c)==expected(16,3),"assignment");
 { struct S16 *q=&c;*q=c; }check(sum16(c)==expected(16,3),"self assignment");
 c=ret16(9);check(sum16(c)==expected(16,9),"struct return");
 c=rp(3);check(sum16(c)==expected(16,3),"return function pointer");
 check(mix16(a,7,b,11UL,&p)==expected(16,3)+expected(16,9)+31u,"mixed argument positions");
 check(mix16(ret16(3),7,ret16(9),11UL,&p)==expected(16,3)+expected(16,9)+31u,"nested returns");
 check(rec16(a,3)==4u*expected(16,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 16 done\n");
}
static void test20(void) {
 struct S20 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S20)=sum20;struct S20 (*rp)(unsigned int)=ret20;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<20;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==20u,"size");check(sum20(a)==expected(20,3),"pass");
 check(fp(b)==expected(20,9),"function pointer");c=a;check(sum20(c)==expected(20,3),"assignment");
 { struct S20 *q=&c;*q=c; }check(sum20(c)==expected(20,3),"self assignment");
 c=ret20(9);check(sum20(c)==expected(20,9),"struct return");
 c=rp(3);check(sum20(c)==expected(20,3),"return function pointer");
 check(mix20(a,7,b,11UL,&p)==expected(20,3)+expected(20,9)+31u,"mixed argument positions");
 check(mix20(ret20(3),7,ret20(9),11UL,&p)==expected(20,3)+expected(20,9)+31u,"nested returns");
 check(rec20(a,3)==4u*expected(20,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 20 done\n");
}
static void test32(void) {
 struct S32 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S32)=sum32;struct S32 (*rp)(unsigned int)=ret32;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<32;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==32u,"size");check(sum32(a)==expected(32,3),"pass");
 check(fp(b)==expected(32,9),"function pointer");c=a;check(sum32(c)==expected(32,3),"assignment");
 { struct S32 *q=&c;*q=c; }check(sum32(c)==expected(32,3),"self assignment");
 c=ret32(9);check(sum32(c)==expected(32,9),"struct return");
 c=rp(3);check(sum32(c)==expected(32,3),"return function pointer");
 check(mix32(a,7,b,11UL,&p)==expected(32,3)+expected(32,9)+31u,"mixed argument positions");
 check(mix32(ret32(3),7,ret32(9),11UL,&p)==expected(32,3)+expected(32,9)+31u,"nested returns");
 check(rec32(a,3)==4u*expected(32,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 32 done\n");
}
static void test64(void) {
 struct S64 a,b,c;unsigned int i,p=13u;volatile unsigned int guard[4];
 unsigned int (*fp)(struct S64)=sum64;struct S64 (*rp)(unsigned int)=ret64;
 guard[0]=0x12345678u;guard[1]=0x87654321u;guard[2]=0xa5a5a5a5u;guard[3]=0x5a5a5a5au;
 for(i=0;i<64;i++){a.b[i]=(unsigned char)(3u+i*3u);b.b[i]=(unsigned char)(9u+i*3u);}
 check(sizeof(a)==64u,"size");check(sum64(a)==expected(64,3),"pass");
 check(fp(b)==expected(64,9),"function pointer");c=a;check(sum64(c)==expected(64,3),"assignment");
 { struct S64 *q=&c;*q=c; }check(sum64(c)==expected(64,3),"self assignment");
 c=ret64(9);check(sum64(c)==expected(64,9),"struct return");
 c=rp(3);check(sum64(c)==expected(64,3),"return function pointer");
 check(mix64(a,7,b,11UL,&p)==expected(64,3)+expected(64,9)+31u,"mixed argument positions");
 check(mix64(ret64(3),7,ret64(9),11UL,&p)==expected(64,3)+expected(64,9)+31u,"nested returns");
 check(rec64(a,3)==4u*expected(64,3),"recursion");
 check(guard[0]==0x12345678u && guard[1]==0x87654321u && guard[2]==0xa5a5a5a5u && guard[3]==0x5a5a5a5au,"stack guards");
 out("SIZE 64 done\n");
}
int main(void) {
 struct Nested a,b;unsigned int i,j,p=17u;
 out("AGG-BEGIN\n");
test1();test2();test3();test4();test5();test8();test12();test16();test20();test32();test64();
 a.l=0x70000u;a.s=65000u;a.z=0x80000000UL;for(i=0;i<2;i++)for(j=0;j<3;j++)a.a[i].b[j]=(unsigned char)(3*i+j);
 b=nested(a,10);check(b.l==0x7000au && b.s==65010u && b.z==0x8000000aUL,"nested scalar members");
 for(i=0;i<2;i++)for(j=0;j<3;j++)check(b.a[i].b[j]==a.a[i].b[j],"nested arrays");
 check(scalars(-3,253,-1234,65000u,-12345,0x87654321u,-123456L,0xfedcba98UL,&p)==0xabcdef01UL,"scalar ABI");
 check(variadic(4u,1u,0x10000u,70010u,3u)==135550u,"supported unsigned varargs");
 check((1u<<31)==0x80000000u && (0xffffffffu>>31)==1u && ((unsigned short)70010u)==4474u,"shifts promotions");
 out("CHECKS=");hex(checks);out(" FAILURES=");hex(failures);out("\nAGG-END\n");return failures?1:0;
}
