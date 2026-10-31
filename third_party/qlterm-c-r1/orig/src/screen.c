/* SCREEN.ASM translation. QDOS owns cursor, wrapping and bitmap rendering.
 * GPL-3.0-or-later. The original deliberately accepts VT52 and CSI in all modes. */
#include "qlterm.h"
static const U8 ansi4[]={0,2,4,6,2,3,5,7};
static const U8 ansi8[]={0,2,4,6,1,3,5,7};
static const U8 vt4[]={0,2,2,3,4,6,5,7};
static const U8 vt8[]={0,2,1,3,4,6,5,7};
static void pos(int *x,int *y) {
    QLRECT_t r;
    memset(&r,0,sizeof(r)); sd_chenq(t.mainwin,-1,&r);
    *x=r.q_x; *y=r.q_y;
}
static void setpos(int x,int y) {
    if(sd_pos(t.mainwin,-1,(short)x,(short)y))
        sd_tab(t.mainwin,-1,(short)x);
}
static void ink(int n) { t.ink=n; sd_setin(t.mainwin,-1,(colour_t)n); }
static void strip(int n) { t.strip=n; sd_setst(t.mainwin,-1,(colour_t)n); }
static void paper(int n) { t.paper=n; sd_setpa(t.mainwin,-1,(colour_t)n); }
void screen_reset(void) {
    t.inverse=0; sd_setul(t.mainwin,-1,0);
    paper(cfg_byte(CFG_PAPER)); strip(t.paper); ink(cfg_byte(CFG_INK));
}
static int number(char **pp,int def,int *present) {
    char *p=*pp;
    unsigned n=0;
    *present=(*p>='0' && *p<='9');
    if(*present) {
        do { n=(n*10+*p++-'0')&65535U; } while(*p>='0' && *p<='9');
        def=(short)n;
    }
    if(*p==';'||*p=='?') ++p;
    *pp=p; return def;
}
static void clear_start(void) {
    int x,y,n; pos(&x,&y); sd_tab(t.mainwin,-1,0);
    for(n=0;n<=x;++n) io_sbyte(t.mainwin,-1,' ');
    sd_pcol(t.mainwin,-1);
}
static void rendition(int n) {
    int old;
    const U8 *col=t.colour==1?ansi4:ansi8;
    switch(n&255) {
    case 0:screen_reset();break;
    case 4:case 24:sd_setul(t.mainwin,-1,(char)(n==4));break;
    case 7:case 27:
        if(t.inverse!=(n==7)) {
            t.inverse=(n==7);old=t.strip;strip(t.ink);ink(old);
        } break;
    default:
        if(!t.colour) break;
        if(n>=30&&n<=37) ink(col[n-30]);
        if(n>=40&&n<=47) {paper(col[n-40]);strip(t.paper);}
    }
}
static void csi(int command) {
    char *p=t.params,reply[24];
    int n,more,x,y,def=1;
    if(command=='n'||command=='J'||command=='K'||command=='m'||command=='s'||command=='u') def=0;
    n=number(&p,def,&more);
    if(t.space) {
        if(command=='@') sd_pan(t.mainwin,-1,(short)(-6*n));
        if(command=='A') sd_pan(t.mainwin,-1,(short)(6*n));
        return;
    }
    switch(command) {
    case 'D':n=-n; /* fall through */
    case 'I':case 'C':case 'a':pos(&x,&y);setpos(x+n,y);break;
    case 'A':n=-n; /* fall through */
    case 'B':case 'e':pos(&x,&y);setpos(x,y+n);break;
    case 'd':pos(&x,&y);setpos(x,n-1);break;
    case 'H':case 'f':y=n-1;x=number(&p,1,&more)-1;setpos(x,y);break;
    case 'G':case '`':sd_tab(t.mainwin,-1,(short)(n-1));break;
    case 'E':pos(&x,&y);y+=n;
        if(y>=t.rows) {sd_scrol(t.mainwin,-1,(short)(-10*(1+y-t.rows)));y=t.rows-1;}
        setpos(0,y);break;
    case 'F':pos(&x,&y);y-=n;
        if(y<0) {sd_scrol(t.mainwin,-1,(short)(-10*y));y=0;}
        setpos(0,y);break;
    case 'S':sd_scrol(t.mainwin,-1,(short)(-10*n));break;
    case 'T':sd_scrol(t.mainwin,-1,(short)(10*n));break;
    case 'm':case 'J':case 'K':
        do {
            if(command=='m') rendition(n);
            else if(command=='K') {
                if(n==1) clear_start();
                else if(n>1) sd_clrln(t.mainwin,-1);
                else sd_clrrt(t.mainwin,-1);
            } else {
                if(n==1) {clear_start();sd_clrtp(t.mainwin,-1);}
                else if(n>1) sd_clear(t.mainwin,-1);
                else {sd_clrrt(t.mainwin,-1);sd_clrbt(t.mainwin,-1);}
            }
            n=number(&p,n,&more);
        } while(more);
        break;
    case 's':pos(&t.saved_x,&t.saved_y);break;
    case 'u':sd_pos(t.mainwin,-1,(short)t.saved_x,(short)t.saved_y);break;
    case 'n':if(n==6) {pos(&x,&y);sprintf(reply,"\033[%d;%dR",y+1,x+1);tx_data(reply,strlen(reply),1);}break;
    }
}
static void vt(int c) {
    int col;
    switch(c) {
    case 'A':sd_prow(t.mainwin,-1);break;
    case 'B':sd_nrow(t.mainwin,-1);break;
    case 'C':sd_ncol(t.mainwin,-1);break;
    case 'D':sd_pcol(t.mainwin,-1);break;
    case 'H':sd_pos(t.mainwin,-1,0,0);break;
    case 'I':if(sd_prow(t.mainwin,-1))sd_scrol(t.mainwin,-1,10);break;
    case 'J':sd_clrbt(t.mainwin,-1);sd_clrrt(t.mainwin,-1);break;
    case 'K':sd_clrrt(t.mainwin,-1);break;
    case 'Y':sd_pos(t.mainwin,-1,(U8)(t.params[1]-32),(U8)(t.params[0]-32));break;
    case 'T':case 'U':case 'V':
        col=(U8)(t.params[0]-64);
        if(!t.colour||col>7) break;
        col=vt4[col];if(t.colour!=1) col=vt8[col];
        if(c=='T') ink(col);else if(c=='U')strip(col);else paper(col);
        break;
    }
}
void screen_byte(U8 c) {
    int x,y;
    static const U8 beep[]={10,8,0x55,0x55,0xaa,0xaa,5,5,0,0,144,13,0,0,1};
    if(t.log>=0 && (!t.escstate || !t.cooked) &&
       !(t.convlf&&c==13) && !(t.cooked&&c==ESC)) io_sbyte(t.log,-1,c);
    if(t.escstate) {
        if(t.escstate==1) {
            if(c=='[') {t.escstate=2;t.paramlen=0;t.space=0;return;}
            if(c=='T'||c=='U'||c=='V'||c=='Y') {
                t.vtcmd=c;t.vtleft=c=='Y'?2:1;t.paramlen=0;t.escstate=3;return;
            }
            sd_curs(t.mainwin,-1);sd_donl(t.mainwin,-1);vt(c);
        } else if(t.escstate==3) {
            t.params[t.paramlen++]=c;
            if(--t.vtleft) return;
            sd_curs(t.mainwin,-1);sd_donl(t.mainwin,-1);vt(t.vtcmd);
        } else {
            if(c==';'||c=='?'||(c>='0'&&c<='9')) {
                if(t.paramlen<79)t.params[t.paramlen++]=c;
                return;
            }
            if(c==' ') {t.space=1;return;}
            t.params[t.paramlen]=0;
            sd_curs(t.mainwin,-1);sd_donl(t.mainwin,-1);csi(c);
        }
        t.escstate=0;return;
    }
    sd_curs(t.mainwin,-1);
    if(c>=32&&c!=127) {io_sbyte(t.mainwin,-1,c);return;}
    sd_donl(t.mainwin,-1);
    switch(c) {
    case 7:if(t.bell)mt_ipcom((void *)beep);break;
    case 8:sd_pcol(t.mainwin,-1);break;
    case 9:pos(&x,&y);sd_tab(t.mainwin,-1,(short)((x+8)&248));break;
    case 10:if(sd_nrow(t.mainwin,-1))sd_scrol(t.mainwin,-1,-10);break;
    case 11:if(sd_prow(t.mainwin,-1))sd_scrol(t.mainwin,-1,10);break;
    case 12:sd_clear(t.mainwin,-1);break;
    case 13:sd_tab(t.mainwin,-1,0);break;
    case ESC:t.escstate=1;break;
    case 127:sd_pcol(t.mainwin,-1);io_sbyte(t.mainwin,-1,' ');sd_pcol(t.mainwin,-1);break;
    }
}
void screen_job(void) {
    U8 c;U32 end,next,n;
    for(;;) {
        if(t.transfer || t.head==t.tail) {
            if(!t.pause_screen&&!t.transfer)sd_cure(t.mainwin,-1);
            wait_frames(1);continue;
        }
        /* Keep the source's contiguous printable-string fast path. */
        end=t.head;next=t.tail;c=t.queue[next];
        if(!t.escstate&&c>=32&&c!=127) {
            n=0;
            do {++n;++next;
                if(next==t.qsize||next==end||n==32767UL)break;
                c=t.queue[next];
            } while(c>=32&&c!=127);
            sd_curs(t.mainwin,-1);
            if(t.log>=0)io_sstrg(t.log,-1,t.queue+t.tail,(short)n);
            io_sstrg(t.mainwin,-1,t.queue+t.tail,(short)n);
            if(next==t.qsize)next=0;
            t.tail=next;
        } else {
            next++;if(next==t.qsize)next=0;t.tail=next;screen_byte(c);
        }
        n=t.head>=t.tail?t.head-t.tail:t.qsize-t.tail+t.head;
        if(t.xoff&&n<=32UL&&tx_char(17,0)==0)t.xoff=0;
    }
}
