# Rage 2 RTPC analysis

Static analysis of the running IDA database for `RAGE2.exe`, image base
`0x140000000`, on 2026-09-26. Compared against the current GenZero implementation
in `src/apex/rtpc.cpp` and `include/apex/rtpc.h`. Addresses refer to this executable.
No executable or IDB changes were made. The subsequent sample validation and reader implementation are recorded below.

## Entry point and function map

`sub_14182F740` is RTPC-related, but it **serializes/clones a blob view into an
owned byte buffer**. It is not a conventional file-reading/deserialization entry
point. Its first argument behaves as a byte vector (begin/end/capacity); its second
is a pair containing the source blob base and a pointer to a node header.

| Function | Observed role |
| --- | --- |
| `sub_14182F740` | Writes RTPC header and root header, then copies the tree |
| `sub_14182F820` | Recursively copies property/child tables and relocates offsets |
| `sub_14182FB60` | Copies out-of-line property payloads according to the type tag |
| `sub_14182DF00` | Resizes output byte buffer and zero-fills newly added bytes |
| `CEntityResourceLoader::sub_140359B20` | Constructs a blob view and invokes the copy routine |
| `sub_14182FE60` / `sub_141830520` | Entry/group reader for a separate sequential container representation |
| `sub_14182D210` | Value reader for that sequential representation |
| `sub_140D520A0` / `sub_140D52110` | Name hashing thunk / lookup3 implementation |

In the resource loader's kind-8 branch, the view is initialized with the blob
pointer and `blob + 8` (`0x140359C3C` onward), then passed to `sub_14182F740` at
`0x140359C71`. The owned result is again exposed as `{base, base + 8}`. This supports
reading through lightweight views over packed RTPC data rather than constructing
an independent object graph at this stage.

At `0x14182F76B` the copy routine writes the RTPC magic; at `0x14182F781` it checks
the input version. It emits version 3 for version-3 input and version 1 otherwise.
That fallback is the writer's behavior, not evidence that every other version is
valid input. The recursive call is at `0x14182F807`.

## Packed blob layout

The core layout matches the GenZero structures:

```cpp
// Little-endian, packed on disk. Illustrative declarations, not native ABI types.
struct Header { char magic[4]; uint32_t version; };                 // 8 bytes
struct Node {
    uint32_t name_hash;
    uint32_t data_offset;       // absolute offset from blob base
    uint16_t property_count;
    uint16_t child_count;
};                                                               // 12 bytes
struct Property {
    uint32_t name_hash;
    uint32_t value_or_offset;   // inline bits or absolute payload offset
    uint8_t type;
};                                                               // 9 bytes
```

The root header follows the file header. At each node's `data_offset`:

1. `property_count` packed property headers.
2. Padding to a 4-byte boundary.
3. `child_count` packed child node headers.
4. **Version 3 only: an additional 4-byte word.**

The version-3 word's meaning remains unknown. `sub_14182F820` reads it at
`0x14182F989`, extends output by four bytes at `0x14182F99D`, and stores it at
`0x14182F9A5`. Its source address is:

```text
align_up(base + node.data_offset + 9 * node.property_count, 4)
    + 12 * node.child_count
```

The word is copied without interpretation. It is present even when the node has
no children. Child bodies are reached using each child's `data_offset`; they are
not required to immediately follow its header. The writer copies payloads and
child bodies later, patching offsets in the copied tables.

## Property payloads

Names below follow the existing GenZero enum; the binary supplies the numeric
switch cases and payload operations.

| Tag | GenZero interpretation | Rage 2 blob copy behavior |
| --- | --- | --- |
| 0 | None | Header remains unchanged |
| 1 / 2 | U32 / F32 | Inline four-byte value remains unchanged |
| 3 | String | NUL-terminated bytes, including terminator |
| 4 / 5 / 6 | Vec2 / Vec3 / Vec4 | 8 / 12 / 16 bytes, aligned to 4 |
| 7 | Mat3x3 | No payload relocation case in this writer |
| 8 | Mat4x4 | 64 bytes, aligned to 4 |
| 9 / 10 | U32 / F32 array | U32 count followed by count × 4 bytes, aligned to 4 |
| 11 | U8 array | U32 count followed by count bytes, aligned to 4 |
| 12 | Deprecated | No payload relocation case in this writer |
| 13 | Object ID | 8 bytes, aligned to **2** |
| 14 | Event array | U32 count followed by count × 8 bytes, aligned to 4 |
| 15 / 16 | Unknown | No payload relocation case in this writer |

The default path returns zero, leaving the already-copied property header alone.
Missing switch cases do not prove a type is unsupported everywhere in the engine.
The meaning of the two words in each event record has not been established here.

Supporting helpers: vectors `sub_14182DD70`, `sub_14182DDF0`, `sub_14182DE80`;
matrix `sub_14182DC50`; object ID `sub_14182DCF0`. Readers must follow stored
payload offsets rather than infer that object IDs have 8-byte alignment.

