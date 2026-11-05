# Just Cause 2 RBMDL (`.rbm`) file format

Analyzed from `JustCause2.exe` in the user's IDA database and
`exported/go142_lod1-d.rbm` (22,627 bytes). Addresses are image virtual
addresses in that executable. The sample validates **General version 3**;
other readable block layouts below come from static analysis of their
readers, not fixtures. Six registered types have no successful RBMDL read
path in this executable. This is not a claim about every engine build.

## Reader and dispatch

| Address | IDA name | Role |
|---|---|---|
| `0xA45D30` | `JC2_Model_ReadRBMDL` | Reads header, resolves each block hash, calls the block reader, checks its terminator. |
| `0x963570` | `JC2_RenderBlockFactory_CreateByHash` | Creates a registered block for a 32-bit type hash. |
| `0x715CB0` / `0x9362B0` | `JC2_AppSystem_RegisterRenderBlockTypes` / `JC2_RegisterRenderBlockTypes` | App-system callback and the factory's 21-type registration initializer. |
| `0x92E750` | `JC2_RenderBlockFactory_RegisterType` | Inserts a type descriptor keyed by its virtual hash getter; ignores duplicate hashes. |
| `0x973DA0` | `JC2_GeneralBlock_Read` | Reads the General material, strings, vertex and index buffers. |
| `0x91CA60` | `JC2_GeneralBlock_ReadTextureStrings` | Reads eight length-prefixed strings; submits nonempty slots 0–3 as textures. |
| `0x94C710` | `JC2_RenderBlock_ReadTexturesAndMode` | Calls the string reader, then reads a 32-bit mode; setter `0x6EEB50` stores its low 16 bits. |
| `0x971680` / `0x970E70` | `JC2_GeneralBlock_ReadVertices28` / `JC2_GeneralBlock_ReadVertices40` | Read counted arrays at the selected stride. |
| `0x92D7F0` | `JC2_GeneralBlock_ReadIndices16` | Reads a counted `u16` array. |
| `0x92FF00` / `0x973690` | `JC2_GeneralVertex28_ByteSwap` / `JC2_GeneralMaterialV3_ByteSwap` | Establish field boundaries for the packed vertex and material. |

Integers and floats in this sample are **little-endian**. The game has
big-endian byte-swap branches for the numeric header fields, material,
vertices and indices. Offsets below are file offsets; sizes are bytes. A
string is `u32 byte_length` followed by exactly that many bytes, with no
on-disk NUL terminator (`File::ReadString`, `0x4E5380`). The engine reader
handles zero length as empty and treats lengths above `0x400` as absent without
reading their bytes. A strict independent parser should reject oversize or
truncated lengths rather than copy that recovery behavior.

The shared counted-array helpers read a `uint32_t count` before the records
but gate record reads using a signed-positive comparison. An independent
parser should reject counts above `INT32_MAX` rather than interpreting that
engine quirk as a valid zero-length array. Unless a block section says
otherwise, the arrays below have **no padding, per-array marker, or
additional count**.

## Header

All declarations in this document describe **packed, little-endian on-disk
bytes**, not the in-memory C++ render-block objects. `uint8_t`, `uint16_t`
and `uint32_t` are from `<stdint.h>`; `float` is IEEE-754 binary32. Flexible
tails are written as comments because counted arrays and length-prefixed
strings cannot be represented by a single fixed-size C struct.

```c
#pragma pack(push, 1)
typedef struct RBMDLHeader {
    uint32_t name_length;     // 0x00: 5 in the sample
    char     name[5];         // 0x04: "RBMDL", no NUL; fixed only for this magic
    uint32_t major_version;   // 0x09: 1
    uint32_t minor_version;   // 0x0D: 13
    uint32_t reserved;        // 0x11: 0; read but not used here
    float    bbox_min[3];     // 0x15: XYZ
    float    bbox_max[3];     // 0x21: XYZ
    uint32_t block_count;     // 0x2D: 2
} RBMDLHeader;                // sizeof == 0x31 for the five-byte signature
#pragma pack(pop)
```

The sample's AABB minimum is `(-1.529480, -0.019476, -1.050000)` and maximum
is `(1.529480, 2.818974, 1.050001)`. `File::ReadString` reads the magic as a
variable-length string: `name[5]` above is valid only when `name_length == 5`;
there is no padding before `major_version`.

The reader compares the full signature with `RBMDL`. It checks the two
version words against `(1, 13)` **when** the global `byte_108A952` is set;
otherwise it bypasses that version check. The third word is read but not used
by this function. Version-word semantics beyond this comparison are unproven.

## Block framing

Starting at `0x31`, repeat `block_count` times, consecutively:

```
u32 type_hash
<type-specific block payload>
u32 end_marker                 // must be 0x89ABCDEF
```

`type_hash = 0xA7583B2B` selects the registered `General` block. The value
matches the game's `str_hash("General")` (lookup3 implementation). Unknown
hashes, failed block reads or a bad end marker make `JC2_Model_ReadRBMDL`
return failure and clear the model. The reader can destroy a successfully
parsed block instead of retaining it depending on that block's virtual
acceptance predicate. Do not assume the number retained equals the count in
the file.

### Registered block types

`JC2_AppSystem_RegisterRenderBlockTypes` (`0x715CB0`) invokes
`JC2_RegisterRenderBlockTypes` (`0x9362B0`) on the factory held at
`dword_11918E0`. The initializer constructs each descriptor and calls the
factory's virtual method at **vtable offset `+0x18`**, which is
`JC2_RenderBlockFactory_RegisterType` (`0x92E750`, vtable entry `0xF22844`).
That method obtains the descriptor's hash through its virtual getter at
`+0x08` and inserts the hash/type pair into the factory's sorted collection.
Type constructors assign the literal name and `str_hash(name)`; four
specialized constructors use the common `JC2_RenderBlockType_InitNamed`
(`0x933420`) to do so. `str_hash` (`0xA165A0`) calls lookup3 (`0xA16340`)
with a zero seed. The following hashes are **computed** from those constructor
names, not immediate constants stored in the initializer:

| Registration order | Block name | Type hash (`u32`) | Descriptor constructor |
|---:|---|---:|---:|
| 1 | `Occluder` | `0x2A44553C` | `0x929FA0` |
| 2 | `BillboardFoliage` | `0xAD529E70` | `0x94B260` |
| 3 | `Lambert` | `0xD5D78AE0` | `0x91FAE0` |
| 4 | `General` | `0xA7583B2B` | `0x91A390` |
| 5 | `Facade` | `0xCE39D7BF` | `0x95AA40` |
| 6 | `SkinnedGeneral` | `0x5E657F20` | `0x932250` |
| 7 | `CarPaint` | `0xCD931E75` | `0x9354D0` |
| 8 | `CarPaintSimple` | `0x81938490` | `0x92DFB0` |
| 9 | `Window` | `0x5B2003F6` | `0x958CC0` |
| 10 | `DeformableWindow` | `0x06B1F602` | `0x973C50` |
| 11 | `VegetationBark` | `0xB1F9133D` | `0x952530` |
| 12 | `VegetationFoliage` | `0xD79884C6` | `0x953B10` |
| 13 | `Halo` | `0x65D9B5B2` | `0x96E820` |
| 14 | `3DText` | `0x474ECF38` | `0x92C250` |
| 15 | `DecalDeformable` | `0xDF9E9916` | `0x9586F0` |
| 16 | `DecalSkinned` | `0x8A42C0C1` | `0x935130` |
| 17 | `DecalSimple` | `0x2BAFDD6C` | `0x95ADE0` |
| 18 | `Merged` | `0x9185A4C3` | `0x968530` |
| 19 | `Box` | `0x416C4035` | `0x942ED0` |
| 20 | `Beam` | `0x1B5E8315` | `0x930650` |
| 21 | `Flag` | `0xD885FCDB` | `0x94FAF0` |

