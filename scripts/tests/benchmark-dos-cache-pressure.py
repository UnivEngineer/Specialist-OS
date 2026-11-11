"""Нагрузка на реальный кэш DOS в модели 8080; синтетический секторный драйвер."""
import argparse, hashlib, json, re, struct, subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
TICKS=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--report',type=Path,required=True)
 p.add_argument('--kernel',type=Path,default=R/'bin/banked/KERNEL.bin')
 a=p.parse_args(); o=a.report.resolve();o.mkdir(parents=True,exist_ok=True)
 kernel=a.kernel.resolve(); syms={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',kernel.with_suffix('.sym').read_text('utf-8'),re.M)}
 cases={'single-24':[(0,n) for n in range(24)], 'balanced-12-12':[(d,n) for n in range(12) for d in (0,1)], 'asymmetric-24-4':[(0,n) for n in range(24)]+[(1,n) for n in range(4)], 'overflow-20-20':[(d,n) for n in range(20) for d in (0,1)], 'three-disks-4':[(d,n) for n in range(4) for d in (0,1,7)], 'mru-one':[(0,1)]*32, 'two-disks-write-8-8':[(d,n) for n in range(8) for d in (0,1)]}
 rows=[]
 for name, sequence in cases.items():
  seq=sequence*8
  src='\n'.join(f'{n} EQU {syms[n]}' for n in ('v_drive','v_diskInfo.sectorShift','v_diskInfo.sectorSize','v_diskInfo.blockSize','v_diskInfo.fatCount','v_logicalCapacity','v_logicalShift','j_diskDriver','resetAllSectorCaches','configureSectorCache','saveSectorCache','selectCacheForDrive','getSectorAddressInCache'))+f'''
 ORG 08000h
 push hl
 push de
 push bc
 ld a,b
 cp 2
 jp nz,mockwrite
 ld hl,(08100h)
 inc hl
 ld (08100h),hl
 pop bc
 pop de
 pop hl
 push hl
 push bc
 ld a,(v_drive)
 xor e
 ld b,0
mockfill:
 ld (hl),a
 inc hl
 dec b
 jp nz,mockfill
 pop bc
 pop hl
 or a
 ret
mockwrite:
 ld a,(v_drive)
 xor e
 cp (hl)
 jp z,writevalid
 ld a,1
 ld (08108h),a
writevalid:
 ld hl,(08102h)
 inc hl
 ld (08102h),hl
 pop bc
 pop de
 pop hl
 or a
 ret
 BLOCK 08200h-$,0
 ORG 08200h
 ld sp,0F480h
 call resetAllSectorCaches
 ld hl,08000h
 ld (j_diskDriver+1),hl
 ld hl,256
 ld (v_diskInfo.sectorSize),hl
 ld (v_diskInfo.blockSize),hl
 ld a,8
 ld (v_diskInfo.sectorShift),a
 ld a,1
 ld (v_diskInfo.fatCount),a
 ld hl,256
 ld (v_logicalCapacity),hl
 xor a
 ld (v_logicalShift),a
 ld a,0FFh
 ld (v_drive),a
 jp run
measure:
 ld sp,0F480h
 ld hl,0
 ld (08100h),hl
 ld (08102h),hl
run:
 ld hl,0C000h
 ld (position),hl
 ld hl,{len(seq)}
 ld (remaining),hl
next:
 ld hl,(position)
 ld a,(hl)
 ld (drive),a
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 ld (position),hl
 ld (sector),de
 ld hl,v_drive
 cp (hl)
 jp z,selected
 call saveSectorCache
 jp c,failed
 ld a,(drive)
 ld (v_drive),a
 call selectCacheForDrive
 call configureSectorCache
selected:
 ld hl,(sector)
 ex de,hl
 ld a,{1 if name == 'two-disks-write-8-8' else 0}
 call getSectorAddressInCache
 jp c,failed
 ld a,(drive)
 ld de,(sector)
 xor e
 cp (hl)
 jp nz,failed
 ld hl,(remaining)
 dec hl
 ld (remaining),hl
 ld a,h
 or l
 jp nz,next
 call saveSectorCache
 jp c,failed
 jp 08FFFh
failed:
 ld a,1
 ld (08108h),a
 jp 08FFFh
position: DW 0
remaining: DW 0
sector: DW 0
drive: DB 0
'''
  # Все пары загрузки DE из памяти оформлены инструкциями 8080.
  src=src.replace(' ld (sector),de',' ex de,hl\n ld (sector),hl').replace(' ld de,(sector)',' push hl\n ld hl,(sector)\n ex de,hl\n pop hl')
  asm=o/(name+'.asm');asm.write_text(src,encoding='utf-8')
  binary=o/(name+'.bin');sym=o/(name+'.sym')
  subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(binary),'--sym='+str(sym),str(asm)],check=True,capture_output=True)
  ss={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',sym.read_text(),re.M)}
  memory=bytearray(65536); data=kernel.read_bytes();memory[0x100:0x100+len(data)]=data
  data=binary.read_bytes();memory[0x8000:0x8000+len(data)]=data
  requests=b''.join(bytes((d,))+struct.pack('<H',n) for d,n in seq);memory[0xC000:0xC000+len(requests)]=requests
  for phase,pc in [('warm',0x8200),('measured',ss['measure'])]:
   inp=o/'in.bin';out=o/'out.bin';inp.write_bytes(memory)
   result=subprocess.run([str(TICKS),'-m8080','-pc',f'{pc:x}','-end','8fff','-counter','50000000','-output',str(out),str(inp)],check=True,capture_output=True,text=True)
   data=out.read_bytes();memory=bytearray(data[:65536]);assert int.from_bytes(data[65542:65544],'little')==0x8FFF and memory[0x8108]==0,(name,phase)
  cycles=int(result.stdout.strip().splitlines()[-1]);reads,writes=struct.unpack_from('<HH',memory,0x8100)
  row=dict(case=name,requests=len(seq),cycles=cycles,msAt2MHz=cycles/2000,reads=reads,writes=writes,hits=len(seq)-reads)
  rows.append(row);print(row,flush=True)
 (o/'result.json').write_text(json.dumps(dict(kernelSha256=hashlib.sha256(kernel.read_bytes()).hexdigest(),driver='synthetic 256-byte fill; real DOS cache code; Intel 8080 model',rows=rows),indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()

