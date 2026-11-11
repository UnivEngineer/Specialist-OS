"""Fill a disposable H: FAT16 image to 250 root entries for NC sort tests."""
import hashlib
import json
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[2]


def make_full_h(destination, target=250):
    original = (ROOT/'images/FlashDisk_emu80_seed.bin').read_bytes()
    image = bytearray(original)
    tree = runpy.run_path(str(ROOT/'scripts/tests/check-nested-image.py'))['read_tree']
    old_files, old_dirs = tree(original)
    u16 = lambda at: struct.unpack_from('<H', image, at)[0]
    sector, spc, reserved, copies = u16(11), image[13], u16(14), image[16]
    fat_sectors, root_entries = u16(22), u16(17)
    fat = reserved*sector
    root = (reserved+copies*fat_sectors)*sector
    data = root+((root_entries*32+sector-1)//sector)*sector
    cluster_size = sector*spc
    clusters = (u16(19)*sector-data)//cluster_size
    original_root_entries = root_entries
    if root_entries < target+1:
        # The stock H: seed has only 128 root slots. Grow the root in this
        # disposable image; preserve cluster numbers and move the data region.
        root_entries = ((target+1+15)//16)*16
        new_data = root+((root_entries*32+sector-1)//sector)*sector
        new_clusters = (u16(19)*sector-new_data)//cluster_size
        if any(u16(fat+c*2) for c in range(new_clusters+2, clusters+2)):
            raise ValueError('Cannot grow root: occupied clusters at the image tail')
        delta = new_data-data
        image[new_data:] = image[data:len(image)-delta]
        image[data:new_data] = bytes(delta)
        struct.pack_into('<H', image, 17, root_entries)
        data, clusters = new_data, new_clusters
    active, free, names = [], [], set()
    for i in range(root_entries):
        at = root+i*32
        entry = image[at:at+32]
        if entry[0] in (0, 0xe5):
            free.append(at)
        elif not entry[11] & 8:
            active.append(at)
            names.add(bytes(entry[:11]).upper())
    needed = target-len(active)
    if needed < 12 or len(free) < needed:
        raise ValueError('Not enough room to build the 250-entry test root')
    candidate = 2

    def descriptor(name, attr=0x20, cluster=0):
        base, _, ext = name.partition('.')
        out = bytearray(32)
        out[:11] = (base.ljust(8)+ext.ljust(3)).encode('ascii')
        out[11] = attr
        struct.pack_into('<H', out, 26, cluster)
        return out

    added = []
    for i in range(needed):
        if i < 12:
            name = f'SRTDIR{i//3:02}.'+('ASM', 'TXT', 'RKS')[i % 3]
            while candidate < clusters+2 and u16(fat+candidate*2):
                candidate += 1
            if candidate == clusters+2:
                raise ValueError('No free cluster for synthetic directory')
            cluster = candidate
            for copy in range(copies):
                struct.pack_into('<H', image, fat+copy*fat_sectors*sector+cluster*2, 0xffff)
            content = bytearray(cluster_size)
            dot = bytearray(32); dot[:11] = b'.          '; dot[11] = 16
            struct.pack_into('<H', dot, 26, cluster)
            parent = bytearray(32); parent[:11] = b'..         '; parent[11] = 16
            content[:32], content[32:64] = dot, parent
            at = data+(cluster-2)*cluster_size
            image[at:at+cluster_size] = content
            entry = descriptor(name, 16, cluster)
        else:
            j = i-12
            name = f'SRT{j//10:05}.'+('TXT','ASM','BIN','DAT','CFG','INI','LOG','OBJ','MAP','HLP')[j % 10]
            entry = descriptor(name)
        if bytes(entry[:11]).upper() in names:
            raise ValueError('Synthetic name collision: '+name)
        names.add(bytes(entry[:11]).upper())
        at = free[i]
        image[at:at+32] = entry
        added.append(dict(name=name, directory=bool(entry[11]&16)))
    new_files, new_dirs = tree(bytes(image))
    assert all(new_files[name] == payload for name,payload in old_files.items())
    assert all(new_dirs[name] == cluster for name,cluster in old_dirs.items())
    assert len(active)+len(added) == target
    destination.write_bytes(image)
    destination.with_suffix('.json').write_text(json.dumps(dict(
        original_entries=len(active), final_entries=target, added=added,
        original_root_slots=original_root_entries, root_slots=root_entries,
        seed_sha256=hashlib.sha256(original).hexdigest(),
        image_sha256=hashlib.sha256(image).hexdigest()), indent=2)+'\n')
    return destination


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    make_full_h(args.output)
    print('H: image validated: 250 root entries, 12 empty directories with extensions.')