There are 21 descriptor-constructor calls and 21 distinct hashes. The
initializer contains 22 textual calls to the registration method because
the final `Flag` registration has mutually exclusive success/failure paths
(the failure path passes null); it does **not** define a 22nd type. The
independent lookup3 calculation reproduces `General`'s `0xA7583B2B` tag in
the supplied `.rbm`. Shader program names such as `SkinnedGeneral8`,
`WindowSimple` and `SkinnedDecal` are **not** separate registered block
types: their enclosing descriptor constructors register `SkinnedGeneral`,
`Window` and `DecalSkinned`, respectively. This table is exhaustive for the
static factory initializer in this executable; it does not prove that no
other module can add types at runtime.


### Occluder (`0x2A44553C`, reader `0x9220F0`)

```c
#pragma pack(push, 1)
typedef struct OccluderPrefix {
    uint32_t type_hash;
    uint8_t  version;  // exactly 0
} OccluderPrefix;
typedef struct OccluderVertex12 {
    uint32_t words[3]; // storage layout; attribute semantics unknown
} OccluderVertex12;
#pragma pack(pop)
/*
 * uint32_t vertex_count; OccluderVertex12 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 * NO texture strings, mode, or material in this reader.
 */
```

`0x91E9B0` reads the counted 12-byte records and `0x92D7F0` reads the
indices. Version 0 is the sole accepted version.

### BillboardFoliage (`0xAD529E70`, reader `0x960490`)

```c
#pragma pack(push, 1)
typedef struct BillboardFoliagePrefix {
    uint32_t type_hash;
    uint8_t  version;  // exactly 0
} BillboardFoliagePrefix;
typedef struct BillboardFoliageVertex28 {
    float    position[3];       // first three floats used for a runtime mean
    uint32_t unknown_words[4];  // meaning unknown
} BillboardFoliageVertex28;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode; uint32_t vertex_count;
 * BillboardFoliageVertex28 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x951CD0` consumes eight strings but submits only nonempty slot 0.
`0x94F6E0` reads 28-byte records. The average of the first three floats
computed after loading is runtime data, **not** another serialized field.

### Lambert (`0xD5D78AE0`, reader `0x919160`)

```c
#pragma pack(push, 1)
typedef struct LambertMaterialV4 {
    uint32_t vertex_format;      // +0x00: 0 => 40-byte, 1 => 28-byte vertices
    float    position_scale;     // +0x04: uniform packed-position scale
    float    uv_scale[4];        // +0x08: primary XY, secondary XY
    float    unknown_float;      // +0x18
    uint8_t  packed[4];          // +0x1C: purpose unknown; not endian-swapped
    uint32_t flags;              // +0x20
    float    depth_offset;       // +0x24: clip-space z bias per clip-space w
    uint8_t  channel_selectors[2]; // +0x28: base/secondary channel selectors
    uint8_t  trailing_unknown[2];  // +0x2A: not endian-swapped
} LambertMaterialV4;             // 44 bytes
typedef struct LambertMaterialV3 {
    uint32_t flags;              // +0x00
    float    depth_offset;       // +0x04
    uint32_t vertex_format;      // +0x08
    float    position_scale;     // +0x0C
    float    uv_scale[4];        // +0x10
    float    unknown_float;      // +0x20
    uint8_t  packed[4];          // +0x24: not endian-swapped
} LambertMaterialV3;             // 40 bytes
typedef struct LambertMaterialV2 {
    uint32_t flags;
    float    depth_offset;
} LambertMaterialV2;             // 8 bytes
typedef struct LambertPrefix {
    uint32_t type_hash;
    uint8_t  version;            // 0, 2, 3, or 4; NOT 1
    // version 4: LambertMaterialV4 material;
    // version 3: LambertMaterialV3 material;
    // version 2: LambertMaterialV2 material;
    // version 0: uint32_t flags;
} LambertPrefix;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode;
 * if (derived vertex_format == 0):
 *     uint32_t vertex_count; GeneralVertex40 vertices[vertex_count];
 * else if (derived vertex_format == 1):
 *     uint32_t vertex_count; GeneralVertex28 vertices[vertex_count];
 * else:
 *     // NO vertex count or records; reader subsequently fails
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

The constructor's current version is 4; `0x919160` additionally recognizes
legacy versions 0, 2 and 3. Version 4 reads 44 material bytes (`0x9191AA`);
version 3 reads 40 (`0x94AFE0`), version 2 reads eight (`0x921630`), and
version 0 reads only a `uint32_t flags` (`0x965460`). The runtime constructor
(`0x967D00`) initializes vertex format 0, position/UV scales to 1, the
unclassified float to 1, packed bytes to `FF FF FF FF`, flags to `0x40`,
depth offset to 0 and both channel selectors to 0. Version 0 replaces
the flags with the file word **OR `0x40`** and keeps the other defaults;
version 2 replaces flags and depth offset without forcing `0x40`.
Version 3 reads flags and depth offset first, followed by the 32-byte
format/scale/UV/float/packed group, and copies that group to the v4
runtime positions (`0x928690`); its absent channel selectors stay zero.
The two unclassified trailing bytes are not initialized by the game
constructor for older versions; the independent reader zeroes them
deterministically. None of these synthesized values are extra file bytes.
`0x92EB30` submits nonempty texture slots 0–2; `0x970E70` and `0x971680`
read the respective vertex formats, then `0x92D7F0` reads indices.

Lambert's draw path (`0x93E110`) uploads material `depth_offset` to vertex
shader `cb2[0].x`, where the `lambert` VS at bundle offset `0x33774` computes
`clip_z += depth_offset * clip_w`. It uploads the four UV scale components
to VS `cb2[1]`. `position_scale` is used by the packed-position path
(`0x926050`, `0x929420`, `0x971030`). The first two trailing bytes select
one-hot pixel-shader channel masks for the base and secondary color samples
when flags `0x80` and `0x100` enable grayscale conversion. In the `lambert`
PS (`0x5DCEAC`), slot 0 supplies base color, slot 1 a tangent-space normal,
and slot 2 a multiplicative secondary color; the masks dot the respective
sample's channels before multiplication. The `+0x18` float, four packed
bytes and last two trailing bytes remain unclassified.

#### Lambert material flags (`material +0x20`)

| Bit | Observed effect |
|---:|---|
| `0x01` | Changes the pass/acceptance predicate (`0x94B0F0`) and, together with `0x02`, selects an alternate draw/input-layout path (`0x93E110`). **Likely** transparent/blended rendering; the source-art name is unconfirmed. |
| `0x02` | Enables the alpha-test render state (`0x93E110` → `0x952CC0`); also selects the alternate pass/layout path. |
| `0x04` | Disables back-face culling: state argument 1 instead of 3 (`0x93E110`). |
| `0x08` | Skips the requirement and binding for block texture slot 1, the normal map (`0x919160`, `0x93E110`). |
| `0x10` | Skips the requirement and binding for block texture slot 2, the secondary color (`0x919160`, `0x93E110`). |
| `0x20` | Enables per-instance pixel-shader constants derived from runtime color/state (`0x929420`); exact artistic meaning unproven. |
| `0x40` | Permits a runtime-dependent shader permutation (`0x929420`); version-0 loading forces this bit on. It is **not** established as a snow property. |
| `0x80` | Enables grayscale conversion of base texture slot 0, with its channel selected by material byte `+0x28` (`0x93E110`). |
| `0x100` | Enables grayscale conversion of secondary texture slot 2, with its channel selected by material byte `+0x29` (`0x93E110`). |

Other Lambert flag bits are unclassified. These assignments must not be
transferred to General or other block types.

### General payload (`0xA7583B2B`, reader `0x973DA0`)

`JC2_GeneralBlock_Read` (`0x973DA0`) accepts block version 3, and also a
legacy version 2 whose shorter material is expanded into the current runtime
form. The type word belongs to the outer block frame; it is included here to
make each prefix's position explicit.

```c
#pragma pack(push, 1)
typedef struct GeneralMaterialV3 {
    float    channel_texture_mask[4]; // +0x00: base RGBA grayscale weights
    float    channel_ao_mask[3];       // +0x10: secondary RGB grayscale weights
    float    channel_ao_grayscale;     // +0x1C: enable secondary grayscale when > 0.5
    float    depth_offset;             // +0x20: clip-space z offset per clip-space w
    float    specular_power;           // +0x24: specular exponent
    uint32_t vertex_format;            // +0x28: 0 => 40 bytes; 1 => 28 bytes
    float    position_scale;           // +0x2C: uniform packed-position scale
    float    primary_uv_scale[2];      // +0x30: primary UV X/Y
    float    secondary_uv_scale[2];    // +0x38: secondary UV X/Y
    float    unknown_parameter;        // +0x40: purpose unverified
    uint8_t  packed_parameters[4];     // +0x44: four bytes, purpose unverified
    uint32_t flags;                    // +0x48
} GeneralMaterialV3;                   // sizeof == 76

