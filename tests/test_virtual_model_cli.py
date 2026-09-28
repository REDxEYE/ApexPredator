#!/usr/bin/env python3
"""End-to-end ADF -> virtual model -> glTF test; no game installation required.

Usage: python tests/test_virtual_model_cli.py BUILD/ApexPredator [--skeleton FILE ...]
Optional skeletons exercise real Havok files without checking game assets into git.
"""
import argparse
import base64
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


def array(blob, at, data, count):
    offset = len(blob)
    blob.extend(data)
    struct.pack_into('<4I', blob, at, offset, 0, count, 0)


def mesh_adf(index_width):
    # Generation Zero generated layouts: AmfMeshHeader, AmfLodGroup, AmfMesh.
    header = bytearray(56 + 24 + 152)
    struct.pack_into('<4I', header, 32, 56, 0, 1, 0)
    struct.pack_into('<4I', header, 56 + 8, 80, 0, 1, 0)
    mesh = 80
    struct.pack_into('<IIIBB2xI', header, mesh, 0, 3, 3, 0, index_width, 0)
    array(header, mesh + 24, bytes([0, 1]), 2)
    array(header, mesh + 40, bytes([12, 4]), 2)
    array(header, mesh + 56, struct.pack('<II', 0, 0), 2)
    array(header, mesh + 120, struct.pack('<III6f', 0, 3, 0, -2, -1, 0, 2, 3, 0), 1)
    attributes = struct.pack('<IIBBB8sB', 1, 3, 0, 0, 12, bytes(8), 0)
    attributes += struct.pack('<IIBBB8sB', 2, 30, 1, 0, 4, struct.pack('<ff', 1, 1), 0)
    array(header, mesh + 136, attributes, 2)
    # AmfMeshBuffers and its index / vertex AmfBuffer arrays.
    buffers = bytearray(40 + 24 + 48)
    struct.pack_into('<4I', buffers, 8, 40, 0, 1, 0)
    struct.pack_into('<4I', buffers, 24, 64, 0, 2, 0)
    indices = struct.pack('<3H' if index_width == 2 else '<3I', 0, 1, 2)
    positions = struct.pack('<9f', -2, -1, 0, 2, 0, 0, 0, 3, 0)
    uv = struct.pack('<6h', 0, 0, 32767, 0, 0, 32767)
    array(buffers, 40, indices, len(indices))
    array(buffers, 64, positions, len(positions))
    array(buffers, 88, uv, len(uv))
    first = 120
    total = first + len(header) + len(buffers)
    adf = bytearray(struct.pack('<4s13IQ', b' FDA', 4, 2, 72, 0, 0, 0, 0, 0, 0, total, 0, 0, 0, 0))
    adf.extend(bytes(8))
    adf.extend(struct.pack('<4IQ', 0, 0xEA60065D, first, len(header), 0))
    adf.extend(struct.pack('<4IQ', 0, 0x67B3A453, first + len(header), len(buffers), 0))
    return bytes(adf + header + buffers)


def unpack_accessor(gltf, index, fmt):
    accessor = gltf['accessors'][index]
    view = gltf['bufferViews'][accessor['bufferView']]
    buffer = gltf['buffers'][view['buffer']]
    data = base64.b64decode(buffer['uri'].split(',', 1)[1])
    offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
    return struct.unpack_from(fmt, data, offset)


