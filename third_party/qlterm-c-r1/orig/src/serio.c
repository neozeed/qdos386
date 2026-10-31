/* serio.asm and MAIN.ASM keyboard tables, GPL-3.0-or-later. */
#include "qlterm.h"
int tx_raw(const void *vp,int n) {
    const U8 *p=vp;int rc=0,k;
    if(t.out<0)return -1;
    lock(&t.txlock);
    while(n>0) {
        k=io_sstrg(t.out,0,(void *)p,(short)(n>32767?32767:n));
        if(k==-1||k==0) {wait_frames(4);continue;}
        if(k<0) {rc=k;break;}
        p+=k;n-=k;
    }
    unlock(&t.txlock);return rc;
}
int queue_put(U8 c) {
    U32 next;
    lock(&t.qlock);
    next=t.head+1;if(next==t.qsize)next=0;
    if(next==t.tail) {unlock(&t.qlock);return -1;}
    t.queue[t.head]=c;t.head=next;unlock(&t.qlock);return 0;
}
int tx_data(const void *vp,int n,int echo) {
    const U8 *p=vp,*start=p;int len=n,rc=0,k=0;
    static const U8 twice[]={IAC,IAC};
    if(!echo||t.echo<=0) {
        if(t.port!=P_TELNET)rc=tx_raw(p,n);
        else {
            while(n>0) {
                for(k=0;k<n&&p[k]!=IAC;++k) ;
                if(k&&(rc=tx_raw(p,k))!=0)break;
                p+=k;n-=k;
                if(n) {rc=tx_raw(twice,2);if(rc)break;++p;--n;}
            }
        }
    }
    if(!rc&&echo&&t.echo>=0) {
        while(len--) {
            /* Never overwrite unread input on local echo. */
            while(queue_put(*start))wait_frames(1);
            ++start;
        }
    }
    return rc;
}
int tx_char(int c,int echo) { U8 b=(U8)c;return tx_data(&b,1,echo); }
void telnet_start(void) {
    static const U8 opts[]={255,253,0,255,251,0,255,253,1,255,253,3,255,251,3,255,251,24,255,251,31};
    tx_raw(opts,sizeof(opts));
}
/* Returns 0 for a data byte, +1 for timeout, negative QDOS error otherwise.
 * State lives across calls, including fragmented IAC negotiations. */
int rxchar(int timeout,U8 *out) {
    U8 c,reply[24];int e,cmd,n;
    const char *name;
    for(;;) {
        if(t.in<0)return -10;
        e=io_fbyte(t.in,(short)timeout,(char *)&c);
        if(e)return e==-1?1:e;
        if(t.port!=P_TELNET) {*out=c;return 0;}
        if(!t.got_iac) {
            if(c!=IAC) {*out=c;return 0;}
            t.got_iac=1;t.got_cmd=0;continue;
        }
        cmd=t.got_cmd;
        if(!cmd) {
            if(c==IAC) {t.got_iac=0;*out=c;return 0;}
            if(c==240) {
                t.got_iac=0;
                if(t.got_sub==24&&t.sub_oper==1) {
                    name=t.terminal<0?"ASCII":t.terminal==0?"VT52":"ANSI";
                    reply[0]=255;reply[1]=250;reply[2]=24;reply[3]=0;
                    n=strlen(name);memcpy(reply+4,name,n);reply[n+4]=255;reply[n+5]=240;
                    tx_raw(reply,n+6);
                }
                t.got_sub=0;continue;
            }
            if(c>=250)t.got_cmd=c;else t.got_iac=0;
            continue;
        }
        if(cmd==250) {
            if(!t.got_sub) {t.got_sub=c;continue;}
            t.sub_oper=c;t.got_iac=0;continue;
        }
        t.got_iac=0;
        if(cmd!=251&&cmd!=253)continue;
        if(c==31) {
            reply[0]=255;reply[1]=250;reply[2]=31;
            put16(reply+3,(U16)t.cols);put16(reply+5,(U16)t.rows);
            reply[7]=255;reply[8]=240;tx_raw(reply,9);continue;
        }
        reply[0]=255;reply[2]=c;
        if(c==0||c==3||c==24||(c==1&&cmd==251))reply[1]=cmd^6;
        else reply[1]=cmd==251?254:252;
        tx_raw(reply,3);
    }
}
void serial_job(void) {
    U8 c;int e,n;U32 used,space,head,tail;
    for(;;) {
        if(t.transfer||t.in<0) {wait_frames(1);continue;}
        if(t.port==P_TELNET) {
            for(n=0;n<32767;++n) {
                head=t.head+1;if(head==t.qsize)head=0;
                if(head==t.tail)break;
                e=rxchar(0,&c);if(e) {if(e==-10)t.eof=1;break;}
                while(queue_put(c))wait_frames(1);
            }
        } else {
            lock(&t.qlock);
            head=t.head;tail=t.tail;
            space=tail>head?tail-head-1:t.qsize-head-(tail==0?1:0);
            if(space>32767UL)space=32767UL;
            if(space) {
                e=io_fstrg(t.in,0,t.queue+head,(short)space);
                if(e>0) {head+=e;if(head==t.qsize)head=0;t.head=head;}
                else if(e==-10)t.eof=1;
            }
            unlock(&t.qlock);
        }
        used=t.head>=t.tail?t.head-t.tail:t.qsize-t.tail+t.head;
        if(used>=t.qsize*3/4&&!t.xoff&&!t.pipes&&tx_char(19,0)==0)t.xoff=1;
        wait_frames(1);
    }
}
void key_send(int c) {
    static const U8 ascii[]={0xc0,1,8,0xc8,1,9,0xd0,1,11,0xd8,1,10,0};
    static const U8 vt52[]={0xc0,2,27,'D',0xc1,1,8,0xc8,2,27,'C',0xca,2,27,'K',0xd0,2,27,'A',0xd1,1,11,0xd4,2,27,'I',0xd8,2,27,'B',0xd9,1,10,0};
    static const U8 ansi[]={0xfd,3,27,'[','Z',0xc0,3,27,'[','D',0xd8,3,27,'[','B',0xc8,3,27,'[','C',0xd0,3,27,'[','A',0xc3,3,27,'[','M',0xcb,3,27,'[','K',0xca,3,27,'[','@',0xda,3,27,'[','L',0xdc,3,27,'[','U',0xd4,3,27,'[','V',0xd9,3,27,'[','T',0xc5,4,27,'[',' ','@',0xcd,4,27,'[',' ','A',0xd1,3,27,'[','S',0};
    const U8 *p;
    switch(c) {
    case 10:if(t.enterkey==2) {tx_data("\r\n",2,1);return;} c=t.enterkey?10:13;break;
    case 0xc2:c=t.delkey?127:8;break;
    case 0xf4:c=19;break;
    case 0xf6:c=17;break;
    case 0xf8:c=3;break;
    default:
        p=t.terminal<0?ascii:t.terminal==0?vt52:ansi;
        while(*p) {if(*p++==c) {tx_data(p+1,*p,1);return;}p+=*p+1;}
    }
    tx_char(c,1);
}
