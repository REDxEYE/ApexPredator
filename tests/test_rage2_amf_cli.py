#!/usr/bin/env python3
"""Exercise RAGE2 modelc lookup3 references and merged mesh buffers through the CLI."""
import argparse
import base64
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


MODEL = 'models/props/cable/horizontal_03.modelc'
MESH = 'models/props/cable/horizontal_03.meshc'
MODEL_LOOKUP3, MODEL_MURMUR = 0x0016D077, 0x4BC448C04962B97E
MESH_LOOKUP3, MESH_MURMUR = 0x93324A6B, 0x09190F49E8D1D96C
WIDE_MESH = 'models/props/cable/horizontal_03.hrmeshc'
WIDE_LOOKUP3, WIDE_MURMUR = 0x090E388F, 0x0F088655A51E4282
EIGHT_MESH = 'models/props/cable/eight_influences.meshc'
EIGHT_LOOKUP3, EIGHT_MURMUR = 0xA011B011, 0x12345678


def array(blob, at, payload, count):
    offset = len(blob)
    blob.extend(payload)
    struct.pack_into('<4I', blob, at, offset, 0, count, 0)


def adf(*instances):
    first = 72 + 24 * len(instances)
    total = first + sum(len(payload) for _, payload in instances)
    data = bytearray(struct.pack('<4s13IQ', b' FDA', 4, len(instances), 72,
                                 0, 0, 0, 0, 0, 0, total, 0, 0, 0, 0))
    data.extend(bytes(8))
    for type_hash, payload in instances:
        data.extend(struct.pack('<4IQ', 0, type_hash, first, len(payload), 0))
        first += len(payload)
    for _, payload in instances:
        data.extend(payload)
    return bytes(data)


def mesh_adf(index_width, position_format=3, uv_format=30, with_tangent_space=False, with_skin=False):
    # Rage2: header -> LOD -> mesh; IndexOffsets count indices while
    # VertexOffsets and mesh stream offsets count bytes in MergedBuffer.
    header = bytearray(56 + 24 + 128)
    struct.pack_into('<4I', header, 32, 56, 0, 1, 0)
    struct.pack_into('<4I', header, 56 + 8, 80, 0, 1, 0)
    mesh = 80
    struct.pack_into('<IBB2xII', header, mesh, 0, index_width, 0, 6 if index_width == 4 else 3, 3)
    extra_stride = [16] if with_skin else [4] if with_tangent_space else []
    array(header, mesh + 16, bytes([12, 4] + extra_stride), 2 + len(extra_stride))
    stream_offsets = [0, 36] + ([48] if extra_stride else [])
    array(header, mesh + 32, struct.pack('<' + 'I' * len(stream_offsets), *stream_offsets),
          len(stream_offsets))
    if with_skin:
        array(header, mesh + 80, struct.pack('<8h', *range(8)), 8)
    submesh = struct.pack('<II6f', 0, 3, 0, 0, 0, 1, 1, 1)
    array(header, mesh + 96, submesh * (2 if index_width == 4 else 1),
          2 if index_width == 4 else 1)
    attributes = struct.pack('<IIBBB8sB', 1, position_format, 0, 0, 12, bytes(8), 0)
    attributes += struct.pack('<IIBBB8sB', 2, uv_format, 1, 0, 4, struct.pack('<ff', 1, 1), 0)
    attributes += struct.pack('<IIBBB8sB', 10, 42, 0, 6, 12, bytes(8), 0)
    attributes += struct.pack('<IIBBB8sB', 0xDEADBEEF, 0xDEADBEEF, 0, 0, 0, bytes(8), 0)
    if with_tangent_space:
        attributes += struct.pack('<IIBBB8sB', 6, 50, 2, 0, 4, bytes(8), 0)
    if with_skin:
        for usage, offset, fmt in ((7, 0, 24), (8, 4, 22), (7, 8, 24), (8, 12, 22)):
            attributes += struct.pack('<IIBBB8sB', usage, fmt, 2, offset, 16, bytes(8), 0)
    array(header, mesh + 112, attributes, 4 + bool(with_tangent_space) + 4 * bool(with_skin))

    buffers = bytearray(64)
    indices = struct.pack('<3H' if index_width == 2 else '<3I', 0, 1, 2)
    if index_width == 4:
        indices += struct.pack('<3I', 2, 1, 0)
    positions = struct.pack('<9f', -2, -1, 0, 2, 0, 0, 0, 3, 0)
    uvs = struct.pack('<6h', 0, 0, 32767, 0, 0, 32767)
    array(buffers, 8, struct.pack('<I', 0), 1)
    array(buffers, 24, struct.pack('<I', len(indices)), 1)
    # Positive, negative, and coincident tangent/bitangent frames.
    frames = bytes((128, 191, 191, 191, 128, 191, 64, 64, 128, 191, 128, 191))
    skin = b''.join(
        bytes((0, 1, 2, 3)) + primary + bytes((4, 5, 6, 7)) + secondary
        for primary, secondary in ((bytes((128, 0, 0, 0)), bytes((64, 0, 0, 0))),
                                   (bytes(4), bytes((255, 0, 0, 0))),
                                   (bytes((64, 64, 0, 0)), bytes((64, 64, 0, 0))))
    ) if with_skin else b''
    merged = indices + positions + uvs + (frames if with_tangent_space else b'') + skin
    array(buffers, 40, merged, len(merged))
    return adf((0x7A2C9B73, header), (0x0E1C0800, buffers))


