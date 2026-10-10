#!/usr/bin/env python3
"""Run both collectors on RTPC/GTOC/STOC archives and inspect SQLite output."""
import pathlib
import sqlite3
import struct
import subprocess
import sys
import tempfile


def rotate(value, count, bits):
    mask = (1 << bits) - 1
    return ((value << count) | ((value & mask) >> (bits - count))) & mask


def lookup3(text):
    """Independent little-endian Jenkins lookup3 reference, seed zero."""
    data = text.encode()
    mask = 0xffffffff
    a = b = c = (0xdeadbeef + len(data)) & mask
    while len(data) > 12:
        x, y, z = struct.unpack('<III', data[:12])
        a, b, c = (a+x) & mask, (b+y) & mask, (c+z) & mask
        a = ((a-c) ^ rotate(c, 4, 32)) & mask; c = (c+b) & mask
        b = ((b-a) ^ rotate(a, 6, 32)) & mask; a = (a+c) & mask
        c = ((c-b) ^ rotate(b, 8, 32)) & mask; b = (b+a) & mask
        a = ((a-c) ^ rotate(c, 16, 32)) & mask; c = (c+b) & mask
        b = ((b-a) ^ rotate(a, 19, 32)) & mask; a = (a+c) & mask
        c = ((c-b) ^ rotate(b, 4, 32)) & mask; b = (b+a) & mask
        data = data[12:]
    if not data:
        return c
    x, y, z = struct.unpack('<III', data.ljust(12, b'\0'))
    a, b, c = (a+x) & mask, (b+y) & mask, (c+z) & mask
    c = ((c ^ b) - rotate(b, 14, 32)) & mask
    a = ((a ^ c) - rotate(c, 11, 32)) & mask
    b = ((b ^ a) - rotate(a, 25, 32)) & mask
    c = ((c ^ b) - rotate(b, 16, 32)) & mask
    a = ((a ^ c) - rotate(c, 4, 32)) & mask
    b = ((b ^ a) - rotate(a, 14, 32)) & mask
    return ((c ^ b) - rotate(b, 24, 32)) & mask


def murmur(text):
    """Low 64 bits of MurmurHash3 x64_128, seed zero, signed for SQLite."""
    data = text.encode()
    mask = (1 << 64) - 1
    c1, c2 = 0x87c37b91114253d5, 0x4cf5ad432745937f
    h1 = h2 = 0

    def mix(k, first, rotation, last):
        return (rotate((k * first) & mask, rotation, 64) * last) & mask

    def finish(k):
        k = ((k ^ (k >> 33)) * 0xff51afd7ed558ccd) & mask
        k = ((k ^ (k >> 33)) * 0xc4ceb9fe1a85ec53) & mask
        return k ^ (k >> 33)

    end = len(data) // 16 * 16
    for pos in range(0, end, 16):
        k1, k2 = struct.unpack_from('<QQ', data, pos)
        h1 ^= mix(k1, c1, 31, c2)
        h1 = (rotate(h1, 27, 64) + h2) & mask
        h1 = (h1 * 5 + 0x52dce729) & mask
        h2 ^= mix(k2, c2, 33, c1)
        h2 = (rotate(h2, 31, 64) + h1) & mask
        h2 = (h2 * 5 + 0x38495ab5) & mask
    tail = data[end:]
    if len(tail) > 8:
        h2 ^= mix(int.from_bytes(tail[8:], 'little'), c2, 33, c1)
    if tail:
        h1 ^= mix(int.from_bytes(tail[:8], 'little'), c1, 31, c2)
    h1 ^= len(data)
    h2 ^= len(data)
    h1 = (h1 + h2) & mask
    h2 = (h2 + h1) & mask
    result = (finish(h1) + finish(h2)) & mask
    return result if result < 2**63 else result - 2**64


def toc_fixture(paths):
    # Two archives, shared metadata at members 20 and 40, external-only at 48.
    data = bytearray(struct.pack('<4sIIII', b'GT0C', 2, lookup3('first.sarc'), 7, 1))
    data += struct.pack('<II', 0, 128)
    data += struct.pack('<III', lookup3('second.sarc'), 9, 2)
    data += struct.pack('<IIII', 0, 256, 0, 0)
    for path, members in zip(paths, [(20, 40), (48,)]):
        metadata = len(data)
        extension = pathlib.PurePosixPath(path).suffix
        data += struct.pack('<III', lookup3(path), lookup3(extension), 0x12345678)
        data += path.encode() + b'\0'
        data += b'\0' * (-len(data) % 4)
        for member in members:
            struct.pack_into('<I', data, member, metadata-member)
    return data


