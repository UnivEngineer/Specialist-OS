"""Сверка сохранённых сцен родного Monitor-2 и MXOS RUN -mon без оценки ввода."""
import collections
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'.local/game-campaign-python'))
from PIL import Image, ImageChops

BASE=ROOT/'docs/compat/game-campaign'
NATIVE=BASE/'native-monitor'

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    index=json.loads((NATIVE/'catalog-index.json').read_text(encoding='utf8'))
    comparison={};counts=collections.Counter()
    for h,case in index.items():
        out={'mxos_trial':case['mxos_trial'],'mxos_evidence':case['mxos_evidence'],'frames':{}}
        for name in ('title','before','after'):
            native=NATIVE/case['evidence']/(name+'.png')
            other=BASE/'trials'/case['mxos_trial']/case['mxos_evidence']/(name+'.png')
            if not native.is_file() or not other.is_file():continue
            a=Image.open(native).convert('RGB');b=Image.open(other).convert('RGB')
            ma=a.convert('L').point(lambda x:255 if x else 0);mb=b.convert('L').point(lambda x:255 if x else 0)
            eq=ma.size==mb.size and ImageChops.difference(ma,mb).getbbox() is None
            result=dict(native_sha256=sha(native),mxos_sha256=sha(other),native_size=a.size,mxos_size=b.size,
                        same_monochrome_mask=eq,
                        native_colors=len(a.getcolors(a.width*a.height) or []),
                        mxos_colors=len(b.getcolors(b.width*b.height) or []))
            out['frames'][name]=result;counts[name+('_mask_equal' if eq else '_mask_different')]+=1
        if not out['frames']:counts['no_common_screenshot']+=1
        comparison[h]=out
    (NATIVE/'catalog-comparisons.json').write_text(json.dumps(comparison,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print('rows',len(comparison),'frame observations',dict(counts))
    print('Разные кадры могут отражать время/ввод/палитру; это не доказательство дефекта.')

if __name__=='__main__':main()
