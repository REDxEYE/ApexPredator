#!/usr/bin/env python3
"""Find, validate and extract uncompressed embedded ADF v4 files from a PE.
Usage: extract.py RAGE2.exe output-directory [ida-candidates.json]
Uses only the Python standard library; does not modify the executable or IDB.
"""
import collections
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import zipfile


def parse_adf(blob):
    h = struct.unpack_from('<4s15I', blob)
    magic, version, ni, oi, nt, ot, nh, oh, nn, on, size, metadata, flags, libraries, r0, r1 = h
    assert magic == b' FDA' and version == 4 and size == len(blob) and size >= 64, 'header'
    def span(offset, length):
        assert 0 <= offset <= size and 0 <= length <= size - offset, 'range'
        return blob[offset:offset + length]
    def cstring(offset):
        span(offset, 1)
        end = blob.index(0, offset)
        return blob[offset:end].decode('utf-8'), end + 1
    for count, offset, stride in [(ni, oi, 24), (nt, ot, 36), (nh, oh, 9), (nn, on, 2)]:
        if count:
            assert offset >= 64, 'table offset'
            span(offset, count * stride)
    comment, _ = cstring(64)
    names = []
    pos = on + nn
    for _ in range(nn):
        name, pos = cstring(pos)
        names.append(name)
    def name_at(index):
        assert index < len(names), 'name index'
        return names[index]
    types = []
    pos = ot
    for _ in range(nt):
        span(pos, 36)
        meta, ts, align, th, name_id, tf, scalar, element, elen = struct.unpack_from('<4IQHHII', blob, pos)
        assert 0 <= meta <= 10, 'metatype'
        pos += 36
        item = {'name': name_at(name_id), 'hash': hex(th), 'metatype': meta, 'size': ts}
        if meta == 1:
            count, = struct.unpack_from('<I', span(pos, 4)); pos += 4
            span(pos, count * 32)
            for j in range(count):
                name_at(struct.unpack_from('<Q', blob, pos + j * 32)[0])
            pos += count * 32
        elif meta == 8:
            count, = struct.unpack_from('<I', span(pos, 4)); pos += 4
            span(pos, count * 12)
            for j in range(count):
                name_at(struct.unpack_from('<Q', blob, pos + j * 12)[0])
            pos += count * 12
        elif meta in (2, 3, 4, 7, 9):
            span(pos, 4); pos += 4
        elif meta == 10:
            raise ValueError('DeferredType layout needs validation')
        types.append(item)
    instances = []
    for i in range(ni):
        name_hash, type_hash, offset, length, name_id = struct.unpack_from('<4IQ', blob, oi + i * 24)
        span(offset, length)
        instances.append({'name': name_at(name_id), 'name_hash': hex(name_hash),
                          'type_hash': hex(type_hash), 'offset': offset, 'size': length})
    pos = oh
    for _ in range(nh):
        _, pos = cstring(pos)
        span(pos, 8); pos += 8
    return {'version': version, 'size': size, 'comment': comment,
            'instance_count': ni, 'type_count': nt, 'name_count': nn,
            'stringhash_count': nh, 'types': types, 'instances': instances,
            'names': names, 'metadata_offset': metadata, 'flags': flags,
            'included_libraries_offset': libraries}


