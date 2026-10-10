#!/usr/bin/env python3
"""Check export-time decal separation for General, Lambert, and Facade blocks."""
import argparse
import json
import math
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile

from test_jc2_facade import accessor_values
from test_jc2_general_channels import dds_rgba


NORMAL_PACKED = 0.25


def block(hash_value, material, vertex_format, facade=False, texture=False):
    payload = bytearray(struct.pack('<IB', hash_value, 1 if facade else 4 if hash_value == 0xD5D78AE0 else 3))
    payload.extend(material)
    for name in (['AnimationSet'] * 3 + [''] * 5 if texture else [''] * 8):
        encoded = name.encode()
        payload.extend(struct.pack('<I', len(encoded)) + encoded)
    payload.extend(struct.pack('<II', 3, 3))  # Mode, vertex count
    for i in range(3):
        if vertex_format == 0:
            base = (1.0 + i, 2.0, 3.0)
            uv = (0.0, 0.0, 1.0, 1.0)
            packed = (NORMAL_PACKED, -NORMAL_PACKED, 24.5)
            payload.extend(struct.pack('<11f' if facade else '<10f',
                                       *base, *uv, *packed, *((0.75,) if facade else ())))
        elif facade:
            payload.extend(struct.pack('<4h8f', 16384 + i, 0, -32768, 32767,
                                       0, 0, 1, 1, NORMAL_PACKED, -NORMAL_PACKED, 24.5, 0.75))
        else:
            payload.extend(struct.pack('<4h3f4h', 0, 0, 32767, 32767,
                                       NORMAL_PACKED, -NORMAL_PACKED, 24.5,
                                       16384 + i, 0, -32768, 32767))
    payload.extend(struct.pack('<I3HI', 3, 0, 1, 2, 0x89ABCDEF))
    return payload


def general_material(fmt, depth, scale):
    material = (struct.pack('<4f3f', 1, 1, 1, 0, 1, 1, 1)
                + struct.pack('<3fIf', 0, depth, 16, fmt, scale)
                + struct.pack('<5f4BI', 1, 1, 1, 1, 0, 0, 0, 0, 0, 0))
    assert len(material) == 76
    return material


def lambert_material(depth):
    material = struct.pack('<If4ff4BIf4B', 0, 1, 1, 1, 1, 1, 1,
                           255, 255, 255, 255, 0, depth, 0, 0, 0, 0)
    assert len(material) == 44
    return material


def facade_material(fmt, depth, scale):
    material = struct.pack('<4f3f3fffIfI', 1, 1, 1, 0, 1, 1, 1,
                           1.8, 1.8, 1.6, depth, 16, fmt, scale, 0)
    assert len(material) == 60
    return material


def model():
    blocks = [
        block(0xA7583B2B, general_material(0, 0.00001, 2), 0, texture=True),
        block(0xA7583B2B, general_material(1, -0.00002, 3), 1, texture=True),
        block(0xD5D78AE0, lambert_material(0.00001), 0),
        block(0xCE39D7BF, facade_material(0, 0, 1), 0, facade=True),
        block(0xCE39D7BF, facade_material(1, 0.00001, 2), 1, facade=True),
    ]
    return struct.pack('<I5sIII6fI', 5, b'RBMDL', 1, 13, 0,
                       -4, -4, -4, 4, 4, 4, len(blocks)) + b''.join(blocks)


def verify(scene):
    primitives = scene['meshes'][0]['primitives']
    assert len(primitives) == 5
    cases = [(0.00001, 2, (1, 2, 3)), (-0.00002, 3, (3 * 16384 / 32767, 0, -3)),
             (0.00001, 1, (1, 2, 3)), (0, 1, (1, 2, 3)),
             (0.00001, 2, (2 * 16384 / 32767, 0, -2))]
    for primitive, (depth, scale, base) in zip(primitives, cases):
        attributes = primitive['attributes']
        positions = accessor_values(scene, attributes['POSITION'])
        normals = accessor_values(scene, attributes['NORMAL'])
        assert len(positions) == 3
        pos, normal = positions[0], normals[0]
        delta = tuple(pos[i] - base[i] for i in range(3))
        if depth == 0:
            assert all(math.isclose(pos[i], base[i], abs_tol=1e-6) for i in range(3))
            continue
        length = math.sqrt(sum(x * x for x in delta))
        # A world-space separation, not the original view-dependent clip-space offset.
        assert 0.0001 * scale < length < 0.01 * scale, (depth, scale, delta)
        along = sum(delta[i] * normal[i] for i in range(3))
        assert along * depth > 0, (depth, delta, normal)
        assert math.isclose(abs(along), length, rel_tol=1e-4)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    binary = parser.parse_args().binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-depth-offset-') as temp:
        root = Path(temp) / 'Just Cause 2'
        archives = root / 'archives_win32'
        archives.mkdir(parents=True)
        (root / 'DLC').mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        rbm = model()
        texture = dds_rgba((10, 100, 250, 128))
        texture_offset = (len(rbm) + 2047) & ~2047
        (archives / 'pc0.tab').write_bytes(struct.pack('<I6I', 2048,
                                                       0xFACE0125, 0, len(rbm),
                                                       2035976115, texture_offset, len(texture)))
        (archives / 'pc0.arc').write_bytes(rbm.ljust(texture_offset, b'\0') + texture)
        database_path = Path(temp) / 'jc2.db'
        with sqlite3.connect(database_path) as db:
            db.executescript(
                'CREATE TABLE kv (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0, '
                'v TEXT NOT NULL, PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;'
                'CREATE TABLE files (lookup3 INTEGER NOT NULL, murmur INTEGER NOT NULL DEFAULT 0, '
                'name TEXT, size INTEGER NOT NULL, parent INTEGER NOT NULL DEFAULT 0, '
                'PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;'
            )
        output = Path(temp) / 'output'
        result = subprocess.run([str(binary), 'extract', str(root), '0xFACE0125',
                                 '-o', str(output), '--db_path', str(database_path)],
                                cwd=temp, capture_output=True, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        verify(json.loads((output / 'FACE0125.gltf').read_text()))
    print('RBMDL depth-offset geometry export passed')


if __name__ == '__main__':
    main()