typedef struct GeneralMaterialV2 {
    float    channel_texture_mask[4]; // +0x00
    float    channel_ao_mask[3];       // +0x10
    float    channel_ao_grayscale;     // +0x1C
    float    depth_offset;             // +0x20
    float    specular_power;           // +0x24
    uint32_t vertex_format;            // +0x28
    float    position_scale;           // +0x2C
    float    primary_uv_scale;         // +0x30: duplicated to X and Y at load
    float    secondary_uv_scale;       // +0x34: duplicated to X and Y at load
    float    unknown_parameter;        // +0x38
    uint8_t  packed_parameters[4];     // +0x3C
    uint32_t flags;                    // +0x40
} GeneralMaterialV2;                   // sizeof == 68

typedef struct GeneralV3Prefix {
    uint32_t type_hash;          // block +0x00: 0xA7583B2B
    uint8_t  version;            // block +0x04: 3
    GeneralMaterialV3 material;  // block +0x05
} GeneralV3Prefix;               // sizeof == 0x51

typedef struct GeneralV2Prefix {
    uint32_t type_hash;          // block +0x00: 0xA7583B2B
    uint8_t  version;            // block +0x04: 2
    GeneralMaterialV2 material;  // block +0x05
} GeneralV2Prefix;               // sizeof == 0x49

typedef struct GeneralVertex28 {
    int16_t texcoord0[4];  // +0x00: TEXCOORD0, R16G16B16A16_SNORM
    float   texcoord1[3];  // +0x08: TEXCOORD1, R32G32B32_FLOAT
    int16_t position[4];   // +0x14: POSITION0, R16G16B16A16_SNORM
} GeneralVertex28;         // sizeof == 28

typedef struct GeneralVertex40 {
    float position[3];     // +0x00: POSITION0, R32G32B32_FLOAT
    float texcoord0[4];    // +0x0C: TEXCOORD0, R32G32B32A32_FLOAT
    float texcoord1[3];    // +0x1C: TEXCOORD1, R32G32B32_FLOAT
} GeneralVertex40;         // sizeof == 40

