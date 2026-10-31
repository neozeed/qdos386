/* MAIN.ASM translation, GPL-3.0-or-later. Native QDOS program. */
#include "qlterm.h"
Terminal t;
char _prog_name[]="QLTerm C";
char _version[]="3.00b2 C-R1";
char *_endmsg=NULL;
long _stack=16384L;
long _mneed=32768L;
int (*_cmdparams)()=NULL;
long (*_cmdchannels)()=NULL;
int (*_cmdwildcard)()=NULL;
long (*_stackchannels)()=NULL;
/* All channels are owned explicitly below; suppress libc's stdio windows. */
chanid_t _conchan=-1;
void _stdchans(void) {}
static const char *const echoes[]={" Echo Off "," Echo On "," Local Only "};
static const char *const terms[]={" ASCII "," VT52 "," ANSI "};
static const char *const colours[]={" No colour "," 4-Colour "," 8-Colour "};
static const char *const ports[]={" SER1I "," SER1H "," SER2I "," SER2H "," Custom "," TCP "," Telnet "};
static const char *const bauds[]={" NONE "," 300 "," 600 "," 1200 "," 2400 "," 4800 "," 9600 "," 19K2 "," 38K4 "," 57K6 "," 115K2 "," 12H75 "};
static const char *const enters[]={" CR "," LF "," CR+LF "};
static const char *const dels[]={" CTRL-H (8) "," DEL (127) "};
static const char *const logs[]={" RAW "," COOKED "};
static const char *const yesno[]={" NO "," YES "};
static void text(chanid_t ch,const char *s) {io_sstrg(ch,-1,(void *)s,(short)strlen(s));}
void status_line(void) {
    aux(t.connected?" Online |":" Offline |");
    text(t.cmdwin,echoes[t.echo+1]);text(t.cmdwin,"|");
    text(t.cmdwin,terms[t.terminal+1]);text(t.cmdwin,"|");
    text(t.cmdwin,colours[t.colour]);text(t.cmdwin,"|");
    text(t.cmdwin,t.redirected?" ReDir ":ports[t.port]);text(t.cmdwin,"|");
    text(t.cmdwin,bauds[t.baud]);text(t.cmdwin,"| F3 - Menu");
}
int menu(const char *prompt,const char *const *items,int n,int current) {
    int i,x,col;U8 c;
    aux(prompt);col=strlen(prompt);
    for(i=0;i<n;++i)text(t.cmdwin,items[i]);
    if(current<0||current>=n)current=0;
    for(;;) {
        x=col;for(i=0;i<current;++i)x+=strlen(items[i]);
        sd_tab(t.cmdwin,-1,(short)x);sd_setst(t.cmdwin,-1,7);sd_setin(t.cmdwin,-1,0);
        text(t.cmdwin,items[current]);sd_setst(t.cmdwin,-1,0);sd_setin(t.cmdwin,-1,7);
        if(io_fbyte(t.cmdwin,-1,(char *)&c))continue;
        if(c==10)return current;
        if(c==ESC)return -1;
        sd_tab(t.cmdwin,-1,(short)x);text(t.cmdwin,items[current]);
        if(c==0xc0)current=(current+n-1)%n;
        if(c==0xc8)current=(current+1)%n;
    }
}
int ask_yes(const char *s) {
    U8 c;aux(s);text(t.cmdwin,"? (Y/N) ");sd_cure(t.cmdwin,-1);
    for(;;) {
        if(io_fbyte(t.cmdwin,-1,(char *)&c))continue;
        c&=0xdf;
        if(c=='Y'||c=='N') {sd_curs(t.cmdwin,-1);return c=='Y';}
    }
}
/* QDOS IO.EDLIN retains the connection string and its native edit keys. */
static int edit_line(const char *prompt,char *buf,int max,int remember) {
    char *p;short n;int rc;
    for(;;) {
        aux(prompt);n=remember?(short)strlen(buf):0;
        if(n)text(t.cmdwin,buf);
        p=buf+n;sd_cure(t.cmdwin,-1);
        rc=io_edlin(t.cmdwin,-1,&p,(short)max,n,&n);sd_curs(t.cmdwin,-1);
        if(rc<0)return rc;
        if(n&&p[-1]==ESC)return -1;
        if(n&&p[-1]==10) {buf[n-1]=0;return n-1;}
        buf[0]=0;
    }
}
int ask_file(int downloading) {
    int n;
    n=edit_line(downloading?"Download - File Name: ":"File Name: ",t.filename,44,0);
    return n>0?0:-1;
}
static jobid_t make_job(void (*entry)(void),const char *name,int priority) {
    char *base;jobid_t id;U8 *p;int n;
    n=strlen(name);
    id=mt_cjob(64,8192,NULL,-1,&base);
    if(id<0)return id;
    p=(U8 *)base;put16(p,0x4ef9);put32(p+2,(U32)entry);
    put16(p+6,0x4afb);put16(p+8,(U16)n);memcpy(p+10,name,n);
    /* Child executes ordinary C with its own stack, sharing relocated globals.
     * Entry functions never return; QDOS removes children with their parent. */
    mt_activ(id,(unsigned char)priority,0);return id;
}
void set_baud(void) {
    static const long rates[]={0,300,600,1200,2400,4800,9600,19200,38400,57600,115200,75};
    static const U8 check[]={0x0c,2,0,0,0,10,0x9f,0xf7,2};
    U8 cmd[]={0x0c,2,0,0,0,8,13,4,2,0};
    if(t.baud<=0||t.baud>11)return;
    mt_baud(rates[t.baud]);
    if(t.baud==11&&mt_ipcom((void *)check)==8) {
        /* SETBAUD inserts this bit in the fourth byte of $00080D04,
         * after the leading IPC header: byte 7 of the complete command. */
        cmd[7]|=(U8)(((t.port>>1)>>1)|(((t.port>>1)&1)<<7));mt_ipcom(cmd);
    }
}
static void send_command(int off) {
    char cmd[34];const char *p;U8 c;
    cfg_string(off,cmd);
    for(p=cmd;*p;++p) {
        c=*p;
        if(c=='~') {wait_frames(25);continue;}
        if(c=='|')c=13;else if(c=='\\')c=10;
        tx_raw(&c,1);
    }
}
void close_connection(void) {
    chanid_t ch;
    if(t.pipes)return;
    lock(&t.txlock);lock(&t.qlock);
    mt_prior(t.serialjob,0);
    ch=t.in;t.in=t.out=-1;
    if(ch>=0)io_close(ch);
    t.tail=t.head;t.connected=0;t.eof=0;
    t.got_iac=t.got_cmd=t.got_sub=t.sub_oper=0;
    unlock(&t.qlock);unlock(&t.txlock);
    mt_prior(t.serialjob,127);
}
int open_connection(void) {
    static const char *const devs[]={"SER1I","SER1H","SER2I","SER2H"};
    char name[50],message[128];chanid_t ch;
    if(t.redirected&&t.in>=0) {t.connected=1;return 0;}
    if(t.comname[0])strcpy(name,t.comname);
    else if(t.port<4)strcpy(name,devs[t.port]);
    else if(t.port==P_CUSTOM)cfg_string(CFG_CUSTOM,name);
    else return 0;
    if(t.in>=0)close_connection();
    ch=io_open(name,cfg_byte(CFG_OPENMODE));
    if(ch<0) {
        sprintf(message,"\n*** Warning: Could not open comms device %s. Try a different device.\n\n",name);
        text(t.mainwin,message);return (int)ch;
    }
    t.in=t.out=ch;t.eof=0;t.connected=1;return 0;
}
static void connect_command(void) {
    char cmd[40];int n;
    if(t.port>=P_TCP&&t.connected) {
        if(!ask_yes("You are still connected. Disconnect"))return;
        close_connection();
    }
    n=edit_line("Enter phone number, hostname or IP address: ",t.hostname,37,1);
    if(n<0)return;
    if(t.port>=P_TCP) {
        strcpy(t.comname,"TCP_");strcat(t.comname,t.hostname);
        if(!strchr(t.hostname,':'))strcat(t.comname,":23");
        if(open_connection()==0&&t.connected&&t.port==P_TELNET)telnet_start();
    } else {
        if(t.in<0&&open_connection())return;
        send_command(CFG_PREFIX);
        /* Phone number obeys the original modem substitutions too. */
        for(n=0;t.hostname[n];++n) {
            cmd[0]=t.hostname[n];
            if(cmd[0]=='~')wait_frames(25);
            else {if(cmd[0]=='|')cmd[0]=13;else if(cmd[0]=='\\')cmd[0]=10;tx_raw(cmd,1);}
        }
        send_command(CFG_POSTFIX);
    }
    t.xoff=0;
}
static void hangup(void) {
    if(!ask_yes("Disconnect line"))return;
    if(t.pipes)return;
    if(t.port>=P_TCP||t.eof)close_connection();
    else {send_command(CFG_HANGUP);wait_frames(50);}
}
static void interface_menu(void) {
    int n=menu("Transmit Mode: ",echoes,3,t.echo+1);
    if(n<0)return;t.echo=n-1;
    if(!t.redirected) {
        n=menu("Port: ",ports,7,t.port);if(n<0)return;
        close_connection();t.port=n;t.comname[0]=0;
    }
    if(t.port<P_TCP) {n=menu("Baud: ",bauds,12,t.baud);if(n>=0)t.baud=n;set_baud();}
    open_connection();
}
static void options(void) {
    int n;
    n=menu("Terminal Emulation: ",terms,3,t.terminal+1);if(n<0)return;t.terminal=n-1;
    n=menu("Colour: ",colours,3,t.colour);if(n<0)return;t.colour=n;if(!n)screen_reset();
    n=menu("Character to send on ENTER key: ",enters,3,t.enterkey);if(n<0)return;t.enterkey=n;
    n=menu("Character to send on Backspace key: ",dels,2,t.delkey);if(n<0)return;t.delkey=n;
    n=menu("Logging: ",logs,2,t.cooked);if(n<0)return;t.cooked=n;
    n=menu("Convert CR/LF for LOG & TRANSMIT: ",yesno,2,t.convlf);if(n<0)return;t.convlf=n;
    t.escstate=0;t.paramlen=0;
}
static void log_command(void) {
    chanid_t ch;
    if(t.log>=0) {
        if(ask_yes("Logging is active. Close logfile")) {ch=t.log;t.log=-1;io_close(ch);aux("Logfile closed");wait_frames(100);}
        return;
    }
    if(ask_file(0))return;
    ch=open_download(t.filename);if(ch<0)file_error(ch);else t.log=ch;
}
static void transmit(void) {
    chanid_t ch;U8 buf[1026],c;int n;
    if(ask_file(0))return;
    ch=open_upload(t.filename);if(ch<0) {file_error(ch);return;}
    /* Reading bytes preserves partial final lines and exactly identifies LF. */
    n=0;
    while(io_fbyte(ch,-1,(char *)&c)==0) {
        if(c==10&&t.convlf)buf[n++]=13;
        buf[n++]=c;
        if(n>=1024||c==10) {tx_data(buf,n,1);n=0;}
        if(io_fbyte(t.cmdwin,0,(char *)&c)==0&&c==ESC)break;
    }
    if(n)tx_data(buf,n,1);io_close(ch);
}
static void buffer_menu(void) {
    char line[8],prompt[80],*end;long n;
    sprintf(prompt,"Current buffer size is %dK. New size (in K): ",t.buflen);
    for(;;) {
        if(edit_line(prompt,line,5,0)<0)return;
        n=strtol(line,&end,10);
        if(*line&&!*end&&n>=0&&n<=32767) {t.buflen=(int)n;return;}
        strcpy(prompt,"Invalid input, enter again: ");
    }
}
static int commands(void) {
    U8 c;
    t.pause_screen=1;sd_curs(t.mainwin,-1);
    for(;;) {
        aux("Connect, Download, Hangup, Interface, Log, Options, Quit, Transmit, Upload >");
        sd_cure(t.cmdwin,-1);
        if(io_fbyte(t.cmdwin,-1,(char *)&c))continue;
        sd_curs(t.cmdwin,-1);
        if(c==ESC)break;
        switch(c&0xdf) {
        case 'B':buffer_menu();break;
        case 'C':connect_command();break;
        case 'D':download();break;
        case 'H':hangup();break;
        case 'I':interface_menu();break;
        case 'L':log_command();break;
        case 'O':options();break;
        case 'Q':if(t.connected)hangup();return 1;
        case 'T':transmit();break;
        case 'U':upload();break;
        default:continue;
        }
        break;
    }
    t.pause_screen=0;return 0;
}
static chanid_t window_open(WINDOWDEF_t *w,int console) {
    QLRECT_t r;chanid_t ch;int e;
    ch=io_open(console?"CON_":"SCR_",0);if(ch<0)return ch;
    r.q_width=w->width;r.q_height=w->height;r.q_x=w->x_origin;r.q_y=w->y_origin;
    e=sd_wdef(ch,-1,w->border_colour,w->border_width,&r);
    if(e) {io_close(ch);return e;}
    sd_setpa(ch,-1,w->paper);sd_setst(ch,-1,w->paper);sd_setin(ch,-1,w->ink);
    sd_clear(ch,-1);return ch;
}
static int windows(void) {
    WINDOWDEF_t w;
    QLRECT_t r;
    w.border_colour=0;w.border_width=0;w.paper=0;w.ink=7;
    w.width=cfg_word(CFG_COLS)*6+4;w.height=cfg_word(CFG_ROWS)*10+14;
    w.x_origin=cfg_word(CFG_X);w.y_origin=cfg_word(CFG_Y);
    t.guardwin=window_open(&w,0);
    if(t.guardwin==-4) {w.width=484;w.height=254;w.x_origin=14;w.y_origin=2;t.guardwin=window_open(&w,0);}
    if(t.guardwin<0)return (int)t.guardwin;
    w.border_colour=cfg_byte(CFG_BORDER);w.border_width=1;
    w.paper=cfg_byte(CFG_PAPER);w.ink=cfg_byte(CFG_INK);w.height-=12;
    t.mainwin=window_open(&w,1);if(t.mainwin<0)return (int)t.mainwin;
    if(sd_chenq(t.mainwin,-1,&r))return -19;
    t.cols=r.q_width;t.rows=r.q_height;
    sd_fount(t.mainwin,-1,NULL,(char *)gr_set1);
    w.y_origin+=w.height;w.height=12;w.border_colour=2;w.paper=0;w.ink=7;
    t.cmdwin=window_open(&w,1);return t.cmdwin<0?(int)t.cmdwin:0;
}
int main(void) {
    U8 *sp=(U8 *)_SPorig,c;int n,e,quit=0;short mode=-1,display=-1;
    chanid_t errwin;
    memset(&t,0,sizeof(t));t.in=t.out=t.log=-1;t.serialjob=t.screenjob=-1;
    t.baud=cfg_byte(CFG_BAUD);t.echo=(signed char)cfg_byte(CFG_ECHO);
    t.terminal=(signed char)cfg_byte(CFG_TERM);t.colour=(signed char)cfg_byte(CFG_COLOUR);
    t.convlf=cfg_byte(CFG_CONVLF);t.cooked=cfg_byte(CFG_COOKED);t.port=cfg_byte(CFG_PORT);
    t.enterkey=cfg_byte(CFG_ENTER);t.delkey=cfg_byte(CFG_DEL);t.protocol=cfg_byte(CFG_PROTO);
    t.buflen=cfg_word(CFG_DLBUF);t.bell=cfg_byte(CFG_BELL);t.disable_tra=cfg_byte(CFG_TRA);
    cfg_string(CFG_DLDIR,t.dldir);
    n=be16(sp);sp+=2;
    if(n>3)goto bad_channels;
    if(n) {
        t.redirected=1;t.in=be32(sp);t.out=t.in;sp+=4;
        if(n==3) {t.log=be32(sp);sp+=4;}
        if(n>=2) {t.out=be32(sp);sp+=4;t.pipes=1;}
    } else {
        /* C68 startup converts the counted command tail in place to a C
         * string even when _cmdparams is disabled. Channels stay counted. */
        strncpy(t.comname,(const char *)sp,QLT_NAME_MAX-1);
        t.comname[QLT_NAME_MAX-1]=0;
    }
    mt_dmode(&mode,&display);
    if(mode==8) {
        errwin=io_open("CON_",0);text(errwin,"Sorry, this program cannot be run in MODE 8\n");goto failure;
    }
    if(t.colour<0)t.colour=mode==2?0:mode==0?1:2;
    t.qsize=(U32)cfg_word(CFG_RXBUF)*1024UL;
    if(t.qsize<64)t.qsize=64;
    t.queue=malloc(t.qsize);
    if(!t.queue) {
        errwin=io_open("CON_",0);
        text(errwin,"Cannot allocate receive buffer. Please configure a smaller buffer\n");
        goto failure;
    }
    e=windows();if(e)goto failure;
    screen_reset();
    text(t.mainwin,"QLTERM Version 3.00b2 - C port R1\n\nCopyright (c) 1987-2026 Jan Bredenbeek\nGPL v3 or later\n\nColour mode: ");
    text(t.mainwin,t.colour==0?"Black and White\n":t.colour==1?"4-colour\n":"8-colour\n");
    mt_prior(-1,32);
    t.screenjob=make_job(screen_job,"QLTERM screen",32);if(t.screenjob<0)goto failure;
    t.serialjob=make_job(serial_job,"QLTerm I/O handler",127);if(t.serialjob<0)goto failure;
    if(!t.redirected)set_baud();
    open_connection();t.status=1;status_line();
    while(!quit) {
        e=io_fbyte(t.mainwin,0,(char *)&c);
        if(!e) {
            if(c==0xf0) {
                quit=commands();if(t.status&&!quit)status_line();
            } else {
                if(!t.status) {t.status=1;status_line();}
                key_send(c);
            }
        } else {
            wait_frames(2);
            if(t.eof&&t.head==t.tail) {
                if(t.pipes)break;
                if(t.in>=0) {close_connection();screen_reset();text(t.mainwin,"\n\n*** Session disconnected by remote ***\n\n");status_line();}
            }
        }
    }
    mt_prior(t.screenjob,0);mt_prior(t.serialjob,0);sd_clear(t.guardwin,-1);
    mt_frjob(-1,0);return 0;
bad_channels:
    errwin=io_open("CON_",0);text(errwin,"Too many files specified in EX/EW command\n");
failure:
    mt_frjob(-1,-1);return 1;
}