def check_sessions(runner, module_path, root):
    for i in [1, 2]:
        database = root / f'session{i}.db'
        with sqlite3.connect(root / 'hashes.db') as source, sqlite3.connect(database) as destination:
            source.backup(destination)
            destination.execute('insert into kv(lookup3,murmur,v) values(?,0,?)', (0x12340001, f'session{i}.mesh'))
    subprocess.run([str(runner), str(module_path), str(root)], check=True)
    assert (root/'session1/session1.mesh').read_bytes() == mesh_adf(2)
    assert (root/'session2/session2.mesh').read_bytes() == mesh_adf(2)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('--module-library', type=Path)
    parser.add_argument('--session-runner', type=Path)
    parser.add_argument('--skeleton', type=Path, action='append', default=[])
    args = parser.parse_args()
    assets = [mesh_adf(2), mesh_adf(4)] + [p.read_bytes() for p in args.skeleton]
    with tempfile.TemporaryDirectory(prefix='apex-vm-test-') as directory:
        root = Path(directory)
        (root / 'steam_appid.txt').write_text('704270\n')
        for part in ['initial', 'optional', 'supplemental']:
            (root / part).mkdir()
        tab = bytearray(struct.pack('<4shhi', b'TAB\0', 2, 1, 2048))
        arc = bytearray()
        hashes = []
        for i, data in enumerate(assets):
            hash_value = 0x12340001 + i
            hashes.append(f'0x{hash_value:08X}')
            tab.extend(struct.pack('<III', hash_value, len(arc), len(data)))
            arc.extend(data)
        (root / 'initial/test.tab').write_bytes(tab)
        (root / 'initial/test.arc').write_bytes(arc)
        with sqlite3.connect(root / 'hashes.db') as db:
            db.executescript('CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;'
                            'CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,'
                            'parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;')
        def probe_output():
            return subprocess.check_output([str(args.binary.resolve()), 'modules', str(root), '--module', 'generation-zero'], text=True)
        assert 'supported [100]' in probe_output()
        (root / 'steam_appid.txt').unlink()
        assert 'unsupported [0]' in probe_output()
        (root / 'steam_appid.txt').write_text('704270\n')
        invalid_tab = bytearray(tab); invalid_tab[4] = 3
        (root / 'initial/test.tab').write_bytes(invalid_tab)
        assert 'unsupported [0]' in probe_output()
        (root / 'initial/test.tab').write_bytes(tab)
        (root / 'initial/test.arc').rename(root / 'initial/test.arc.disabled')
        assert 'unsupported [0]' in probe_output()
        (root / 'initial/test.arc.disabled').rename(root / 'initial/test.arc')
        subprocess.run([str(args.binary.resolve()), 'extract', str(root), *hashes,
                        '-d', str(root / 'hashes.db'), '-o', str(root / 'output')], check=True)
        for i, width in enumerate([2, 4]):
            gltf = json.loads((root / f'output/{0x12340001+i:08X}.gltf').read_text())
            assert len(gltf['meshes']) == 1, 'Previous asset leaked into scene'
            primitive = gltf['meshes'][0]['primitives'][0]
            positions = gltf['accessors'][primitive['attributes']['POSITION']]
            assert positions['min'] == [-2, -1, 0] and positions['max'] == [2, 3, 0]
            assert unpack_accessor(gltf, primitive['attributes']['POSITION'], '<9f') == (-2,-1,0,2,0,0,0,3,0)
            assert unpack_accessor(gltf, primitive['attributes']['TEXCOORD_0'], '<6f') == (0,0,1,0,0,1)
            assert gltf['accessors'][primitive['indices']]['componentType'] == (5123 if width == 2 else 5125)
            assert unpack_accessor(gltf, primitive['indices'], '<3H' if width == 2 else '<3I') == (0,1,2)
        for i, _ in enumerate(args.skeleton, 2):
            gltf = json.loads((root / f'output/{0x12340001+i:08X}.gltf').read_text())
            assert len(gltf['skins']) == 1 and gltf['skins'][0]['joints']
            assert not gltf.get('meshes'), 'Mesh from previous asset leaked into skeleton export'
        if args.module_library:
            check_sessions((args.session_runner or args.binary.parent / ("ApexModuleSessionTests" + args.binary.suffix)).resolve(), args.module_library.resolve(), root)
    print('ADF mesh / CLI virtual model tests passed')


if __name__ == '__main__':
    main()