typedef struct GeneralGeometryCounts {
    uint32_t mode;               // after eight strings; setter retains low 16 bits
    uint32_t vertex_count;
    // vertex_count * (vertex_format == 0 ? 40 : 28) bytes
    // uint32_t index_count;
    // uint16_t indices[index_count];
    // uint32_t end_marker;      // 0x89ABCDEF; read by the outer model reader
} GeneralGeometryCounts;
#pragma pack(pop)
```

After either prefix, the reader consumes exactly **eight** consecutive
`uint32_t byte_length; uint8_t bytes[byte_length];` string records. Only
nonempty slots 0–3 are submitted for texture loading. The sample then has
`mode == 3`, followed by `vertex_count`, counted vertices, `index_count` and
counted 16-bit indices as shown above. Unknown `vertex_format` values cannot
produce the vertex buffer required by this reader. The sample's index counts
are divisible by three, but triangle topology is not established by this
read function alone.

These names follow the editable material properties at `0x935840` and the
installed DX10 `general` shaders. With flag `0x20` (`UseChannelTextures`),
`channel_texture_mask` dots the base texture's RGBA into grayscale RGB.
`channel_ao_mask` dots the secondary texture's RGB when
`channel_ao_grayscale > 0.5`; `UseChannelAO` sets that selector to 1 and flag
`0x200`. `0x949B50` also records the first positive component of each enabled
mask as a runtime channel index. Editable `DepthOffset` is multiplied by
`1e-5` before serialization; the stored `depth_offset` biases clip-space z
by `depth_offset * clip_w`. `specular_power` is the specular-lobe exponent.
The General vertex shader applies `primary_uv_scale` to TEXCOORD0.xy and
`secondary_uv_scale` to TEXCOORD0.zw. The final float and four-byte quartet
have no demonstrated shader use in this path; the quartet is **not** a float.
For version 2 the reader expands its two scalar UV scales into runtime
`(primary_x, primary_y) = (p1, p1)` and
`(secondary_x, secondary_y) = (p2, p2)`; its last float remains unclassified.
This legacy variant has no fixture here. The vertex field types and semantic
labels below come from Direct3D 10 input declarations, **not** the endian
swap alone. `TEXCOORD` denotes an input semantic, not proof every component
is an ordinary UV coordinate.

The General draw path (`0x972CE0`) binds block texture slots 0–3. In the
`general` PS (`0x5C7FB8`), slot 0 is base color, slot 1 the tangent-space
normal, slot 2 supplies surface/mask values and slot 3 is sampled at the
secondary UV and optionally reduced through `channel_ao_mask`. The shader
computes its specular lobe with `log(dot) * specular_power`, then `exp`.
The supplied General-v3 material has flags `0x60`,
`specular_power == 16.0`, and channel-texture weights `(0, 1, 0, 0.2)`;
it corroborates the serialized offsets but cannot validate another block
type's material.

The General glTF export applies this base mask only when `flags & 0x20`.
It computes the weighted RGBA sum for each decoded 8-bit RGB(A) texel,
replicates the result into RGB, and keeps the original alpha. This is not
per-channel multiplication: a one-hot weight chooses a packed texture
channel, while multiple nonzero weights (as in the sample above) mix them.
When `0x20` is clear, the original base texture is exported unchanged.
`tests/test_jc2_general_channels.py` exercises both paths, a mixed mask,
and one-hot red/alpha masks through an exported glTF. The secondary
slot-3/AO path remains a separate renderer-specific operation.

#### General material flags (`material +0x48`)

`0x935840` maps editable render-mode/property names to these bits; the draw
path `0x972CE0` confirms the indicated render-state and shader uses.

| Bit | Property or effect |
|---:|---|
| `0x01` | `NoCulling`: cull-state argument 1 instead of 3. |
| `0x02` | `WithAlpha`: alpha-blended mode. |
| `0x04` | Additive-alpha modifier: `WithAdditiveAlpha` sets **both** `0x02` and `0x04`. |
| `0x20` | `UseChannelTextures`: dot base RGBA against material `+0x00..+0x0C` for grayscale RGB. |
| `0x40` | `UseSnow`: enables the snow shader path. |
| `0x80` | `AnimateTexture`: enables texture animation. |
| `0x100` | `AlphaTest`: toggles alpha-test state. |
| `0x200` | `UseChannelAO`: sets the material `+0x1C` secondary-grayscale selector and enables channel AO. |

`0x08`, `0x10` and higher unlisted bits have no demonstrated source-property
meaning here. These masks are **General-specific**; Lambert assigns different
effects to several of the same bit positions.

#### Direct3D 10 vertex declarations

`0x92A8F0` constructs `D3D10_INPUT_ELEMENT_DESC` records and calls the
device's `CreateInputLayout` virtual method at `+0x138` with a shader input
signature. Its 20-byte internal token is `(input_slot, component_count,
storage_kind, semantic_kind, semantic_index)`; the format lookup
`dword_F223C4[4*storage_kind + component_count]` gives `DXGI_FORMAT`, and
`dword_F22314[...]` advances the per-slot byte offset. Semantic table
`off_108D0D8` maps `0` to `POSITION`, `5` to `TEXCOORD`, and `14` to `VOID`
(skip bytes without emitting an input element). These are **D3D10** input
layouts, not Direct3D 9 vertex declarations. Microsoft documents the
[input element fields](https://learn.microsoft.com/en-us/windows/win32/api/d3d10/ns-d3d10-d3d10_input_element_desc),
[DXGI format values](https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format)
and [`CreateInputLayout` signature](https://learn.microsoft.com/en-us/windows/win32/api/d3d10/nf-d3d10-id3d10device-createinputlayout).

The **complete** layouts created in General's descriptor constructor
`0x91A390` (Lambert `0x91FAE0` creates the same pair) are:

```cpp
// input slot 0; per-vertex, instance step rate 0 throughout.
// vertex_format == 1: 28 bytes, General descriptor[26] / Lambert[6].
D3D10_INPUT_ELEMENT_DESC vertex28[] = {
    {"TEXCOORD", 0, DXGI_FORMAT_R16G16B16A16_SNORM, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT,     0,  8, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"POSITION", 0, DXGI_FORMAT_R16G16B16A16_SNORM, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0},
};
// vertex_format == 0: 40 bytes, General descriptor[25] / Lambert[5].
D3D10_INPUT_ELEMENT_DESC vertex40[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT,    0, 28, D3D10_INPUT_PER_VERTEX_DATA, 0},
};
```

General renderer `0x972CE0` binds descriptor `[25 + vertex_format]` on the
main path through `0x951F60` (`IASetInputLayout`); Lambert renderer
`0x93E110` binds `[5 + vertex_format]`. Reduced **shadow/depth** layouts
reuse the same vertex buffers and stride: format 0 uses POSITION0 float3
at `+0` alone, or POSITION0 float3 `+0` plus TEXCOORD0 float2 `+12`.
Format 1 uses POSITION0 SNORM4x16 at `+20` alone, or TEXCOORD0
SNORM2x16 `+0` plus POSITION0 SNORM4x16 `+20`. General descriptor slots
`[27, 28]` and `[29, 30]` implement these subsets for formats 0 and 1;
Lambert slots `[7, 8]` and `[9, 10]` mirror them. The builder's `VOID`
tokens account for skipped bytes; these are not alternate serialized
layouts.

The matching shader variants confirm why those subsets omit attributes:
`tex0shadow` (DXBC `0x4836C`) reads only POSITION0.xyz;
`generalshadow` (`0x2F654`), `generalprez` (`0x2F0FC`) and
`generalprezvelocity` (`0x2F334`) additionally read TEXCOORD0.xy.
`generaloutline` (`0x2EBD8`) instead decodes a normal from
TEXCOORD1.x to displace the silhouette.

For the supplied General-v3 file, POSITION0's signed-normalized XYZ at
vertex `+0x14`, multiplied by `material.position_scale == 2.81897449`,
reconstructs the model AABB from all 661 sample vertices within
`0.0001` per coordinate. Render functions `0x9188E0` and `0x920950`
likewise build a uniform scale matrix from that material parameter. Every
sample POSITION0.w is `-13108` (approximately `-0.4` after SNORM conversion);
its serialized purpose remains unknown. The full General and Lambert vertex
shaders declare only POSITION0.xyz, so these paths do not read its `.w`.

The big-endian helper `0x92FF00` swaps all four 16-bit components at
`+0x00`, all three float words at `+0x08`, but only the first three
16-bit POSITION components at `+0x14`. It leaves POSITION.w bytes
`+0x1A..+0x1B` unchanged even though the D3D declaration treats them
as a fourth SNORM16 component. The sample has `0xCCCC` there, whose
byte-reversal is indistinguishable; do not assume big-endian correctness
for a non-symmetric fourth component.

#### Packed surface attributes inside TEXCOORD

The installed `DX10_Shaders_F.shader_bundle` contains the named `general`
vertex shader (DXBC at file offset `0x2DF78`, 3152 bytes) and pixel shader
(DXBC at `0x5C7FB8`, 13128 bytes). `0x951920` loads the `General` vertex
shader used to create **both** full General input layouts. The `lambert`
vertex shader (`0x33774`, 2984 bytes) is similarly used for both Lambert
layouts by `0x91FAE0`. Their inputs are `POSITION0.xyz`, `TEXCOORD0.xyzw`
and `TEXCOORD1.xyz` regardless of whether the vertex buffer is 28 or 40
bytes. Thus the same shader interpretation applies to either storage
format; the supplied fixture exercises only the 28-byte General path.

For General, let `uv = TEXCOORD0` and `packed = TEXCOORD1`, after DXGI
converts the 28-byte format's signed-normalized 16-bit `uv` components
to floats (the 40-byte format already stores floats). The vertex shader
uses `uv.xy` as a primary UV pair (material scale plus a 2D affine
transform) and `uv.zw` as another UV pair (material scale). The pixel
shader samples its base-color and normal textures at the primary pair;
it also samples another texture at the secondary pair. **These are UVs,
not stored normals/tangents.** The shader decodes the other channel as:

```text
unpack_unit(v) = normalize(2 * frac(v * (1, 1/256, 1/65536)) - 1)
normal         = unpack_unit(packed.x)
tangent        = unpack_unit(abs(packed.y))
bitangent_sign = -sign(packed.y)   // +1 if negative, -1 if positive, 0 if zero
vertex_rgb     = frac(packed.z * (1, 1/64, 1/4096))
```

`frac(x)` means `x - floor(x)`, componentwise. The vertex shader transforms
the two unpacked unit vectors with the surface-frame matrix and outputs
them as TEXCOORD5/6 (`o6.xyz`/`o7.xyz`), plus `bitangent_sign` in `o7.w`.
The General pixel shader (DXBC `0x5C7FB8`) normalizes these vectors,
computes `cross(normal, tangent) * bitangent_sign`, samples the normal
map at the primary UV, converts its RGB from `[0, 1]` to `[-1, 1]`, and
combines its tangent-space XYZ with tangent, bitangent and normal.
This **confirms** packed normal, tangent and handedness in the three
`TEXCOORD1` floats, despite their large numerical values. Its `vertex_rgb`
is output as COLOR0 and multiplied with the sampled base color; COLOR0.a
comes from a material constant rather than `packed.z`.

The Lambert vertex shader uses the same packed normal/tangent/sign decode,
but extracts **RGBA** instead of RGB from `packed.z` using fractional
multipliers `(1, 1/64, 1/4096, 1/262144)`. Its pixel shader (DXBC
`0x5DCEAC`) also builds the signed bitangent and applies the normal map.
No claim about unrelated RBMDL block types follows from these shaders.

In the 661-vertex General fixture the unpacked normal and tangent have
median `abs(dot)` `0.01282` and maximum `0.02771`, consistent with a
quantized near-orthogonal basis. The decoded normals also agree with
normals averaged from indexed triangles (median dot product `0.999890`;
minimum `0.933401`). `packed.y` is positive throughout, so the shader
outputs bitangent sign `-1` for this model. `packed.z` has four distinct
values. The near-unit, near-orthogonal vectors and the pixel shader's
normal-map reconstruction independently corroborate the packing; a
sample for the 40-byte General stream is not available here.

`src/apex/rbmdl/rbmdl.cpp` exports both General vertex formats as float
`POSITION`, `TEXCOORD_0`/`TEXCOORD_1` (two `vec2` UV sets), `NORMAL` (`vec3`),
`TANGENT` (`vec4`, with the shader's bitangent sign in `.w`) and `COLOR_0`
(`vec3` RGB). Format 1 first converts its 16-bit UV components as SNORM,
clamping `-32768` to `-1`; format 0 takes its float UV components directly.
UVs retain their file values before the shader's material-dependent scale
and affine transform. The General vertex shader sources color
alpha from a material constant, so the exporter does not invent a fourth
per-vertex color component.

### Facade (`0xCE39D7BF`, reader `0x96CC20`)

```c
#pragma pack(push, 1)
typedef struct FacadeMaterial {
    float    base_channel_mask[4];      // +0x00: PS base RGBA dot weights if flags & 0x20
    float    secondary_channel_mask[3]; // +0x10: PS secondary RGB dot weights if flags & 0x10
    float    lighting_color_target[3];  // +0x1C: multiplied by 4 for PS color target
    float    depth_offset;              // +0x28: clip-space z bias per clip-space w
    float    specular_power;            // +0x2C: PS specular exponent
    uint32_t vertex_format;             // +0x30: 0 => 44 bytes, 1 => 40 bytes
    float    position_scale;            // +0x34: format-1 packed-position scale
    uint32_t flags;                     // +0x38
} FacadeMaterial;                        // 60 bytes
typedef struct FacadePrefix {
    uint32_t type_hash;
    uint8_t  version;         // exactly 1
    FacadeMaterial material;   // block +0x05
} FacadePrefix;                // 65 bytes
typedef struct FacadeVertex44 {
    float position[3];       // +0x00: POSITION0, R32G32B32_FLOAT
    float texcoord0[4];      // +0x0C: TEXCOORD0, two UV pairs
    float texcoord1[4];      // +0x1C: TEXCOORD1, packed attributes
} FacadeVertex44;            // 44 bytes
typedef struct FacadeVertex40 {
    int16_t position[4];     // +0x00: POSITION0, R16G16B16A16_SNORM
    float texcoord0[4];      // +0x08: TEXCOORD0, two UV pairs
    float texcoord1[4];      // +0x18: TEXCOORD1, packed attributes
} FacadeVertex40;            // 40 bytes
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode;
 * if (vertex_format == 0):
 *     uint32_t vertex_count; FacadeVertex44 vertices[vertex_count];
 * else if (vertex_format == 1):
 *     uint32_t vertex_count; FacadeVertex40 vertices[vertex_count];
 * else:
 *     // vertex count and records are NOT read
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x96CC20` directly reads 60 material bytes; swap routine `0x93E570`
establishes the component widths. `0x967D40` submits nonempty texture slots
0–3. Vertex readers `0x92AD00` (44 bytes) and `0x91C570` (40 bytes) precede
the index reader `0x92D7F0`. Unlike Lambert, an unsupported vertex selector
skips its vertex count/records yet this reader can still return success;
don't consume a count in that branch.

#### Facade vertex declarations and packed channels

Descriptor constructor `0x95AA40` builds two full D3D10 input layouts for
the same `facade` VS (DXBC `0x1EC0C`). Format 0 (descriptor `[18]`)
uses `POSITION0` float3 at `+0x00`, `TEXCOORD0` float4 at `+0x0C` and
`TEXCOORD1` float4 at `+0x1C`. Format 1 (descriptor `[19]`) replaces
position with SNORM16x4 at `+0x00`, then has `TEXCOORD0` float4 at
`+0x08` and `TEXCOORD1` float4 at `+0x18`. The shader input signature
confirms these three semantics and consumes only position XYZ. The
format-1 position scale is supplied through `0x9663B0`; POSITION.w's
meaning is unproven. In the big-endian path, `0x91F2F0` swaps **all
four** 16-bit position components then the eight float words; format 0's
`0x949020` swaps eleven float words.

For either format, `texcoord0.xy` drives block textures 0–2 and
`texcoord0.zw` drives block texture 3 (`facade` PS `0x4A8018`). The VS
passes those pairs through without a material UV scale. Its four
`TEXCOORD1` floats have different uses:

```text
unpack_unit(v) = normalize(2 * frac(v * (1, 1/256, 1/65536)) - 1)
normal         = unpack_unit(texcoord1.x)
tangent        = unpack_unit(abs(texcoord1.y))
bitangent_sign = -sign(texcoord1.y)
vertex_rgb     = frac(texcoord1.z * (1, 1/64, 1/4096))
lookup_u       = frac(texcoord1.w)
lookup_v       = floor(texcoord1.w) / 2048
```

The `facade` VS normal/tangent decode and cross-product sign match the
normal-map reconstruction in its PS. It also emits a fourth channel from
`texcoord1.z` using multiplier `1/262144`, but this named PS variant
reads only RGB and multiplies it with sampled base color. The shader
samples a **type/scene** texture at slot 10 with
`(lookup_u, lookup_v)` and an offset lookup; `texcoord1.w` is a packed
lookup coordinate, not a third ordinary UV pair. The exact artistic
meaning of its lookup values and the unused fourth color channel is
not established. No Facade fixture is available here: storage and uses
are established by reader, input layout, and DXBC rather than sample
vertex bytes.

The draw path `0x9260F0` binds four block textures (or a dummy for missing
slot 3). It uploads material `+0x28` to VS `cb1[11].x`; the `facade` VS
(DXBC `0x1EC0C`) computes `clip_z += depth_offset * clip_w`. It uploads
`+0x2C` to PS `cb2[1].w`; the `facade` PS (`0x4A8018`) computes
`exp(log(dot) * specular_power)`. Format 1 uses `+0x34` as a position
scale in the geometry setup (`0x9663B0`). Material `+0x1C..+0x24`,
multiplied by four, supplies PS `cb2[0].xyz`; the shader interpolates
toward that color based on a scene texture, but the artistic name of
this color target is unconfirmed.

The base texture is sampled at the first UV pair. With flag `0x20`,
`0x962270` uploads material `+0x00..+0x0C` as PS `cb1[1]`, and the
pixel shader dots base RGBA against it to produce grayscale RGB.
With flag `0x10`, `0x9260F0` uploads `+0x10..+0x18` as PS `cb2[1].xyz`;
the shader dots the slot-3 secondary RGB sample at the second UV pair.
Slot 1 is a tangent-space normal; slot 2 supplies further surface values.
Shader resources beyond block slots 0–3 are renderer/scene bindings.
Flag `0x1` disables back-face culling (state argument 1 versus 3);
`0x2` makes the `0x94A590` pass predicate false, without a proven
blending name. Flag `0x40` chooses per-instance pixel-color/lighting
constants (`0x962270`); the remaining flag meanings are unproven.

The object constructor (`0x964D20`) initializes base weights
`(1,1,1,0)`, secondary weights `(1,1,1)`, color target
approximately `(1.8,1.8,1.6)`, depth offset 0, specular power 16,
format 0, position scale 1 and flags 0. The version-1 reader overwrites
all 60 material bytes; these defaults are **not** extra serialized data.

#### Facade export support

`FacadeRenderBlock` handles version 1, selectors 0 and 1, and mode 3. It
exports indices, positions (including the format-1 SNORM16 scale), both UV
pairs, and decodes `TEXCOORD1.xyz` into glTF `NORMAL`, `TANGENT` (including
handedness), and `COLOR_0`. Flag `0x1` maps to `doubleSided`. Slot 0 is the
base texture; flag `0x20` converts its decoded 8-bit RGB(A) pixels to the
shader's weighted grayscale. Slot 1 is the normal map. Slot 2 gets the
same approximate metallic/roughness channel mapping used for General.

The glTF material cannot reproduce the slot-3 color multiplication at a
separate UV pair, the scene texture sampled with `TEXCOORD1.w`, or the
dynamic lighting/specular constants and exact clip-space depth bias.
Consequently slot 3 and the lookup coordinate are not exported, and flag
`0x2` is **not** assumed to mean alpha blending. Grayscale conversion
supports decoded 8-bit RGB(A)
textures; unsupported pixel layouts report an error rather than silently
exporting an unmasked texture. `tests/test_jc2_facade.py` exports synthetic
blocks of both vertex formats through the JC2 CLI and checks their glTF
attributes; real Facade asset appearance remains unverified without a fixture.

### Export-time decal depth separation

General, Lambert and Facade add `depth_offset * clip_w` to clip-space Z
in their vertex shaders. This shifts normalized-device Z by a constant,
but **cannot** be baked into model-space positions exactly for every
camera. glTF has no portable material depth-bias setting. The exporter
therefore offsets each position along its already-decoded local normal by
`depth_offset * max(1, abs(position_scale)) * 100` model units. Positive
values move decals outward; zero leaves geometry untouched. Shading
normals, UVs and indices are unchanged. The factor is a conservative
geometry approximation, not a recovered game transform; depth testing
can still differ at extreme camera distances or grazing angles.

The `geo.dpv.ee.epe` example's `gb220_lod1-a0.rbm` and
`gb220_lod1-a_dst.rbm` both contain `logo_chaos_dif.dds` General blocks
with `depth_offset ≈ 1e-5`. Eight of 20 and thirteen of 30 logo vertices,
respectively, exactly coincide with substrate vertices in the previous
glTF. Re-exporting these models moves the logo along its normals by
approximately `0.003525` and `0.003005` local units while leaving
depth-zero substrate primitives unchanged. Synthetic CLI coverage for
positive/negative/zero offsets across the three block types is in
`tests/test_jc2_depth_offset.py`.

### SkinnedGeneral (`0x5E657F20`, reader `0x92D350`)

```c
#pragma pack(push, 1)
typedef struct SkinnedMaterial28 {
    uint32_t words[7];       // words[0] & 0x80000 selects 28-byte vertices
} SkinnedMaterial28;
typedef struct SkinnedGeneralPrefix {
    uint32_t type_hash;
    uint8_t  version;        // exactly 3
    SkinnedMaterial28 material;
} SkinnedGeneralPrefix;
typedef struct SkinPrimaryVertex20 { uint32_t words[5]; } SkinPrimaryVertex20;
typedef struct SkinPrimaryVertex28 { uint32_t words[7]; } SkinPrimaryVertex28;
typedef struct SkinExtraVertex20   { uint32_t words[5]; } SkinExtraVertex20;
typedef struct SkinBatchPrefix {
    uint32_t first_raw;
    uint32_t second_raw;
    uint32_t entry_count;
    // uint16_t entries[entry_count]; // follows immediately
} SkinBatchPrefix;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode;
 * uint32_t primary_count;
 * if (material.words[0] & 0x80000):
 *     SkinPrimaryVertex28 primary[primary_count];
 * else:
 *     SkinPrimaryVertex20 primary[primary_count];
 * uint32_t extra_count; SkinExtraVertex20 extra[extra_count];
 * uint32_t batch_count;
 * for (uint32_t i = 0; i < batch_count; ++i):
 *     SkinBatchPrefix batch; uint16_t entries[batch.entry_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

The reader uses `0x934FB0` or `0x958A60` for the conditional primary array,
`0x927E00` for the second 20-byte array, and `0x971C40` for counted batches.
Each batch's virtual reader `0x974010` consumes two words, then a counted
16-bit array; the in-memory batch's vptr and pointers are **not** file fields.
`0x92EB30` submits nonempty texture slots 0–2; `0x931080` binds them.
In the first material word, `0x80000` changes the primary stream from
20 to 28 bytes and selects the matching input layout. Exact low-word
values `1` and `4` select Hair and EyeGloss shader families (`0x9657C0`);
value `4` also binds scene textures at GPU slots 6/7, which are **not**
block texture strings. Bits `0x10000` and `0x40000` alter renderer/pass
selection, while `0x100000` permits a conditional shader-family variant;
their authored property names are unknown. The third material word
(file block `+0x0D`) scales a distance-dependent pixel-shader constant.
The named `skinnedgeneral` VS at bundle offset `0x413E0` reads additional
weight/index-like inputs; exact batch and vertex attribute labels remain
inferred rather than established by these declarations alone.

### CarPaint (`0xCD931E75`, reader `0x955EF0`)

```c
#pragma pack(push, 1)
typedef struct CarMaterial56 {
    float    paint_rgb_a[3];           // +0x00: first shader paint color
    float    paint_rgb_b[3];           // +0x0C: second shader paint color
    float    lighting_factor;          // +0x18: uploaded to PS multiplied by 8
    float    depth_offset;             // +0x1C: clip-space z bias
    float    highlight_factor;         // +0x20: highlight/reflection blend
    float    conditional_weights[4];   // +0x24: environment-color weighting inferred
    uint32_t flags;                    // +0x34
} CarMaterial56;                       // 56 bytes
typedef struct CarPaintPrefix {
    uint32_t type_hash;
    uint8_t  version;       // 1, 2, 3 or 4; current descriptor reports 3
    CarMaterial56 material;
} CarPaintPrefix;
typedef struct CarLegacyVertex52 {
    uint32_t words[4];
    uint16_t halves[4];
    uint32_t tail_words[7];
} CarLegacyVertex52;
typedef struct CarVertex24 {
    uint32_t words[4];
    uint16_t halves[4];
} CarVertex24;
typedef struct CarAuxVertex28 { uint32_t words[7]; } CarAuxVertex28;
typedef struct CarDeformLookup {
    uint32_t words[256];    // exactly 1024 bytes; NO preceding count
} CarDeformLookup;
#pragma pack(pop)
/*
 * Versions 1 and 2:
 *     Eight length-prefixed strings; uint32_t mode;
 *     uint32_t vertex_count; CarLegacyVertex52 vertices[vertex_count];
 *     uint32_t index_count; uint16_t indices[index_count];
 *     CarDeformLookup lookup; outer end_marker.
 *
 * Version 3:
 *     Eight length-prefixed strings; uint32_t mode;
 *     uint32_t vertex_count; CarVertex24 vertices[vertex_count];
 *     uint32_t auxiliary_count; CarAuxVertex28 auxiliary[auxiliary_count];
 *     uint32_t index_count; uint16_t indices[index_count];
 *     CarDeformLookup lookup; outer end_marker.
 *
 * Version 4:
 *     CarDeformLookup lookup; // BEFORE the strings
 *     Eight length-prefixed strings; uint32_t mode;
 *     uint32_t vertex_count; CarVertex24 vertices[vertex_count];
 *     uint32_t auxiliary_count; CarAuxVertex28 auxiliary[auxiliary_count];
 *     uint32_t index_count; uint16_t indices[index_count];
 *     outer end_marker.
 */
```

All four versions read the same 56-byte material (`0x93C680` swaps its
fourteen words). Versions 1 and 2 read 52-byte vertices, then
`0x91E590` converts/reorders them into smaller **runtime** buffers; do not
mistake that conversion for further on-disk arrays. Versions 3 and 4 read
separate 24- and 28-byte arrays (`0x951D80`, `0x96AA90`). The embedded
deformation-table reader `0x9669E0` consumes exactly 256 32-bit lookup
entries (`0x8B68E0`) and derives a 16-bit runtime table. `0x952BD0`
submits nonempty texture slots 0–5.

`0x93F540` uploads the two stored RGB triples to PS `cb1[6/7].xyz`;
the `carpaint` PS at bundle offset `0xEDA84` interpolates between them.
Material `+0x1C` reaches VS `cb1[4].x`, which biases clip-space z by
that value times clip-space w (also observed in `carpaintsimple` VS
`0x180C8`). `+0x18` is multiplied by eight for a PS lighting term;
`+0x20` affects highlight/reflection mixing; the four floats at
`+0x24..+0x30` conditionally weight an environment contribution. Their
precise artistic names are not established. Flag `0x1` disables back-face
culling; full CarPaint's `0x8` selects these stored colors instead of
external runtime colors. Other flag bits select passes and permutations
(`0x93F540`, `0x948650`, `0x953DA0`), not established source properties.
The PS samples slot 0 as base RGB, slot 1 as a normal, slots 2/3 as
surface mask/secondary-UV overlay, slot 4 as a secondary normal and slot 5
as a secondary color multiplier. Type-level `car_dirt.dds` at GPU slot 6
is not a seventh block texture string.

### CarPaintSimple (`0x81938490`, reader `0x956390`)

```c
#pragma pack(push, 1)
typedef struct CarPaintSimplePrefix {
    uint32_t type_hash;
    uint8_t  version;      // exactly 1
    CarMaterial56 material;
} CarPaintSimplePrefix;
typedef struct CarSimpleVertex32 { uint32_t words[8]; } CarSimpleVertex32;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode; uint32_t vertex_count;
 * CarSimpleVertex32 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 * NO 256-word deformation table is read by this variant.
 */
```

The same 56-byte material swap `0x93C680` is used; `0x91A300` reads the
counted 32-byte vertices, and `0x9491E0` submits nonempty texture slots
0–2. `0x93C8C0` binds the same paint colors and depth offset; this
variant does not bind all six full-CarPaint block textures. Its flag
`0x80000000` selects an alternate input layout/pipeline (`0x9223EE`).

### Window (`0x5B2003F6`, reader `0x937420`)

```c
#pragma pack(push, 1)
typedef struct WindowMaterial {
    float    specular_power;    // +0x00: shader specular exponent
    uint32_t flags;             // +0x04
} WindowMaterial;               // 8 bytes
typedef struct WindowPrefix {
    uint32_t type_hash;
    uint8_t  version;           // exactly 0
    WindowMaterial material;    // block +0x05
} WindowPrefix;
typedef struct WindowVertex40 { uint32_t words[10]; } WindowVertex40;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode; uint32_t vertex_count;
 * WindowVertex40 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x921110` swaps the two material words; `0x970E70` reads 40-byte vertices.
`0x967D40` submits nonempty texture slots 0–3. The draw path `0x9545F0`
uploads `specular_power` to PS `cb1[0].x`; the `window` PS at bundle offset
`0x735C68` uses `exp(log(dot) * cb1[0].x)`. Flag `0x4` selects WindowSimple
shaders and reduced texture setup (`0x963720`); `0x2` suppresses an extra
pass and changes raster state, but its source-art name is unknown. The PS
uses block textures 0–3 for base, normal, mask and secondary color; its
screen/cube textures are renderer-level bindings rather than block slots
4/5. Vertex attributes and other flag bits remain unclassified.

### DeformableWindow (`0x06B1F602`, reader `0x94A250`)

```c
#pragma pack(push, 1)
typedef struct DeformableWindowPrefix {
    uint32_t type_hash;
    uint8_t  version;       // 0, 1 or 2; descriptor reports 1
} DeformableWindowPrefix;
typedef struct DeformVertex48 {
    uint32_t words[4];
    uint16_t halves[4];
    uint32_t tail_words[6];
} DeformVertex48;
#pragma pack(pop)
/*
 * Version 0:
 *     Eight length-prefixed strings; uint32_t mode;
 *     uint32_t vertex_count; DeformVertex48 vertices[vertex_count];
 *     uint32_t index_count; uint16_t indices[index_count];
 *     CarDeformLookup lookup; outer end_marker.
 *
 * Version 1:
 *     Same as version 0; THEN uint32_t trailing_raw; outer end_marker.
 *
 * Version 2:
 *     uint32_t leading_raw; CarDeformLookup lookup;
 *     Eight length-prefixed strings; uint32_t mode;
 *     uint32_t vertex_count; DeformVertex48 vertices[vertex_count];
 *     uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x943A10` reads the 48-byte records; the embedded `0x9669E0` reader
consumes the same fixed 256-word deformation table as CarPaint. It keeps
only each entry's low 16 bits as a runtime index, with `0xFFFFFFFF` as
an inactive sentinel. `0x96C720` gathers per-instance deformation values
from these indices for vertex-shader constants; this table is not a color
palette. The version-1 trailing/version-2 leading word is read into the
same runtime slot (`0x94A250`); bit 0 selects one of two PS shading-color
constants (`0x961F34`), but its artistic name and higher bits are unknown.
`0x95FD20` submits nonempty texture slots 0–5. The lookup's position
**moves before** the strings in version 2; there is no fixed 8- or
56-byte material record in this block.

### VegetationBark (`0xB1F9133D`, reader `0x966840`)

```c
#pragma pack(push, 1)
typedef struct VegetationBarkPrefix {
    uint32_t type_hash;
    uint8_t  version;     // exactly 0
    uint32_t flags;       // bit 2 selects the 32-byte vertex; otherwise 40
} VegetationBarkPrefix;
typedef struct BarkVertex32 { uint32_t words[8];  } BarkVertex32;
typedef struct BarkVertex40 { uint32_t words[10]; } BarkVertex40;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; then uint32_t mode;
 * uint32_t vertex_count;
 * (flags & 4) ? BarkVertex32[vertex_count] : BarkVertex40[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x967D40` submits nonempty texture slots 0–3. The bit-2 format decision is
at `0x966898`; counted vertex readers `0x9573D0` and `0x968270` read 32
and 40 bytes per record. At draw time `0x4` also limits bindings to
slots 0–1; without it the 40-byte path can bind slots 0–3 and use a
`_Dirt` shader permutation. Flag `0x2` selects the `_Water` shader family
(`0x94B840`, shader names built at `0x952530`). The other bits and vertex
words remain unclassified; these flag meanings are Bark-specific.

### VegetationFoliage (`0xD79884C6`, reader `0x95A8A0`)

```c
#pragma pack(push, 1)
typedef struct VegetationFoliagePrefix {
    uint32_t type_hash;
    uint8_t  version;          // exactly 0
    float    distance_fade[2]; // +0x05: VS distance-fade coefficients
    uint32_t unknown_words[3]; // +0x0D
    uint32_t flags;            // +0x19: shader and render-state selections
} VegetationFoliagePrefix;
typedef struct FoliageVertex40 { uint32_t words[10]; } FoliageVertex40;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode; uint32_t vertex_count;
 * FoliageVertex40 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

The reader obtains the 24-byte header at `0x95A8F0` and endian-swaps six
words at `0x958940`. `0x92EB30` submits nonempty texture slots 0–2;
`0x968370` uploads the first two words to VS `cb1[1].xy`. The `foliage`
VS at shader-bundle offset `0x2536C` computes
`1 - saturate(distance * distance_fade[1] - distance_fade[0])`.
Flag `0x40` selects the Simple shader/input layout; in the non-simple
path `0x8`, `0x4`, `0x1` and `0x2` select `_Water`, `_NoSnow`, `_NoSpec`
and `_NoTran` suffixes, respectively (`0x9672A0`, `0x94B360`). The
Simple path uses `0x8` and `0x4` for Water/NoSnow. Flag `0x20` changes
a renderer-state boolean, with no established artistic name. The remaining
three header words are not classified; constants initialized after the
read (`0x95E8C0`) are **runtime data**, not additional file fields.

### Halo (`0x65D9B5B2`, reader `0x946450`)

```c
#pragma pack(push, 1)
typedef struct HaloPrefix {
    uint32_t type_hash;
    uint8_t  version;     // exactly 0
} HaloPrefix;
typedef struct HaloVertex20 { uint32_t words[5]; } HaloVertex20;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; uint32_t mode; uint32_t vertex_count;
 * HaloVertex20 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 */
```

`0x951CD0` submits only nonempty texture slot 0. `0x958A60` reads 20 bytes
per vertex. The reader's initial destruction/reset of runtime buffers does
not consume bytes. There is no fixed serialized material record here;
the draw path instead supplies runtime constants.

### Merged (`0x9185A4C3`, reader `0x965170`)

```c
#pragma pack(push, 1)
typedef struct MergedPrefix {
    uint32_t type_hash;
    uint8_t  version;     // exactly 0
} MergedPrefix;
typedef struct MergedAfterTextures {
    uint32_t mode;
    uint8_t  format_flag; // zero versus nonzero
    uint32_t raw_word;    // meaning unknown
} MergedAfterTextures;
typedef struct MergedVertex24 {
    uint32_t words[4];
    uint16_t halves[4];
} MergedVertex24;
typedef struct MergedRecord52 {
    uint32_t words[12];
    uint16_t halves[2];
} MergedRecord52;
#pragma pack(pop)
/*
 * Eight length-prefixed strings; MergedAfterTextures fields;
 * if (format_flag != 0):
 *     uint32_t vertex_count; MergedVertex24 vertices[vertex_count];
 *     uint32_t index_count; uint16_t indices[index_count];
 * else:
 *     uint32_t record_count; MergedRecord52 records[record_count];
 *     // NO index_count or serialized indices in this branch
 * outer end_marker.
 */
```

`0x92EB30` submits nonempty texture slots 0–2. The flag and raw word are
read at `0x9651EF` and `0x9651F7`, respectively. The nonzero branch uses
vertex reader `0x951D80` and index reader `0x92D7F0`; the zero branch uses
record reader `0x937330`, then constructs runtime mesh resources **without**
further file reads. Draw paths `0x931450` and `0x9458A0` bind textures and
runtime constants rather than a fixed on-disk material. Both the flag's
semantic meaning and `raw_word`'s use remain unknown.

### Box (`0x416C4035`, reader `0x95FD90`)

```c
#pragma pack(push, 1)
typedef struct BoxPrefix {
    uint32_t type_hash;
    uint8_t  version;     // exactly 0
} BoxPrefix;
typedef struct BoxVertex16 { uint32_t words[4]; } BoxVertex16;
#pragma pack(pop)
/*
 * uint32_t vertex_count; BoxVertex16 vertices[vertex_count];
 * uint32_t index_count; uint16_t indices[index_count]; outer end_marker.
 * NO texture strings and NO mode word in this reader.
 */
```

The reader calls `0x950BF0` for counted 16-byte vertices and `0x92D7F0`
for indices. Its vtable has a texture-reading method, but `0x95FD90` never
calls it. There is no serialized material in this reader; attribute
semantics remain unknown.

### Registered types without an RBMDL payload reader

```c
/* The factory constructs these block objects, but their virtual +4 reader
 * is the same function, 0x543D90, which immediately returns 0 without
 * consuming any payload bytes. JC2_Model_ReadRBMDL then fails the model.
 * No accepted on-disk RBMDL struct exists for these six types in this build.
 */
#define RBMDL_3DText           UINT32_C(0x474ECF38)
#define RBMDL_DecalDeformable  UINT32_C(0xDF9E9916)
#define RBMDL_DecalSkinned     UINT32_C(0x8A42C0C1)
#define RBMDL_DecalSimple      UINT32_C(0x2BAFDD6C)
#define RBMDL_Beam             UINT32_C(0x1B5E8315)
#define RBMDL_Flag             UINT32_C(0xD885FCDB)
```

This is a limitation of this executable's **RBMDL read path**, not evidence
that these runtime block classes never exist or cannot be created by another
route. Reporting invented vertex/material structures for them would
misrepresent the file format.

## Sample walk-through

| Block | Type word / version | Material | Mode / vertex count | Vertex bytes | Index count / bytes | Marker / next position |
|---:|---|---:|---|---|---|---|
| 0 | `0x31` / `0x35` | `0x36..0x81` | `3 @ 0xDE`; `39 @ 0xE2` | `0xE6..0x529` (`39 * 28`) | `144 @ 0x52A`; `0x52E..0x64D` | `0x64E`; next `0x652` |
| 1 | `0x652` / `0x656` | `0x657..0x6A2` | `3 @ 0x713`; `622 @ 0x717` | `0x71B..0x4B22` (`622 * 28`) | `1692 @ 0x4B23`; `0x4B27..0x585E` | `0x585F`; EOF `0x5863` |

Block 0's four populated strings are `g_strips_dif.dds`,
`g_strips_nrm.dds`, `g_strips_mpm.dds`, `g_dirt_6.dds`. Block 1 uses
`g_channel_textures_c.dds`, `g_concrete_big_nrm.dds`,
`g_concrete_big_mpm.dds`, `g_dirt_3.dds`. In each block, the other four
strings are empty. The highest referenced vertex indices are 38 and 621,
respectively; index counts represent 48 and 564 groups of three. Both block
markers equal `0x89ABCDEF`, and the second marker ends exactly at EOF.

For an independent parser: bounds-check every length/count multiplication and
slice; dispatch by the hash before interpreting a payload; enforce the marker
at the computed payload end. Exact EOF and index-range checks are useful
validation for this sample, not claims that the original loader enforces them.

IDA local types created for follow-up work: `JC2_RBMDL_Header` (49 bytes,
valid for the five-byte signature), `JC2_RBMDL_BlockPrefix` (5 bytes),
`JC2_GeneralMaterialV3` (76 bytes), and `JC2_GeneralVertex28` (28 bytes).
