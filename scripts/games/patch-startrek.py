"""StarTrek: erase with SPACE instead of the printable DEL glyph."""
from pathlib import Path
from hashlib import sha256
import argparse,runpy,re,sys
R=Path(__file__).resolve().parents[2]
SOURCE=R/'assets/games/Startrek.rks'
TARGET=R/'assets/games/StarTrek/StarTrek-mx.exe'
sys.path.insert(0,str(R/'scripts/images'))
from executable import Image,encode,decode
SHA='44ea1f0dd848c6b9f1bb8503ce7336a57ef0d6716c75988d807e1825c97e52da'
checksum=runpy.run_path(str(R/'scripts/games/repair-rks-checksums.py'))['checksum']
def patch():
 b=SOURCE.read_bytes();assert sha256(b).hexdigest()==SHA
 assert b[4+0x198d:4+0x1998]==bytes.fromhex('0e08cd37c00e7fcd37c0c9')
 data=bytearray(b[4:-2]);data[0x1993]=0x20
 # Append the entry stub so original code/data addresses remain unchanged.
 # Start with the original stack before calling the MXOS encoding service.
 abi=(R/'source/include/mxos.inc').read_text(encoding='utf8')
 encoding=int(re.search(r'^bios_setFontEncoding\s*=\s*0([0-9A-Fa-f]+)h',abi,re.M)[1],16)
 flags=int(re.search(r'^IMAGE_FLAG_MAIN\s*=\s*0([0-9A-Fa-f]+)h',abi,re.M)[1],16)
 entry=len(data)
 assert data[:3]==bytes.fromhex('31ff7f')
 data+=bytes.fromhex('31ff7f3e01cd')+encoding.to_bytes(2,'little')+bytes.fromhex('c30000')
 image=Image(bytes(data),0,entry,flags)
 result=encode(image,'exe');assert decode(result,'exe')==image
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');a=p.parse_args();b=patch()
 if a.check:assert TARGET.read_bytes()==b
 else:TARGET.parent.mkdir(parents=True,exist_ok=True);TARGET.write_bytes(b)
 print('StarTrek: C=7F -> C=20 at 1993h; native MAIN EXE; KOI7 entry; checksums valid; original preserved')
