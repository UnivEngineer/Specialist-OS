"""Проверка сообщений ROM по зафиксированным пикселям R/G."""
import collections
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'.local/game-campaign-python'))
from PIL import Image

OUT=ROOT/'docs/compat/game-campaign/native-monitor'
ERROR='a3446787a306d39529296c40308b7e9bec088408b49514af389422f7c7ca3806'
SUM='b6cef86bf189bd473003388b2dce03eb929bb874a80e4de4e66d1f06351e7629'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    path=OUT/'tape-catalog.json'
    data=json.loads(path.read_text(encoding='utf8'))
    for h,item in data.items():
        read=OUT/item.get('evidence','-')/'read.png'
        if not read.is_file():continue
        image=Image.open(read).convert('L').point(lambda x:255 if x else 0)
        if image.size!=(384,256):continue
        err=sha(image.crop((0,30,120,38)).tobytes())
        total=sha(image.crop((0,30,105,38)).tobytes())
        source=ROOT/item.get('source','')
        raw=source.read_bytes() if source.is_file() else b''
        if item.get('recipe')=='archive_as_named' and raw.startswith(bytes.fromhex('708f828f')):
            item['source_format']='LIK'
            if 'checksum_matches' in item:
                item['raw_header_checksum_matches']=item.pop('checksum_matches')
            item['checksum_status']='неприменимо: первый заголовок принадлежит LIK, не RKS'
        elif item.get('checksum_matches') is True:item['checksum_status']='valid'
        elif item.get('checksum_matches') is False:item['checksum_status']='missing_or_truncated' if len(bytes.fromhex(item.get('checksum_tail','')))<2 else 'invalid'
        else:item['checksum_status']='unconfirmed'
        item.update(rom_message_crop_sha256=err,rom_sum_crop_sha256=total,
                    rom_input_error_visible=err==ERROR,rom_checksum_message_visible=total==SUM)
        item.setdefault('classification_before_screen_review',item.get('classification'))
        if item.get('input_complete') and err==ERROR:
            item['classification']='rom_read_error'
            item['g_entry_confirmed']=False
            item['screen_review']='После R видна строка ОШИБКА ПРИ ВВОДЕ; на снимке команды R и G различимы.'
        elif item.get('input_complete') and total!=SUM:
            item['classification']='rom_read_status_uncertain'
            item['g_entry_confirmed']=False
            item['screen_review']='Возврат в клавиатурную процедуру есть, но строка контрольной суммы не распознана.'
        else:
            item['screen_review']='Строка контрольной суммы ROM распознана.' if total==SUM else 'Завершение R не установлено.'
    tmp=path.with_suffix('.tmp');tmp.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf8');tmp.replace(path)
    print('rows',len(data),'classifications',dict(collections.Counter(x.get('classification') for x in data.values())))
    print('checksum',dict(collections.Counter(x.get('checksum_status') for x in data.values())))
    print('visible ROM read error',sum(x.get('rom_input_error_visible',False) for x in data.values()))
    print('G entry with exact body',sum(x.get('classification')=='rom_read_and_g_entry' and x.get('body_matches_at_g') for x in data.values()))

if __name__=='__main__':main()