def main():
    source, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    data = source.read_bytes()
    pe, = struct.unpack_from('<I', data, 60)
    assert data[pe:pe+4] == b'PE\0\0'
    nsections, = struct.unpack_from('<H', data, pe + 6)
    optional_size, = struct.unpack_from('<H', data, pe + 20)
    optional = pe + 24
    assert struct.unpack_from('<H', data, optional)[0] == 0x20b
    base, = struct.unpack_from('<Q', data, optional + 24)
    sections = []
    for i in range(nsections):
        p = optional + optional_size + 40 * i
        name = data[p:p+8].rstrip(b'\0').decode('ascii')
        vs, rva, raw_size, raw_offset = struct.unpack_from('<4I', data, p+8)
        sections.append((name, vs, rva, raw_size, raw_offset))
    ida = {}
    if len(sys.argv) > 3:
        ida = {row['file_offset']: row for row in json.loads(Path(sys.argv[3]).read_text())}
    records, rejected = [], []
    payloads = {}
    pos = data.find(b' FDA')
    while pos != -1:
        try:
            version, = struct.unpack_from('<I', data, pos + 4)
            assert version == 4, 'not ADF v4'
            size, = struct.unpack_from('<I', data, pos + 40)
            assert 64 <= size <= len(data) - pos, 'file boundary'
            blob = data[pos:pos+size]
            row = parse_adf(blob)
            section = next((s for s in sections if s[4] <= pos and pos + size <= s[4] + s[3]), None)
            assert section is not None, 'not contained in one PE section'
            name, vs, rva, raw_size, raw_offset = section
            address = base + rva + pos - raw_offset
            assert pos + size <= raw_offset + vs, 'virtual boundary'
            label = row['comment'].split(' in ', 1)[0] or 'embedded'
            label = re.sub(r'[^A-Za-z0-9._-]+', '_', label).strip('._')[:140] or 'embedded'
            if not label.lower().endswith('.adf'):
                label += '.adf'
            filename = f'{address:016X}_{label}'
            row.update(address=hex(address), rva=hex(address-base), file_offset=pos,
                       file_offset_hex=hex(pos), end_address=hex(address+size),
                       segment=name, sha256=hashlib.sha256(blob).hexdigest(), filename=filename,
                       ida_name=ida.get(pos, {}).get('name', ''), xrefs=ida.get(pos, {}).get('xrefs', []))
            if pos in ida:
                assert ida[pos]['address'] == row['address'], 'IDA address mismatch'
            records.append(row)
            payloads[filename] = blob
        except (AssertionError, ValueError, struct.error, UnicodeError, IndexError) as exc:
            rejected.append({'file_offset': hex(pos), 'reason': str(exc)})
        pos = data.find(b' FDA', pos+1)
    expected = {r['file_offset'] for r in ida.values() if r['header'][0] == 4}
    found = {r['file_offset'] for r in records}
    summary = {'source': str(source), 'source_sha256': hashlib.sha256(data).hexdigest(),
               'imagebase': hex(base), 'validated_count': len(records),
               'unique_payloads': len({r['sha256'] for r in records}),
               'payload_bytes': sum(r['size'] for r in records),
               'with_instances': sum(bool(r['instance_count']) for r in records),
               'type_only': sum(not r['instance_count'] for r in records),
               'without_direct_xrefs': sum(not r['xrefs'] for r in records),
               'sections': dict(collections.Counter(r['segment'] for r in records)),
               'ida_candidates_not_validated': [hex(x) for x in sorted(expected-found)],
               'overlapping_payloads': [], 'rejected_signatures': rejected}
    for a, b in zip(records, records[1:]):
        if a['file_offset'] + a['size'] > b['file_offset']:
            summary['overlapping_payloads'].append([a['address'], b['address']])
    (out/'manifest.json').write_text(json.dumps({'summary': summary, 'files': records}, indent=2)+'\n')
    fields = ['address', 'rva', 'file_offset_hex', 'size', 'comment', 'instance_count', 'type_count',
              'name_count', 'segment', 'ida_name', 'filename', 'sha256']
    with (out/'manifest.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction='ignore')
        writer.writeheader(); writer.writerows(records)
    with zipfile.ZipFile(out/'embedded-adf.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for name, blob in payloads.items(): z.writestr(name, blob)
    with zipfile.ZipFile(out/'embedded-adf.zip') as z:
        assert z.testzip() is None
        assert len(z.namelist()) == len(records)
    print(json.dumps(summary, indent=2))
    print('EXAMPLES')
    for row in records[:20]: print(row['address'], row['size'], row['comment'])


if __name__ == '__main__':
    main()
