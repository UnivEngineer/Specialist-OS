"""Контактные листы игровых проб; оценка содержимого остаётся за наблюдателем."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / '.local/game-campaign-python'))
from PIL import Image, ImageDraw


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT / 'docs/compat/game-campaign')
    parser.add_argument('--columns', type=int, default=3)
    parser.add_argument('--unreviewed', action='store_true')
    args = parser.parse_args()
    out = args.root / ('contact-sheets-new' if args.unreviewed else 'contact-sheets')
    out.mkdir(parents=True, exist_ok=True)
    reviews_path = args.root / 'reviews.json'
    reviews = json.loads(reviews_path.read_text(encoding='utf-8')) if reviews_path.exists() else {}
    index = []
    tiles = []
    for result in sorted((args.root / 'trials').glob('*/result.json')):
        try:
            data = json.loads(result.read_text(encoding='utf-8'))
            evidence = result.parent / data['evidence']
            frames = {name: evidence / (name + '.png') for name in ('title', 'before', 'idle', 'after', 'obstacle', 'exit-observation')}
            frames = {name: path for name, path in frames.items() if path.is_file()}
            if not frames:
                continue
            entry = dict(id=data['id'], file=data.get('file'), evidence=data['evidence'], frames={name: digest(path) for name, path in frames.items()})
            if args.unreviewed and reviews.get(data['id'], {}).get('frame_sha256') == entry['frames']:
                continue
            index.append(entry)
            for name, path in frames.items():
                image = Image.open(path).convert('RGB')
                image.thumbnail((384, 256))
                tile = Image.new('RGB', (400, 300), 'white')
                tile.paste(image, ((400-image.width)//2, 32))
                draw = ImageDraw.Draw(tile)
                draw.text((4, 2), f'{len(index):04d} {name} {data.get("profile", "")}', fill='black')
                draw.text((4, 282), path.name + ' ' + digest(path)[:12], fill='black')
                tiles.append(tile)
        except (KeyError, ValueError, OSError) as error:
            print(f'{result}: {error}', file=sys.stderr)
    page_size = args.columns * 5
    for offset in range(0, len(tiles), page_size):
        batch = tiles[offset:offset + page_size]
        sheet = Image.new('RGB', (400*args.columns, 300*math.ceil(len(batch)/args.columns)), '#bbb')
        for i, tile in enumerate(batch):
            sheet.paste(tile, ((i%args.columns)*400, (i//args.columns)*300))
        sheet.save(out / f'page-{offset//page_size+1:04d}.png')
    (out / 'index.json').write_text(json.dumps(index, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f'{len(index)} проб, {len(tiles)} кадров, {(len(tiles)+page_size-1)//page_size} листов')


if __name__ == '__main__':
    main()
