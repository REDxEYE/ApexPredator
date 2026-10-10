#!/usr/bin/env python3
"""Verify General's packed RGBA base texture is selected by material dot weights."""
import argparse
import base64
import json
from pathlib import Path
import sqlite3
import struct
import subprocess
import tempfile
import zlib


def dds_rgba(pixel):
    header = struct.pack('<31I', 124, 0x100F, 1, 1, 4, 0, 0,
                         *([0] * 11), 32, 0x41, 0, 32,
                         0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
                         0x1000, 0, 0, 0, 0)
    return b'DDS ' + header + bytes(pixel)


def general_block(flags, weights=(0.0, 1.0, 0.0, 0.2)):
    material = (struct.pack('<4f3f', *weights, 1, 1, 1)
                + struct.pack('<3fIf', 0, 0, 16, 0, 1)
                + struct.pack('<5f4BI', 1, 1, 1, 1, 0, 0, 0, 0, 0, flags))
    assert len(material) == 76
    block = bytearray(struct.pack('<IB', 0xA7583B2B, 3) + material)
    for name in ['AnimationSet'] * 3 + [''] * 5:
        encoded = name.encode()
        block.extend(struct.pack('<I', len(encoded)) + encoded)
    block.extend(struct.pack('<II', 3, 3))  # Mode, vertex count
    for x, y in ((0, 0), (1, 0), (0, 1)):
        block.extend(struct.pack('<10f', x, y, 0, x, y, x, y, 0.25, -0.25, 24.5))
    block.extend(struct.pack('<I3HI', 3, 0, 1, 2, 0x89ABCDEF))
    return block


def model():
    return (struct.pack('<I5sIII6fI', 5, b'RBMDL', 1, 13, 0,
                        0, 0, 0, 1, 1, 1, 4)
            + general_block(0x20)
            + general_block(0x41)
            + general_block(0x20, (1.0, 0.0, 0.0, 0.0))
            + general_block(0x20, (0.0, 0.0, 0.0, 1.0)))


def png_pixel(image):
    assert image.startswith(b'\x89PNG\r\n\x1a\n')
    offset = 8
    compressed = bytearray()
    while offset < len(image):
        length = struct.unpack_from('>I', image, offset)[0]
        kind = image[offset + 4:offset + 8]
        data = image[offset + 8:offset + 8 + length]
        if kind == b'IHDR':
            assert data[:10] == struct.pack('>IIBB', 1, 1, 8, 6)
        if kind == b'IDAT':
            compressed.extend(data)
        offset += 12 + length
    scanline = zlib.decompress(compressed)
    return tuple(scanline[1:5])  # One pixel; every PNG row filter has zero predictors.


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', type=Path)
    binary = parser.parse_args().binary.resolve()
    with tempfile.TemporaryDirectory(prefix='apex-general-channels-') as temp:
        root = Path(temp) / 'Just Cause 2'
        archives = root / 'archives_win32'
        archives.mkdir(parents=True)
        (root / 'DLC').mkdir()
        (root / 'JustCause2.exe').write_bytes(b'MZ')
        rbm = model()
        texture = dds_rgba((10, 100, 250, 128))
        texture_offset = (len(rbm) + 2047) & ~2047
        (archives / 'pc0.tab').write_bytes(struct.pack('<I6I', 2048,
                                                       0xFACE0124, 0, len(rbm),
                                                       2035976115, texture_offset, len(texture)))
        (archives / 'pc0.arc').write_bytes(rbm.ljust(texture_offset, b'\0') + texture)
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
        result = subprocess.run([str(binary), 'extract', str(root), '0xFACE0124',
                                 '-o', str(output), '--db_path', str(database_path)],
                                cwd=temp, capture_output=True, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        scene = json.loads((output / 'FACE0124.gltf').read_text())
        primitives = scene['meshes'][0]['primitives']
        assert len(primitives) == 4
        pixels = []
        for primitive in primitives:
            material = scene['materials'][primitive['material']]
            tex_index = material['pbrMetallicRoughness']['baseColorTexture']['index']
            image = scene['images'][scene['textures'][tex_index]['source']]
            view = scene['bufferViews'][image['bufferView']]
            blob = base64.b64decode(scene['buffers'][view['buffer']]['uri'].split(',', 1)[1])
            start = view.get('byteOffset', 0)
            pixels.append(png_pixel(blob[start:start + view['byteLength']]))
        assert pixels == [(126, 126, 126, 128), (10, 100, 250, 128),
                          (10, 10, 10, 128), (128, 128, 128, 128)], pixels
    print('General packed-channel albedo export passed')


if __name__ == '__main__':
    main()
