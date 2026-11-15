/* Real native B0/B1 regression: mandatory close/owner cleanup and high clusters. */
#include <stdio.h>
#include "qdos386.h"
int main(int argc,char **argv)
{
 unsigned char b[137];int ch,n,i,pass,bad=0,total,w;
 if(argc>1 && (argv[1][0]=='b' || argv[1][0]=='f')) {
  ch=qd_open("WIN0:/FRAG.BIN");
  if(ch>=0) { qd_close(ch);puts("R7RO FAIL malformed accepted");return 1; }
  printf("R7RO malformed refused=%d\n",ch);
  if(argv[1][0]=='b') {
   ch=qd_open("WIN0:/NEST/TINY.TXT");if(ch<0)return 1;qd_close(ch);
  } else if(ch!=-18)return 1;
  puts("R7RO refusal cleanup PASS");return 0;
 }
 if(argc>1 && argv[1][0]=='n') {
  ch=qd_open("R7BAD");printf("R7NULL result=%d expected=-15\n",ch);return ch==-15?0:1;
 }
 if(argc>1 && argv[1][0]=='e') {
  ch=qd_open("WIN0:/FRAG.BIN");
  if(ch<0)return 1;
  puts("R7RO owner-cleanup: leaving file open at process exit");return 0;
 }
 for(pass=0;pass<3;pass++) {
  total=0;ch=qd_open("WIN0:/FRAG.BIN");
  if(ch<0){printf("R7RO FAIL open=%d\n",ch);return 1;}
  while((n=qd_read(ch,b,sizeof(b)))>0) {
   for(i=0;i<n;i++)if(b[i]!=(unsigned char)((total+i)*37+19))bad++;
   total+=n;
  }
  if(n<0 || total!=1703)bad++;
  if(qd_seek(ch,511)||qd_read(ch,b,137)!=137)bad++;
  for(i=0;i<137;i++)if(b[i]!=(unsigned char)((511+i)*37+19))bad++;
  if(qd_close(ch))bad++;
  if(qd_close(ch)>=0)bad++;
 }
 w=qd_open_ex("WIN0:/DENIED.TXT",3u|4u);
 if(w!=-80){bad++;if(w>=0)qd_close(w);}
 ch=qd_open("WIN0:/NEST/TINY.TXT");
 if(ch<0)bad++;else {if(qd_read(ch,b,1)!=0)bad++;qd_close(ch);}
 ch=qd_open("WIN0:/ABSENT.TXT");if(ch>=0){bad++;qd_close(ch);}
 printf("R7RO %s bytes=1703 rounds=3 bad=%d writer=%d\n",bad?"FAIL":"PASS",bad,w);
 return bad?1:0;
}
