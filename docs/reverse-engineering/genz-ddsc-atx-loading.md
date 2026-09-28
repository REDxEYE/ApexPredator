# Generation Zero DDSC / ATX loading

Analyzed 2026-09-26 in the user's open IDA database: `GenerationZero_P.exe`
from `GenerationZero/pdbs`, with PDB symbols. Addresses below are virtual
addresses in that database. This is static analysis of this build, not a
runtime trace or a claim about every game version.

The game retains the texture's original path and replaces its extension to
load ATX data. It does not derive an ATX hash from the DDSC hash or read ATX
hashes from the DDSC header in the traced loading path.

## Decisive functions

| Address | Function | Behavior |
|---|---|---|
| `0x141540620` | `TextureCacheLoadRequest_Internal` | Selects ATX extension from stream source; asynchronously reads stream offset/size. |
| `0x14153e9e0` | `ConvertPath` | Copies path, truncates at last dot, appends replacement extension. |
| `0x14153ff40` | `TextureCacheCreateTexture_Internal` | Copies AVTX header; retains path with `.atx1`; initializes resident data from DDSC stream 0. |
| `0x14153ba00` | `TextureCacheCreateResource` | Passes resource-creation callback's `path` to texture creation. |
| `0x14160e220` | `AVATextureGetStreamOffset` | Returns `m_Streams[stream].m_Offset`. |
| `0x14160e2b0` | `AVATextureGetStreamSize` | Returns `m_Streams[stream].m_Size`. |
| `0x14153f820` | `TextureCacheCreateDeviceTextureAVA` | Derives dimensions, mip count, format and layout from retained AVTX header. |
| `0x140c28140` | `VfsReadFileAsync` | Queues a filename, byte offset and byte count. |

Equivalent central logic, with simplified names:

```cpp
const auto& stream = texture.header.streams[stream_index];
const int extension_index = std::clamp(int(stream.source) - 1, 0, 8);
const char* extensions[] = {
    ".atx1", ".atx2", ".atx3", ".atx4", ".atx5",
    ".atx6", ".atx7", ".atx8", ".atx9"
};
ConvertPath(texture.path, texture.path, extensions[extension_index]);
VfsReadFileAsync(vfs, texture.path, stream.offset, buffer, stream.size, &request);
```

The actual extension replacement call is at `0x1415407dd`; the VFS read
is at `0x14154080a`. Texture creation initially stores the `.atx1` path at
`0x14154019b`. Consequently later requests can replace one ATX suffix with
another without retaining a second copy of the original DDSC suffix.

The ATX number is the stream's **source**, not necessarily the stream array
index or mip level. The request function clamps the suffix to 1 through 9;
there is no `.atx0` entry in this table. Resident stream 0 is read directly
from the supplied DDSC bytes by texture creation.

## Filename provenance despite hash-based lookup

For the archive loading path:

1. `CStreamArchive::ReadTOC` (`0x1403685b0`) reads the version-3 name block
   and directory entries with name offset, data offset, size, path hash,
   and extension hash. This matches the SARC v3 directory layout.
2. `CStreamArchive::GetFilename` (`0x14035ec70`) returns
   `TOC_NAMES.data() + item.FileNameOffset`.
3. `CResourceLoader::UpdateCreation_Setup` (`0x1403eafe0`) assigns that
   filename to `CResourceRequest::m_Path`. `Update_Setup`
   (`0x1403ed410`) does the same for external reads.
4. `CResourceLoaderManager::LoadAssetToCache` (`0x1403e1600`) passes
   `request->m_Path` to `ResourceCacheCreateResourceWithOutRegister`
   (`0x140b72740`), or its in-place counterpart.
5. The resource creator receives the path, and `TextureCacheCreateResource`
   passes it to `TextureCacheCreateTexture_Internal`.
6. `STextureResource::m_Path` is a 256-byte array, at byte offset 212 in
   the PDB type, used by later ATX requests.

`TextureCacheGetTextureByHash` (`0x14153dc40`) looks up an existing resource
through `ResourceCacheGetResource`; it does not reconstruct a filename from
the hash. The direct filename API, `TextureCacheGetTexture`
(`0x14153da00`), supplies its input path during synchronous creation.

Thus a tool with only a standalone DDSC blob and a hash is missing context
that the game's archive/resource loader supplies. Preserve names from SARC
entries and other explicit resource filenames to reproduce this path.

## Header and mip interpretation

PDB confirms `AVATextureHeader` is 128 bytes, with 8 stream records starting
at byte offset 32. Each `AVATextureStreamHeader` occupies 12 bytes:

| Relative byte offset | Type | PDB field |
|---|---|---|
| 0 | uint32 | `m_Offset` |
| 4 | uint32 | `m_Size` |
| 8 | uint16 | `m_Alignment` |
| 10 | uint8 | `m_TileMode` |
| 11 | uint8 | `m_Source` |

These match `AVTX::TextureStream` in `include/apex/avtx.h`. No hash field
occurs in this record.

`TextureCacheCreateDeviceTextureAVA` computes:

```cpp
mips_in_device_texture = header.m_MipsResident + stream_index;
first_mip = header.m_Mips - mips_in_device_texture;
width  = header.m_Width  >> first_mip;
height = header.m_Height >> first_mip;
depth  = header.m_Depth  >> first_mip;
```

Format, dimensions, flags, alignment and tile mode come from the retained
header. This explains why ATX payloads do not require their own header.
`AVATextureGetStreamRangeSize` (`0x14160e230`) sums stream sizes from 0
through the requested stream; `GetStreamSize` returns only the requested
stream's size.

The existing extension replacement in `src/apex/avtx.cpp` therefore matches
the game's scheme for valid external source values. A missing filename
cannot be repaired by looking for a hidden ATX hash in this header. Also,
resident fallback dimensions should follow the resident mip count rather
than assuming the header's full-resolution dimensions.

## TAB storage versus filename provenance

The filename does not need to be stored in the archive containing the bytes.
`VfsArchiveReadFile` (`0x140c2cc10`) accepts `file_name`, copies and normalizes
it, computes `HashString(path)` for archive versions >= 2, and calls
`VfsArchiveSearchEntry` with the resulting hash. The async archive provider
(`0x140c2ce60`) delegates to this same function. Thus ATX data can be stored
in a hash-only TAB/ARC archive while requests still carry full filenames.

`CResourceLoader::Update_Setup` (`0x1403ed410`) also handles SARC entries
whose data offset is zero: when the resource is needed, it obtains the
entry's filename, calls `VfsFileSize`, and creates an external VFS request
with that filename. SARC can therefore supply the name of a DDSC whose
bytes are stored separately in TAB/ARC; the DDSC need not be embedded in SARC.
Explicit filename texture requests are another source, as described above.

The confirmed sequence is filename -> extension substitution -> filename
hash -> TAB entry. It is not DDSC hash -> ATX hash. A raw TAB scan lacks the
name provenance available to these game loading paths.
