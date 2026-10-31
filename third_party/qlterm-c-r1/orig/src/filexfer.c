/* FILEXFER.ASM / CRCCITT.ASM port. GPL-3.0-or-later.
 * Wire framing is kept separate from QDOS file/header handling. */
#include "qlterm.h"
#include "crc_table.h"
static const char *const protocols[]={" XMODEM "," XMODEM-1K "," YMODEM "," YMODEM-G "};
static const U8 qdos_marker[]= "]!QDOS File Header"; /* The source's exact 18-byte marker. */
static U8 block[1030];
static U32 last_progress;
U16 crc16(const U8 *p,int n) {
    U16 crc=0;
    while(n--)crc=(U16)((crc<<8)^crc_table[(crc>>8)^*p++]);
    return crc;
}
static U8 checksum(const U8 *p,int n) {U8 sum=0;while(n--)sum+=*p++;return sum;}
int progress(const char *msg,U32 done,long total,U32 start) {
    char line[180],nums[70];U8 c;U32 now=mt_rclck(),elapsed;
    if(msg||now!=last_progress) {
        last_progress=now;
        sprintf(line,"File: %s %lu",t.filename,done);
        if(total>=0) {sprintf(nums,"/%lu",(U32)total);strcat(line,nums);}
        strcat(line,"  ");elapsed=now-start;
        if(start&&elapsed) {sprintf(nums,"%lu B/s  ",done/elapsed);strcat(line,nums);}
        if(msg)strcat(line,msg);
        sd_tab(t.cmdwin,0,0);io_sstrg(t.cmdwin,0,line,(short)strlen(line));sd_clrrt(t.cmdwin,0);
    }
    return io_fbyte(t.cmdwin,0,(char *)&c)==0&&c==CAN;
}
void transfer_mode(int on) {
    if(on) {
        /* Stop children only after any short critical section has ended. */
        lock(&t.txlock);lock(&t.qlock);
        mt_prior(t.serialjob,0);mt_prior(t.screenjob,0);t.transfer=1;
        unlock(&t.qlock);unlock(&t.txlock);
        if(t.disable_tra)mt_trans((void *)0,(void *)0);
    } else {
        if(t.disable_tra)mt_trans((void *)1,(void *)0);
        t.transfer=0;mt_prior(t.serialjob,127);mt_prior(t.screenjob,32);
    }
}
static void cancel_transfer(void) {tx_char(CAN,0);tx_char(CAN,0);}
static int read_bytes(U8 *p,int len,int timeout) {
    int n,e;
    while(len>0) {
        if(t.port==P_TELNET) {e=rxchar(timeout,p);if(e)return e;n=1;}
        else {
            n=io_fstrg(t.in,(short)timeout,p,(short)len);
            if(n<=0)return n==-1||n==0?1:n;
        }
        p+=n;len-=n;
    }
    return 0;
}
static int receive_block(int type,int crc,int *length,int *sequence) {
    int n=type==SOH?128:1024,e;
    U16 check;
    e=read_bytes(block,n+2+(crc?2:1),500);if(e)return e;
    /* Reject corrupt numbering as well as corrupt payloads. */
    if((U8)(block[0]^block[1])!=255)return -11;
    if(crc) {check=crc16(block+2,n);if(check!=be16(block+2+n))return -11;}
    else if(checksum(block+2,n)!=block[2+n])return -11;
    *length=n;*sequence=block[0];return 0;
}
static int send_block(int seq,const U8 *data,int len,int crc) {
    block[0]=len==128?SOH:STX;block[1]=(U8)seq;block[2]=(U8)~seq;
    memmove(block+3,data,len);
    if(crc)put16(block+3+len,crc16(block+3,len));
    else block[3+len]=checksum(block+3,len);
    return tx_data(block,len+3+(crc?2:1),0);
}
static void purge(void) {U8 c;while(rxchar(50,&c)==0) ;}
/* Download file state, independent of the block-number rollover. */
typedef struct {
    chanid_t ch;
    U8 *buffer;
    U32 capacity,used,received,written;
    long length;
    U32 date;
    int first;
} Download;
static int save_buffer(Download *d,const U8 *data,U32 n) {
    U8 header[64];U32 hdrlen,remaining;int e;
    if(!n)return 0;
    if(d->first) {
        d->first=0;
        if(n>=30&&!memcmp(data,qdos_marker,18)) {
            hdrlen=(U32)data[19]*2;
            if(hdrlen<30||hdrlen>n)return -11;
            memset(header,0,sizeof(header));memcpy(header+4,data+20,10);
            e=fs_heads(d->ch,-1,header,14);if(e<0)return e;
            data+=hdrlen;n-=hdrlen;d->received-=hdrlen;
            if(d->length>=0)d->length-=hdrlen;
        }
    }
    if(d->length>=0) {
        remaining=(U32)d->length>d->written?(U32)d->length-d->written:0;
        if(n>remaining)n=remaining;
    }
    if(n) {e=fs_save(d->ch,(void *)data,n);if(e<0)return e;d->written+=n;}
    return 0;
}
static int flush_download(Download *d) {
    int e=save_buffer(d,d->buffer,d->used);d->used=0;return e;
}
static int store_data(Download *d,const U8 *data,int n) {
    int e;
    d->received+=n;
    if(!d->capacity) {e=save_buffer(d,data,n);if(!e)wait_media(t.filename);return e;}
    if(d->used+(U32)n>d->capacity) {
        e=flush_download(d);if(e)return e;wait_media(t.filename);
    }
    memcpy(d->buffer+d->used,data,n);d->used+=n;return 0;
}
static void finish_download(Download *d) {
    long date;
    if(d->ch<0)return;
    if(d->used) {int e=flush_download(d);if(e)file_error(e);}
    /* Apply date after writes, so host/SD drivers cannot replace it on write. */
    if(d->date) {date=d->date;fs_date(d->ch,-1,0,&date);}
    io_close(d->ch);d->ch=-1;
}
static int descriptor(Download *d,const U8 *data,int size) {
    int n=0,k;char name[38],number[64],*end;U32 date;
    if(!data[0])return 1;
    while(n<size&&data[n])++n;
    if(n==size)return -12;
    k=n>35?35:n;memcpy(name,data,k);name[k]=0;
    if(n>35)name[34]='*';
    d->ch=open_download(name);if(d->ch<0)return (int)d->ch;
    ++n;k=size-n;if(k>63)k=63;memcpy(number,data+n,k);number[k]=0;
    d->length=(long)strtoul(number,&end,10);
    if(d->length==0)d->length=-1; /* matches DL_DESC's unknown-length convention */
    date=*end==' '?strtoul(end+1,NULL,8):0;
    d->date=date?date+QL_EPOCH:0;return 0;
}
void download(void) {
    Download d;U32 expected,start;U8 c;int proto,crc,g,tries,errors,n,seq,e;
    int got_eot,can_count,request,done=0,active=0;
    char msg[80],name[QLT_NAME_MAX];
    proto=menu("",protocols,4,t.protocol);if(proto<0)return;t.protocol=proto;
    memset(&d,0,sizeof(d));d.ch=-1;
    if(proto<YMODEM) {
        if(ask_file(1))return;
        strcpy(name,t.filename);d.ch=open_download(name);
        if(d.ch<0) {file_error(d.ch);return;}
    } else strcpy(name,"QLTDOWN_");
    if(t.buflen>0) {
        d.capacity=(U32)t.buflen*1024UL;
        if(d.capacity<1024)d.capacity=1024;
        d.buffer=malloc(d.capacity);
        if(!d.buffer) {aux("No room for download buffer");finish_download(&d);return;}
    }
    transfer_mode(1);active=1;
next_file:
    d.used=d.received=d.written=d.date=0;d.first=1;d.length=-1;
    expected=proto>=YMODEM?0:1;crc=1;g=proto==YMODEMG;errors=0;got_eot=0;can_count=0;
start_request:
    start=mt_rclck();last_progress=0;
    if(progress("Waiting for sender...",d.received,d.length,start))goto cancelled;
    purge();request=g?'G':'C';tries=0;
request_again:
    if(++tries>5) {
        tries=1;
        if(request=='G') {g=0;request='C';}
        else {crc=0;request=NAK;}
    }
    tx_char(request,0);
    e=rxchar(request==NAK?500:250,&c);
    if(e==1) {
        if(request==NAK)goto retry_error;
        goto request_again;
    }
    if(e<0)goto remote_abort;
    if(c==CAN) {if(++can_count==2)goto remote_abort;goto request_again;}
    can_count=0;
    if(c!=SOH&&c!=STX&&c!=EOT)goto request_again;
    goto got_character;
wait_block:
    if(progress(NULL,d.received,d.length,start))goto cancelled;
    e=rxchar(500,&c);
    if(e==1)goto retry_error;
    if(e<0)goto remote_abort;
got_character:
    if(c==CAN) {if(++can_count==2)goto remote_abort;goto wait_block;}
    can_count=0;
    if(c==EOT) {
        if(!g&&!got_eot) {got_eot=1;tx_char(NAK,0);goto wait_block;}
        if(d.ch>=0) {
            e=d.used?flush_download(&d):0;if(e) {file_error(e);goto cancelled;}
            finish_download(&d);
        }
        tx_char(ACK,0);progress("Download OK",d.received,d.length,start);
        if(proto>=YMODEM)goto next_file;
        done=1;goto out;
    }
    if(c!=SOH&&c!=STX)goto wait_block;
    e=receive_block(c,crc,&n,&seq);
    if(e)goto retry_error;
    errors=0;
    if(seq!=(int)(expected&255)) {
        if(seq==(int)((expected-1)&255)) {if(!g)tx_char(ACK,0);goto wait_block;}
        if(expected==0&&d.received==0&&seq==1) {
            d.ch=open_download("QLTDOWN_");if(d.ch<0)goto cancelled;
            expected=1;
        } else {progress("FATAL ERROR: Synchronisation loss",d.received,d.length,start);goto cancelled;}
    }
    if(expected==0) {
        e=descriptor(&d,block+2,n);
        if(e==1) {tx_char(ACK,0);progress("Download OK",d.received,d.length,start);done=1;goto out;}
        if(e<0) {file_error(e);goto cancelled;}
        ++expected;if(!g)tx_char(ACK,0);goto start_request;
    }
    e=store_data(&d,block+2,n);if(e) {file_error(e);goto cancelled;}
    ++expected;if(!g)tx_char(ACK,0);goto wait_block;
retry_error:
    if(g) {progress("FATAL ERROR: Synchronisation loss",d.received,d.length,start);goto cancelled;}
    ++errors;sprintf(msg,e==-11?"Checksum/CRC error #%d":"Timeout #%d",errors);
    if(progress(msg,d.received,d.length,start))goto cancelled;
    if(errors>10) {if(!ask_yes("10X Transmit error. Retry"))goto cancelled;errors=0;}
    purge();tx_char(NAK,0);goto wait_block;
remote_abort:
    progress("Download aborted by remote",d.received,d.length,start);
cancelled:
    cancel_transfer();
out:
    finish_download(&d);if(d.buffer)free(d.buffer);
    if(active) {wait_frames(100);transfer_mode(0);}
    t.status=0;(void)done;
}
/* Wait for a receiver, requiring two CANs for cancellation. */
static int response(U8 *c,int timeout,U32 sent,long total,U32 start) {
    int e,cans=0,remaining=timeout;
    while(remaining>0) {
        if(progress(NULL,sent,total,start))return -24;
        e=rxchar(50,c);remaining-=50;
        if(e<0)return e;
        if(e>0)continue;
        if(*c==CAN) {if(++cans==2)return -24;continue;}
        return 0;
    }
    return 1;
}
void upload(void) {
    chanid_t ch;U8 header[64],data[1024],c;U32 length,date,sent,start;
    int proto,crc=0,g=0,n,got,seq,e,offset,first,finished=0,can_count=0,header_bytes;
    long total;
    proto=menu("",protocols,4,t.protocol);if(proto<0)return;t.protocol=proto;
    if(ask_file(0))return;
    ch=open_upload(t.filename);if(ch<0) {file_error(ch);return;}
    memset(header,0,sizeof(header));e=fs_headr(ch,-1,header,64);
    if(e<0) {file_error(e);io_close(ch);return;}
    length=be32(header);date=be32(header+52);total=(long)length;
    transfer_mode(1);start=0;last_progress=0;sent=0;
    aux("Waiting for receiver...");
    e=response(&c,3000,0,total,0);if(e)goto abort;
    crc=c=='C'||c=='G';g=c=='G';
    if(c!=NAK&&!crc)goto abort;
    start=mt_rclck();
    if(proto>=YMODEM) {
        memset(data,0,128);offset=t.name_offset;
        strcpy((char *)data,t.filename+offset);n=strlen((char *)data)+1;
        sprintf((char *)data+n,"%lu %lo",length+(header[5]?30UL:0UL),date?date-QL_EPOCH:0UL);
resend_descriptor:
        e=send_block(0,data,128,crc);if(e)goto abort;
        e=response(&c,3000,0,total,start);if(e)goto abort;
        if(c==NAK)goto resend_descriptor;
        if(c==ACK) {e=response(&c,3000,0,total,start);if(e)goto abort;}
        if(c!='C'&&c!='G')goto abort;
        crc=1;g=c=='G';
    }
    seq=1;first=1;
next_data:
    n=proto==XMODEM?128:1024;got=0;header_bytes=0;
    if(first&&header[5]) {
        header_bytes=30;memcpy(data,qdos_marker,18);data[18]=0;data[19]=15;memcpy(data+20,header+4,10);got=30;
    }
    first=0;
    while(got<n) {
        e=io_fstrg(ch,-1,data+got,(short)(n-got));
        if(e==-10)break;
        if(e<0) {file_error(e);goto abort;}
        if(!e)break;got+=e;
    }
    if(!got)goto end_file;
    if(got<=128)n=128;
    memset(data+got,26,n-got);
resend_data:
    if(progress(NULL,sent,total,start))goto abort;
    e=send_block(seq,data,n,crc);if(e)goto abort;
    if(!g) {
        e=response(&c,3000,sent,total,start);if(e)goto abort;
        if(c==NAK||c=='C') {if(c=='C')crc=1;goto resend_data;}
        if(c!=ACK)goto resend_data;
    } else {
        while(rxchar(0,&c)==0) {
            if(c==CAN) {if(++can_count==2)goto abort;}
            else can_count=0;
        }
    }
    sent+=got-header_bytes;++seq;goto next_data;
end_file:
    tx_char(EOT,0);e=response(&c,3000,sent,total,start);if(e)goto abort;
    if(c==NAK)goto end_file;
    if(c!=ACK)goto end_file;
    if(proto>=YMODEM) {
        e=response(&c,3000,sent,total,start);if(e)goto abort;
        if(c!='C'&&c!='G'&&c!=NAK)goto abort;
        crc=c!=NAK;memset(data,0,128);
end_batch:
        send_block(0,data,128,crc);
        e=response(&c,3000,sent,total,start);if(e)goto abort;
        if(c==NAK)goto end_batch;
        if(c!=ACK&&c!='G')goto abort;
    }
    progress("Upload OK",sent,total,start);finished=1;goto out;
abort:
    cancel_transfer();progress(e==1?"Xmit Timeout - Aborted":"Aborted",sent,total,start);
out:
    io_close(ch);transfer_mode(0);t.status=0;(void)finished;
}
