# TAB 3.1 reader

`src/apex/package/tab_v31.*` implements Rage 2 and Second Extinction TAB/ARC lookup, iteration and extraction.
The 32-byte header, 8-byte block descriptors and 24-byte file entries follow the
three reference messages under `modules/rage2/`. Asset names use MurmurHash3
x64/128 with seed zero and the first 64 bits, without case or slash rewriting.

## Corrections verified against installed Rage 2

All 76 installed TAB files were checked. Some claims in the messages do not match
these files:

- Block index 0 means use the file entry's sizes directly, including compressed files.
- Index 1 can be a real block. A descriptor is a sentinel when its size fields are
  `0xffffffff`; do not unconditionally skip the first two descriptors.
- File offsets address the ARC directly. Blocks belonging to one file are contiguous
  compressed streams, with no alignment padding between them. Do not derive offsets
  by globally summing aligned block sizes.
- Each block is decoded independently, then its decoded bytes are appended.
- Equal compressed/uncompressed sizes identify stored data, even with codec 4.
- Compression flags occur as both 0 and 1; they do not select the codec.

The four installed Second Extinction `initial` TAB files also use version 3.1,
but compressed entries use codec 3 (Zstd), not Rage 2's codec 4 (Oodle).
The reader decodes both, checking the exact output size for each block.

The reader checks table lengths, ARC bounds, block indices, sentinels, summed sizes,
codec IDs and decoded lengths. Missing assets return null. Iteration reports decoded
sizes and honors callback cancellation.

## Decoder

The x86 SIMD Oodle decoder is vendored from [q3k/pyooz](https://github.com/q3k/pyooz)
at `fde8c264d0bbcee36d9836cb1895c02861783d4e`, derived from powzix/ooz.
License and local portability changes are recorded in `external/ooz`.
Input/output buffers include the decoder's required padding. The upstream decoder
is not fuzz-safe; metadata checks do not make arbitrary compressed payloads safe.
Use trusted game archives. Native Linux and Windows builds require no game DLL.

## Verification

`tests/test_tab_v31.py` covers Murmur3 hash vectors, stored/zlib/Zstd/Oodle
streams, consecutive blocks, stored blocks within compressed files, index 1,
empty files, malformed tables/blocks/codecs, and missing assets. A live
Second Extinction ARC entry decoded through the TAB reader yielded 43,896 bytes
from a 30,444-byte Zstd stream with `AVTX` payload magic.
Proprietary game payloads are not committed as fixtures.