## Comparison with current GenZero code

The existing parser can serve as the basis for a Rage 2 blob reader. The header,
node/property sizes, absolute offsets, child-table alignment and common payloads
agree with the inspected binary. Full compatibility is not yet demonstrated.

Issues found during the initial comparison (the array, version, hashing and
validation changes are implemented as described below):

- **Array data is currently lost.** `RuntimeProp` reserves capacity for tags 9 and
  10 but never resizes the vectors; tag 11 also leaves its vector empty.
  `IO::File::read_exact(std::vector<T>&)` reads `size() * sizeof(T)`, so all three
  paths read zero elements. Resize after validating the count, or use the
  count-taking `read_exact<T>(count)` overload.
- **Version information is discarded.** `RootNode` checks magic but ignores
  `header.version`. `RuntimeNode` neither reads nor preserves the version-3 word.
  Ignoring the word does not itself break child traversal: recursion seeks to
  absolute offsets and restores the cursor after each child header. However, it
  loses metadata and cannot support faithful round trips.
- **RTPC name hashing must be independent of asset-path hashing.** `has(name)`,
  `get<T>(name)` and `is<T>(name)` call the generic `hash_string`. The current
  Rage 2 branch of that helper uses MurmurHash3 and returns 64 bits. RTPC's keys
  remain 32 bits, and the inspected string-to-key container readers use lookup3
  with seed zero. Give RTPC a dedicated lookup3 name-hash helper; do not reuse the
  game's archive-path hash selection.
- **Tag 7 needs sample evidence.** GenZero reads Mat3x3, but this Rage 2 writer
  has no relocation case for it. Do not infer a new layout from that omission.
- **Input validation remains limited.** Add supported-version handling and
  bounds/count/depth checks when implementing the port; unknown tags currently
  abort the process.

Name-hash evidence: `sub_1418306D0` hashes named properties through
`sub_140D520A0(name, length, 0)` at `0x1418307E1`; `sub_14182FF90` does the same for
named children at `0x141830091`. The thunk targets `sub_140D52110`, whose 12-byte
mixing blocks and final rotations match the repository's lookup3 `hashlittle`.
This evidence comes from the runtime container reader described below. A known
name/hash pair in an extracted RTPC blob would provide the additional end-to-end
check.

## Do not confuse the sequential representation with RTPC blobs

Nearby functions read a second, sequential representation. `sub_141830520` reads
an 8-bit group count, then 16-bit group kinds and entry counts. Its dispatch
includes string-keyed child/property groups (1/2), skipped bytes (3), and
32-bit-hash-keyed child/property groups (4/5). This path is not the packed
RTPC-header/node-table layout above.

Its value reader, `sub_14182D210`, consumes a type byte and sequential data. For
example, strings have a 16-bit length, and tag 8 reads **48 bytes** and expands the
matrix to 64 bytes. Object IDs also undergo bit rearrangement in this reader.
Those operations must not be transplanted into the blob parser: the blob-copy
path proves NUL-terminated strings and a 64-byte matrix payload for that format.
The sequential representation's external file/protocol identity remains unproven.

## Remaining verification

Broaden sample coverage to additional property tags and trace a consumer of the
version-3 word to establish its meaning. The reader validation below does not
establish complete Rage 2 export support or decode the embedded ADF instance.


## Sample validation and reader implementation

Sample `exported/18366995012003127915.bin` has 2,538,514 bytes, exactly the
uncompressed size in `archives_win64/initial/game0.tab` (entry 250). It contains
six version-3 nodes. The root has one child, which has four children and metadata
word 2; the other five metadata words are zero.

Property 3119224088 is tag 11, an array of 2,538,366 bytes. Its count is at offset
144 and its payload starts at offset 148 and ends exactly at EOF. The payload is
an ADF v4 file whose declared total size also equals 2,538,366. Its single instance
is named `AnimationSet`, and its comment mentions `afsm_adf.adf` and
`AnimationStateMachineUserProperties.adf`. Thus the apparent ADF tail is an
ordinary referenced RTPC property, not extraction overrun. No standard AVTX/DDS
texture signatures were found in this sample.

The reader now:

- Reads all three array types into sized vectors and validates counts before allocation.
- Uses dedicated lookup3 name hashing, independently of the selected game.
- Accepts versions 1–3, propagates the version to child nodes and retains the v3
  word through `v3_metadata()` and the JSON `v3_metadata` field. Versions 1/2 keep
  their previous table layout. Version 2 coverage is synthetic, not a Rage 2
  binary observation.
- Rejects out-of-range tables/payloads, excessive nesting, unsupported versions
  and unsupported tags with exceptions instead of aborting.

`ApexRtpcTests` covers versions 1/2/3, nested metadata, all three array types,
known lookup3 name lookup, empty arrays and malformed data. Passing this sample's
path as its optional argument additionally checks the full embedded ADF payload.
The meaning of the v3 metadata word is still unknown; it is preserved verbatim.
