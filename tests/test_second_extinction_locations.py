#!/usr/bin/env python3
"""Exercise world.bin names and header-tag mappings for GTOC containers."""
import pathlib
import sqlite3
import struct
import subprocess
import sys
import tempfile


def rotate(value, amount):
    return ((value << amount) | (value >> (64 - amount))) & ((1 << 64) - 1)


def murmur(name):
    data = name.encode()
    mask = (1 << 64) - 1
    c1, c2 = 0x87c37b91114253d5, 0x4cf5ad432745937f
    h1 = h2 = 0
    for pos in range(0, len(data) // 16 * 16, 16):
        k1, k2 = struct.unpack_from('<QQ', data, pos)
        h1 ^= rotate(k1 * c1 & mask, 31) * c2 & mask
        h1 = (rotate(h1, 27) + h2) & mask
        h1 = (h1 * 5 + 0x52dce729) & mask
        h2 ^= rotate(k2 * c2 & mask, 33) * c1 & mask
        h2 = (rotate(h2, 31) + h1) & mask
        h2 = (h2 * 5 + 0x38495ab5) & mask
    tail = data[len(data) // 16 * 16:]
    if len(tail) > 8:
        h2 ^= rotate(int.from_bytes(tail[8:], 'little') * c2 & mask, 33) * c1 & mask
    if tail:
        h1 ^= rotate(int.from_bytes(tail[:8], 'little') * c1 & mask, 31) * c2 & mask
    h1 ^= len(data)
    h2 ^= len(data)
    h1 = (h1 + h2) & mask
    h2 = (h2 + h1) & mask

    def finish(value):
        value = (value ^ (value >> 33)) * 0xff51afd7ed558ccd & mask
        value = (value ^ (value >> 33)) * 0xc4ceb9fe1a85ec53 & mask
        return value ^ (value >> 33)

    return (finish(h1) + finish(h2)) & mask

def lookup3(name):
    data = name.encode()
    mask = (1 << 32) - 1

    def rotate32(value, count):
        return ((value << count) | (value >> (32 - count))) & mask

    a = b = c = (0xdeadbeef + len(data)) & mask
    while len(data) > 12:
        x, y, z = struct.unpack_from('<III', data)
        a, b, c = (a + x) & mask, (b + y) & mask, (c + z) & mask
        a = ((a-c) ^ rotate32(c, 4)) & mask; c = (c+b) & mask
        b = ((b-a) ^ rotate32(a, 6)) & mask; a = (a+c) & mask
        c = ((c-b) ^ rotate32(b, 8)) & mask; b = (b+a) & mask
        a = ((a-c) ^ rotate32(c, 16)) & mask; c = (c+b) & mask
        b = ((b-a) ^ rotate32(a, 19)) & mask; a = (a+c) & mask
        c = ((c-b) ^ rotate32(b, 4)) & mask; b = (b+a) & mask
        data = data[12:]
    if not data:
        return c
    x, y, z = struct.unpack('<III', data.ljust(12, b'\0'))
    a, b, c = (a+x) & mask, (b+y) & mask, (c+z) & mask
    c = ((c ^ b) - rotate32(b, 14)) & mask
    a = ((a ^ c) - rotate32(c, 11)) & mask
    b = ((b ^ a) - rotate32(a, 25)) & mask
    c = ((c ^ b) - rotate32(b, 16)) & mask
    a = ((a ^ c) - rotate32(c, 4)) & mask
    b = ((b ^ a) - rotate32(a, 14)) & mask
    return ((c ^ b) - rotate32(b, 24)) & mask


def world_manifest(locations):
    data = bytearray(struct.pack('<4sIIIHH', b'RTPC', 1, 0, 20, 0, len(locations)))
    children = len(data)
    data.extend(b'\0' * (12 * len(locations)))
    for index, (stem, suffixes, kind) in enumerate(locations):
        data.extend(b'\0' * (-len(data) % 4))
        properties = len(data)
        data.extend(b'\0' * 27)
        file_offset = len(data)
        data.extend(stem.encode() + b'\0')
        suffix_offset = len(data)
        data.extend(suffixes.encode() + b'\0')
        struct.pack_into('<IIHH', data, children + index * 12, 1, properties, 3, 0)
        struct.pack_into('<IIB', data, properties, 0x6F57DC91, file_offset, 3)
        struct.pack_into('<IIB', data, properties + 9, 0xA64E1E84, suffix_offset, 3)
        struct.pack_into('<IIB', data, properties + 18, 0xF5B03AC8, kind, 1)
    return data


def nested_rtpc(value):
    data = bytearray(struct.pack('<4sIIIHH', b'RTPC', 1, 1, 20, 1, 0))
    data.extend(struct.pack('<IIB', 1, 29, 3))
    data.extend(value.encode() + b'\0')
    return data

def write_tab(initial, entries):
    arc = bytearray()
    tab = bytearray(struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, len(entries), 0, 0, 0, 0))
    for name, content in sorted(entries, key=lambda item: murmur(item[0])):
        tab.extend(struct.pack('<QIIIHBB', murmur(name), len(arc), len(content), len(content), 0, 0, 0))
        arc.extend(content)
    (initial / 'game0.arc').write_bytes(arc)
    (initial / 'game0.tab').write_bytes(tab)


archive_name = 'worlds/arctic/global/bookmarks_gen.bl'
member_name = 'worlds/arctic/global/bookmarks_gen.blo'
assert murmur(archive_name) == 0x9FA86CF330CAE1AB
assert murmur('locations/world.bin') == 0x0BB726490BE51400

executable = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='apex-second-extinction-locations-') as tmp:
    root = pathlib.Path(tmp)
    work = root / 'bin'
    work.mkdir()
    initial = root / 'game/archives_win64/initial'
    initial.mkdir(parents=True)
    strings = root / 'strings' / 'second_extinction'
    strings.mkdir(parents=True)
    texture_name = 'textures/roads/asphalt/asphalt_detail_03_dif.ddsc'
    texture_slice = 'textures/roads/asphalt/asphalt_detail_03_dif.atx2'
    (strings / 'filelist.txt').write_text(texture_name + '\n')
    locations = [
        ('worlds/arctic/global/bookmarks_gen', '.bl', 2),
        ('worlds/arctic/global/event', '.bl', 1),
        ('worlds/arctic/locations/dual', '.nl .fl', 0),
        ('worlds/arctic/locations/near_only', '.nl', 0),
    ]
    payload = nested_rtpc('NestedBookmark')
    unknown_payload = nested_rtpc('TagOnlyNested')
    tag = 0x725ABB72
    unknown_tag = 0xD42E75F5
    unknown_archive = 'worlds/arctic/global/unlisted.bl'
    unknown_member = 'worlds/arctic/global/unlisted.blo'
    unknown_member_extra = 'worlds/arctic/global/unlisted_extra.blo'
    extra_payload = nested_rtpc('TagOnlySecondMember')
    missing_entity = 'editor/entities/test/regen_drop.ee'
    installed_entity = 'editor/entities/test/installed.ee'
    external_entity = 'editor/entities/test/external.ee'
    mismatched_entity = 'editor/entities/test/different.ee'
    entity_payload = nested_rtpc('InstalledEntity')
    missing_tag, installed_tag = 0xA5941E39, 0xB723D504
    records = [
        (archive_name, tag, [(member_name, payload, 4)]),
        (unknown_archive, unknown_tag, [
            (unknown_member, unknown_payload, 4),
            (unknown_member_extra, extra_payload, 4 + len(unknown_payload)),
        ]),
        (missing_entity, missing_tag, [('editor/entities/test/regen_drop.epe_adf', payload, 4)]),
        (installed_entity, installed_tag, [('editor/entities/test/installed.epe', entity_payload, 4)]),
        (external_entity, 0x6CA42130, [('editor/entities/test/external.epe_adf', payload, 0)]),
        (mismatched_entity, 0xBF304235, [('editor/entities/test/wrong.epe_adf', payload, 4)]),
    ]
    gtoc = bytearray(struct.pack('<4sI', b'GT0C', len(records)))
    references = []
    for container, header, members in records:
        gtoc.extend(struct.pack('<III', lookup3(container), header, len(members)))
        for member, content, member_offset in members:
            reference = len(gtoc)
            gtoc.extend(struct.pack('<II', 0, member_offset))
            references.append((reference, member, content))
    for reference, member, content in references:
        struct.pack_into('<I', gtoc, reference, len(gtoc) - reference)
        gtoc.extend(struct.pack('<III', lookup3(member), 0, len(content)))
        gtoc.extend(member.encode() + b'\0')
        gtoc.extend(b'\0' * (-len(gtoc) % 4))
    entries = [
        ('locations/world.bin', world_manifest(locations)),
        (archive_name, struct.pack('<I', tag) + payload),
        (unknown_archive, struct.pack('<I', unknown_tag) + unknown_payload + extra_payload),
        (installed_entity, struct.pack('<I', installed_tag) + entity_payload),
        ('worlds/arctic/global/event.bl', b'event archive'),
        ('worlds/arctic/locations/dual.nl', b'near archive'),
        ('worlds/arctic/locations/dual.fl', b'far archive'),
        ('worlds/arctic/locations/near_only.nl', b'near only archive'),
        ('worlds/arctic/locations/near_only.fl', b'not declared in world.bin'),
        (texture_name, b'AVTX' + bytes(12)),
        (texture_slice, b'streamed texture slice'),
        ('sarc.0.gtoc', gtoc),
    ]
    write_tab(initial, entries)

    result = subprocess.run([executable, str(root / 'game')], cwd=work, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    with sqlite3.connect(root / 'second_extinction_hashes.db') as db:
        named = {name: (size, parent) for name, size, parent in db.execute(
            'SELECT name,size,parent FROM files WHERE name IS NOT NULL')}
        for name, content in entries:
            if name == unknown_archive or name == 'worlds/arctic/locations/near_only.fl':
                assert name not in named
            elif name.endswith(('.bl', '.nl', '.fl')):
                assert named[name] == (len(content), 0), (name, named.get(name))
        archive_row = db.execute('SELECT murmur FROM files WHERE lookup3=?', (0x37B28D6A,)).fetchone()
        assert archive_row == (murmur(archive_name) - 2**64,), archive_row
        unknown_row = db.execute('SELECT murmur FROM files WHERE lookup3=?',
                                 (lookup3(unknown_archive),)).fetchone()
        expected_unknown = murmur(unknown_archive)
        expected_unknown = expected_unknown if expected_unknown < 2**63 else expected_unknown - 2**64
        assert unknown_row == (expected_unknown,), unknown_row
        for name in (missing_entity, installed_entity):
            assert db.execute('SELECT v FROM kv WHERE lookup3=? AND v=?',
                              (lookup3(name), name)).fetchone() == (name,)
        assert db.execute('SELECT name FROM files WHERE lookup3=?',
                          (lookup3(missing_entity),)).fetchone() is None
        assert db.execute('SELECT name,size,parent FROM files WHERE lookup3=?',
                          (lookup3(installed_entity),)).fetchone() == (
                              installed_entity, 4 + len(entity_payload), 0)
        for name in (external_entity, 'editor/entities/test/wrong.ee'):
            assert db.execute('SELECT v FROM kv WHERE lookup3=?',
                              (lookup3(name),)).fetchone() is None
        assert f'Unable to find container for {missing_tag}|{lookup3(missing_entity)}' in result.stdout + result.stderr
        assert named[texture_slice][1] == 0, named[texture_slice]
        assert f'File with different parent hash already exists "{texture_slice}"' not in result.stdout + result.stderr
        assert db.execute('SELECT v FROM kv WHERE v=?', ('NestedBookmark',)).fetchone() == ('NestedBookmark',)
        assert db.execute('SELECT v FROM kv WHERE v=?', ('TagOnlyNested',)).fetchone() == ('TagOnlyNested',)
        assert db.execute('SELECT v FROM kv WHERE v=?',
                          ('TagOnlySecondMember',)).fetchone() == ('TagOnlySecondMember',)

    # A member now exists in TAB too. Its previously indexed GTOC parent causes
    # the archive manager to auto-mount the GTOC during the TAB pass.
    updated_payload = nested_rtpc('TagOnlyReturn')
    assert len(updated_payload) == len(unknown_payload)
    entries = [(name, struct.pack('<I', unknown_tag) + updated_payload + extra_payload
                if name == unknown_archive else content) for name, content in entries]
    entries.append((member_name, b'trigger parent mounting'))
    write_tab(initial, entries)
    result = subprocess.run([executable, str(root / 'game')], cwd=work, capture_output=True, text=True)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    with sqlite3.connect(root / 'second_extinction_hashes.db') as db:
        assert db.execute('SELECT v FROM kv WHERE v=?', ('TagOnlyReturn',)).fetchone() == ('TagOnlyReturn',)
        assert db.execute('SELECT v FROM kv WHERE v=?',
                          ('TagOnlySecondMember',)).fetchone() == ('TagOnlySecondMember',)
        assert db.execute('SELECT murmur FROM files WHERE lookup3=?',
                          (lookup3(unknown_archive),)).fetchone() == (expected_unknown,)
        assert db.execute('SELECT parent FROM files WHERE name=?', (texture_slice,)).fetchone() == (0,)
        assert f'File with different parent hash already exists "{texture_slice}"' not in result.stdout + result.stderr
print('Second Extinction location archives and nested GTOC member resolved')
