#!/usr/bin/env python3
"""Export synthetic Facade RBMDL blocks through the JC2 CLI and inspect GLTF data."""
import argparse
import base64
import json
import math
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile


def facade_model():
    material = struct.pack('<4f3f3fffIfI',
                           1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0,
                           1.8, 1.8, 1.6, 0.0, 16.0, 0, 2.0, 1)
    payload = bytearray(struct.pack('<I5sIII6fI', 5, b'RBMDL', 1, 13, 0,
                                    0, 0, 0, 3, 4, 5, 2))
    for vertex_format in (0, 1):
        block = bytearray(struct.pack('<IB', 0xCE39D7BF, 1))
        block.extend(material[:48] + struct.pack('<I', vertex_format) + material[52:])
        block.extend(struct.pack('<8I', *([0] * 8)))  # Empty texture names
        block.extend(struct.pack('<II', 3, 3))  # Mode, vertex count
        for i in range(3):
            uv_and_packed = (0.125 + i, 0.25, 0.75, 0.875, 0.25, -0.25, 24.5, 2048.75)
            if vertex_format == 0:
                block.extend(struct.pack('<11f', 1 + i, 2 + i, 3 + i, *uv_and_packed))
            else:
                block.extend(struct.pack('<4h8f', 16384 + i, 0, -32768, 32767,
                                         *uv_and_packed))
        block.extend(struct.pack('<I3HI', 3, 0, 1, 2, 0x89ABCDEF))
        payload.extend(block)
    return bytes(payload)


def accessor_values(scene, accessor_index):
    accessor = scene['accessors'][accessor_index]
    view = scene['bufferViews'][accessor['bufferView']]
    blob = base64.b64decode(scene['buffers'][view['buffer']]['uri'].split(',', 1)[1])
    formats = {5123: 'H', 5126: 'f'}
    components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
    layout = '<' + formats[accessor['componentType']] * components[accessor['type']]
    stride = view.get('byteStride', struct.calcsize(layout))
    offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
    return [struct.unpack_from(layout, blob, offset + i * stride)
            for i in range(accessor['count'])]


def check_facade(scene):
    primitives = scene['meshes'][0]['primitives']
    assert len(primitives) == 2
    for vertex_format, primitive in enumerate(primitives):
        attrs = primitive['attributes']
        assert set(attrs) == {'POSITION', 'TEXCOORD_0', 'TEXCOORD_1',
                              'NORMAL', 'TANGENT', 'COLOR_0'}
        assert accessor_values(scene, primitive['indices']) == [(0,), (1,), (2,)]
        position = accessor_values(scene, attrs['POSITION'])[0]
        if vertex_format == 0:
            assert position == (1, 2, 3)
        else:
            assert math.isclose(position[0], 2 * 16384 / 32767, rel_tol=1e-6)
            assert position[1:] == (0, -2)
        assert accessor_values(scene, attrs['TEXCOORD_0'])[0] == (0.125, 0.25)
        assert accessor_values(scene, attrs['TEXCOORD_1'])[0] == (0.75, 0.875)
        normal = accessor_values(scene, attrs['NORMAL'])[0]
        expected = (-0.5, 2 * (0.25 / 256) - 1, 2 * (0.25 / 65536) - 1)
        length = math.sqrt(sum(value * value for value in expected))
        assert all(math.isclose(actual, value / length, rel_tol=1e-5)
                   for actual, value in zip(normal, expected))
        tangent = accessor_values(scene, attrs['TANGENT'])[0]
        assert all(math.isclose(actual, value, rel_tol=1e-5)
                   for actual, value in zip(tangent[:3], normal))
        assert tangent[3] == 1.0
        color = accessor_values(scene, attrs['COLOR_0'])[0]
        assert all(math.isclose(actual, value, rel_tol=1e-5)
                   for actual, value in zip(color, (0.5, 24.5 / 64, 24.5 / 4096)))
        assert scene['materials'][primitive['material']]['doubleSided']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    binary = parser.parse_args().binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-facade-') as temp:
        root = Path(temp) / 'Just Cause 2'
        archives = root / 'archives_win32'
        archives.mkdir(parents=True)
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        (root / 'DLC').mkdir()
        model = facade_model()
        (archives / 'pc0.tab').write_bytes(struct.pack('<4I', 2048, 0xFACE0123, 0, len(model)))
        (archives / 'pc0.arc').write_bytes(model)

        database_path = Path(temp) / 'jc2.db'
        with sqlite3.connect(database_path) as database:
            database.executescript(
                'CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0, '
                'v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;'
                'CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0, '
                'name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0, '
                'PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;'
            )
        output = Path(temp) / 'output'
        result = subprocess.run([str(binary), 'extract', str(root), '0xFACE0123',
                                 '-o', str(output), '--db_path', str(database_path)],
                                cwd=temp, capture_output=True, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        check_facade(json.loads((output / 'FACE0123.gltf').read_text()))
    print('Facade RBMDL export passed')


if __name__ == '__main__':
    main()