# Published project fixtures anchor the reference implementations to real keys.
assert lookup3('AnimationSet') == 2035976115
assert murmur('text/master_eng.stringlookup') == 0x8453EE3581F31F39 - 2**64

for game, executable in enumerate(sys.argv[1:]):
    executable = str(pathlib.Path(executable).resolve())
    with tempfile.TemporaryDirectory(prefix='apex-collector-') as tmp:
        root = pathlib.Path(tmp)
        work = root / 'bin'
        work.mkdir()
        initial = root / 'game/archives_win64/initial'
        initial.mkdir(parents=True)
        strings = root / 'strings' / 'generation_zero'
        strings.mkdir(parents=True)
        for name in ['file_locations', 'filenames', 'cross_game', 'game_dump_clean']:
            (strings / (name + '.txt')).write_text('SharedCollectorPath\n' if name == 'cross_game' else '')
        if game == 1:
            rage_strings = root / 'strings' / 'rage2'
            rage_strings.mkdir()
            (rage_strings / 'filelist.txt').write_text('Rage2FileListEntry\n')
            (rage_strings / 'rage2_exe_strings.txt').write_text('Rage2ExecutableEntry\n')
        asset = 'AnimationSet' if game == 0 else 'text/master_eng.stringlookup'
        asset_hash = 2035976115 if game == 0 else 0x8453EE3581F31F39
        signed = asset_hash if asset_hash < 2**63 else asset_hash - 2**64
        db_path = root / ('hashes.db' if game == 0 else 'rage2_hashes.db')
        with sqlite3.connect(db_path) as db:
            db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                             'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
            db.execute('INSERT INTO kv VALUES(?,?)', (signed, asset))
        payload = bytearray(36)
        payload[:20] = struct.pack('<4sIIIHH', b'RTPC', 3, 1, 20, 1, 0)
        payload[20:29] = struct.pack('<IIB', 1, 36, 3)
        payload += b'CollectedRTPCString\0'
        internal_paths = [
            ('textures/gtoc_only.ddsc', 'external/gtoc_stream.meshc'),
            ('audio/stoc_only.bank', 'external/stoc_stream.animc'),
        ]
        entries = [(asset, payload), ('synthetic/catalog.gtoc', toc_fixture(internal_paths[0])),
                   ('synthetic/stream.stoc', toc_fixture(internal_paths[1]))]
        with sqlite3.connect(db_path) as db:
            for name, _ in entries[1:]:
                key = lookup3(name) if game == 0 else murmur(name)
                db.execute('INSERT INTO kv VALUES(?,?)', (key, name))
        archive = bytearray()
        if game == 0:
            tab = struct.pack('<4sHHI', b'TAB\0', 2, 1, 2048)
        else:
            tab = struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, len(entries), 0, 0, 0, 0)
        for name, content in entries:
            if game == 0:
                tab += struct.pack('<III', lookup3(name), len(archive), len(content))
            else:
                tab += struct.pack('<QIIIHBB', murmur(name) & (2**64-1), len(archive),
                                   len(content), len(content), 0, 0, 0)
            archive += content
        (initial / 'game0.arc').write_bytes(archive)
        (initial / 'game0.tab').write_bytes(tab)
        for _ in range(2):
            result = subprocess.run([executable, str(root / 'game')], cwd=work, capture_output=True, text=True)
            assert result.returncode == 0, result.stdout + result.stderr
        with sqlite3.connect(db_path) as db:
            expected_names = {'CollectedRTPCString', 'SharedCollectorPath'} | {name for name, _ in entries}
            if game == 1:
                expected_names.update({'Rage2FileListEntry', 'Rage2ExecutableEntry'})
            for paths in internal_paths:
                expected_names.update(paths)
            for name in expected_names:
                rows = db.execute('SELECT lookup3,murmur FROM kv WHERE v=?', (name,)).fetchall()
                expected = (lookup3(name), 0 if game == 0 else murmur(name))
                assert rows == [expected], (game, name, rows, expected)
            files = set(db.execute('SELECT lookup3,murmur,name,size FROM files'))
            expected_external = {
                (lookup3(external), 0 if game == 0 else murmur(external), external, 0x12345678)
                for _, external in internal_paths
            }
            assert expected_external <= files, files
print('Both hash collectors passed GTOC/STOC ingestion, dual-hash output and migration checks')
