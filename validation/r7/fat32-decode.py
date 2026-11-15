#!/usr/bin/env python3
"""Independent read-only FAT32 ownership/content judge (no kernel code)."""
import hashlib, json, mmap, struct, sys
from pathlib import Path

def inspect(path):
    errors=[]; files={}; dirs={}; owners={}
    with open(path,'rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as d:
        u16=lambda o:struct.unpack_from('<H',d,o)[0]
        u32=lambda o:struct.unpack_from('<I',d,o)[0]
        assert d[510:512]==b'\x55\xaa' and u16(11)==512
        spc=d[13]; nf=d[16]; res=u16(14); spf=u32(36); total=u32(32)
        root=u32(44); ext=u16(40); active=(ext&15) if ext&128 else 0
        start=res+nf*spf; count=(total-start)//spc; cb=spc*512
        assert 65525<=count<0xffffff0 and total*512<=len(d) and active<nf
        fats=[[(x[0]&0xfffffff) for x in struct.iter_unpack('<I',d[(res+i*spf)*512:(res+(i+1)*spf)*512])] for i in range(nf)]
        fat=fats[active]
        if not ext&128 and any(x!=fat for x in fats):errors.append('mirrored FAT mismatch')
        def chain(c,who):
            out=[];seen=set()
            while True:
                if not 2<=c<count+2:errors.append(f'{who}: out of range {c}');break
                if c in seen:errors.append(f'{who}: cycle {c}');break
                seen.add(c);out.append(c)
                if c in owners:errors.append(f'{who}: crosslink {c} with {owners[c]}');break
                owners[c]=who;nx=fat[c]
                if nx>=0xffffff8:break
                if nx<2 or nx>=0xffffff0:errors.append(f'{who}: invalid link {c}->{nx}');break
                c=nx
            return out
        def cluster(c):return d[(start+(c-2)*spc)*512:(start+(c-1)*spc)*512]
        def first(e):return (struct.unpack_from('<H',e,20)[0]&4095)<<16|struct.unpack_from('<H',e,26)[0]
        def walk(c,parent,path,depth=0):
            if depth>16:errors.append(path+': nesting');return
            ch=chain(c,path or '/');dirs[path or '/']=ch
            raw=b''.join(cluster(x) for x in ch)
            if path:
                for off,expected,tag in [(0,c,b'.          '),(32,parent,b'..         ')]:
                    e=raw[off:off+32]
                    if len(e)!=32 or e[:11]!=tag or first(e)!=expected:errors.append(path+': dot entry')
            seen=set();lfn=[]
            for off in range(0,len(raw),32):
                e=raw[off:off+32]
                if not e[0]:
                    if lfn:errors.append(path+': orphan LFN')
                    break
                if e[0]==229:lfn=[];continue
                if e[11]==15:lfn.append(e);continue
                if e[11]&8:lfn=[];continue
                if e[:1]==b'.':continue
                name=e[:8].rstrip().decode('latin1');ex=e[8:11].rstrip().decode('latin1')
                if ex:name+='.'+ex
                if lfn:
                    n=len(lfn);checksum=0
                    for b in e[:11]:checksum=(((checksum&1)<<7)+(checksum>>1)+b)&255
                    valid=all(q[0]==((n-i)|(64 if i==0 else 0)) and q[13]==checksum and not q[12] and q[26:28]==b'\0\0' for i,q in enumerate(lfn))
                    if not valid:errors.append(path+'/'+name+': invalid LFN')
                    lfn=[]
                if name in seen:errors.append(path+': duplicate '+name)
                seen.add(name);p=path+'/'+name;c1=first(e);sz=struct.unpack_from('<I',e,28)[0]
                if e[11]&16:
                    if sz:errors.append(p+': directory size')
                    if c1 in owners:errors.append(p+': directory alias');continue
                    walk(c1,0 if c==root else c,p,depth+1)
                else:
                    ch1=chain(c1,p) if c1 else []
                    if len(ch1)!=(sz+cb-1)//cb:errors.append(p+': size/chain mismatch')
                    h=hashlib.sha256();remaining=sz
                    for c2 in ch1:
                        b=cluster(c2)[:remaining];h.update(b);remaining-=len(b)
                    files[p]={'size':sz,'sha256':h.hexdigest(),'chain':ch1}
            if lfn:errors.append(path+': orphan LFN')
        walk(root,0,'')
        lost=[c for c in range(2,count+2) if fat[c] and fat[c]!=0xffffff7 and c not in owners]
        if lost:errors.append(f'orphan clusters: {len(lost)} first={lost[:8]}')
        free=sum(x==0 for x in fat[2:count+2]);fi=u16(48);backup=u16(50)
        if fi!=65535:
            for sector in [fi]+([backup+fi] if backup not in (0,65535) else []):
                o=sector*512
                if (u32(o),u32(o+484),u32(o+508))!=(0x41615252,0x61417272,0xaa550000):errors.append('FSInfo signature')
                if u32(o+488) not in (0xffffffff,free):errors.append('FSInfo free count')
        return dict(errors=errors,files=files,directories=dirs,clusters=count,free=free,spc=spc,fats=nf,active=active,root=root)

if __name__=='__main__':
    result=inspect(Path(sys.argv[1]));print(json.dumps(result,indent=2));sys.exit(bool(result['errors']))