def model_adf():
    data = bytearray(48)
    struct.pack_into('<I', data, 0, MESH_LOOKUP3)
    return adf((0xF7C20A69, data))


def accessor_values(gltf, index, fmt):
    accessor = gltf['accessors'][index]
    view = gltf['bufferViews'][accessor['bufferView']]
    data = base64.b64decode(gltf['buffers'][view['buffer']]['uri'].split(',', 1)[1])
    return struct.unpack_from(fmt, data, view.get('byteOffset', 0) + accessor.get('byteOffset', 0))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='apex-rage2-amf-') as temporary:
        root = Path(temporary)
        install = root / 'RAGE 2'
        initial = install / 'archives_win64/initial'
        initial.mkdir(parents=True)
        (install / 'RAGE2.exe').write_bytes(b'MZ')
        assets = [(MODEL, MODEL_LOOKUP3, MODEL_MURMUR, model_adf()),
                  (MESH, MESH_LOOKUP3, MESH_MURMUR, mesh_adf(2, with_tangent_space=True)),
                  (EIGHT_MESH, EIGHT_LOOKUP3, EIGHT_MURMUR, mesh_adf(2, with_skin=True)),
                  (WIDE_MESH, WIDE_LOOKUP3, WIDE_MURMUR, mesh_adf(4))]
        tab = bytearray(struct.pack('<4sHH6I', b'TAB\0', 3, 1, 4096, len(assets), 0, 0, 0, 0))
        arc = bytearray()
        for name, lookup3, murmur, payload in assets:
            tab.extend(struct.pack('<QIIIHBB', murmur, len(arc), len(payload), len(payload), 0, 0, 0))
            arc.extend(payload)
        (initial / 'game0.tab').write_bytes(tab)
        (initial / 'game0.arc').write_bytes(arc)
        db_path = root / 'rage2_hashes.db'
        with sqlite3.connect(db_path) as db:
            db.executescript('CREATE TABLE kv(lookup3 INTEGER NOT NULL,murmur INTEGER NOT NULL DEFAULT 0,'
                             'v TEXT NOT NULL,PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;'
                             'CREATE TABLE files(lookup3 INTEGER NOT NULL,murmur INTEGER NOT NULL DEFAULT 0,'
                             'name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0,'
                             'PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;')
            for name, lookup3, murmur, payload in assets:
                db.execute('INSERT INTO kv VALUES(?,?,?)', (lookup3, murmur, name))
                db.execute('INSERT INTO files VALUES(?,?,?,?,0)', (lookup3, murmur, name, len(payload)))
        for path, index_width in [(MODEL, 2), (MESH, 2), (WIDE_MESH, 4)]:
            output = root / f'out-{index_width}-{Path(path).suffix}'
            subprocess.run([str(args.binary.resolve()), 'extract', str(install), path,
                            '--module', 'rage2', '-n', '-d', str(db_path), '-o', str(output)],
                           check=True, capture_output=True, text=True)
            gltf = json.loads((output / path).with_suffix('.gltf').read_text())
            assert len(gltf['meshes']) == 1, (path, gltf.get('meshes'))
            primitive = gltf['meshes'][0]['primitives'][0]
            attrs = primitive['attributes']
            expected = {'POSITION', 'TEXCOORD_0'}
            if index_width == 2:
                expected |= {'NORMAL', 'TANGENT'}
            assert set(attrs) == expected
            assert accessor_values(gltf, attrs['POSITION'], '<9f') == (-2, -1, 0, 2, 0, 0, 0, 3, 0)
            assert accessor_values(gltf, attrs['TEXCOORD_0'], '<6f') == (0, 0, 1, 0, 0, 1)
            assert gltf['accessors'][primitive['indices']]['componentType'] == (5123 if index_width == 2 else 5125)
            assert accessor_values(gltf, primitive['indices'], '<3H' if index_width == 2 else '<3I') == (0, 1, 2)
            if index_width == 2:
                normals = accessor_values(gltf, attrs['NORMAL'], '<9f')
                tangents = accessor_values(gltf, attrs['TANGENT'], '<12f')
                for vertex in range(3):
                    normal = normals[vertex * 3:vertex * 3 + 3]
                    tangent = tangents[vertex * 4:vertex * 4 + 3]
                    assert abs(sum(x * x for x in normal) - 1) < 1e-4
                    assert abs(sum(x * y for x, y in zip(normal, tangent))) < 1e-4
                    assert normal[2] > 0.99
                assert tangents[3::4] == (1.0, -1.0, 1.0)
            if index_width == 4:
                other = gltf['meshes'][0]['primitives'][1]
                assert accessor_values(gltf, other['indices'], '<3I') == (2, 1, 0)
        output = root / 'out-eight'
        subprocess.run([str(args.binary.resolve()), 'extract', str(install), str(EIGHT_MURMUR),
                        '--module', 'rage2', '-n', '-d', str(db_path), '-o', str(output)],
                       check=True, capture_output=True, text=True)
        skinned = json.loads((output / EIGHT_MESH).with_suffix('.gltf').read_text())
        attrs = skinned['meshes'][0]['primitives'][0]['attributes']
        assert set(attrs) == {'POSITION', 'TEXCOORD_0', 'JOINTS_0', 'WEIGHTS_0', 'JOINTS_1', 'WEIGHTS_1'}
        assert accessor_values(skinned, attrs['JOINTS_0'], '<12H')[:4] == (0, 1, 2, 3)
        assert accessor_values(skinned, attrs['JOINTS_1'], '<12H')[:4] == (4, 5, 6, 7)
        first = accessor_values(skinned, attrs['WEIGHTS_0'], '<12f')
        second = accessor_values(skinned, attrs['WEIGHTS_1'], '<12f')
        for vertex in range(3):
            assert abs(sum(first[vertex * 4:vertex * 4 + 4]) +
                       sum(second[vertex * 4:vertex * 4 + 4]) - 1) < 1e-6
        assert abs(first[0] - 2 / 3) < 1e-6 and abs(second[0] - 1 / 3) < 1e-6
        assert first[4:8] == (0, 0, 0, 0) and second[4:8] == (1, 0, 0, 0)
        # Replacing the last asset preserves TAB offsets and tests unsupported
        # formats of equal width for supported usages.
        wide_size = len(mesh_adf(4))
        unsupported = [
            {'position_format': 11},
            {'uv_format': 27},
            {'uv_format': 28},
            {'position_format': 0xDEADBEEF},
        ]
        for kwargs in unsupported:
            arc[-wide_size:] = mesh_adf(4, **kwargs)
            (initial / 'game0.arc').write_bytes(arc)
            result = subprocess.run([str(args.binary.resolve()), 'extract', str(install), WIDE_MESH,
                                     '--module', 'rage2', '-n', '-d', str(db_path),
                                     '-o', str(root / 'out-unsupported')],
                                    capture_output=True, text=True)
            diagnostic = result.stdout + result.stderr
            assert result.returncode != 0, diagnostic
    print('Rage2 AMF model/mesh conversion passed')


if __name__ == '__main__':
    main()
