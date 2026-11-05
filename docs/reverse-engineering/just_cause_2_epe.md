# Just Cause 2 entity `.ee.epe` / PCBB format

This note describes raw `.ee.epe` assets extracted from the installed Just Cause 2
archives and the matching paths in `JustCause2.exe`. Addresses are image virtual
addresses in that executable. Numeric fields examined here are little-endian.
The layout was checked against six assets, not every EPE variant. The container
name **PCBB** is the four bytes on disk; its expansion is not established.

## Resource relationship

An entity can refer to a model and a skeleton independently:

```text
cutscene_bird_skinned.ee.epe
├── resource_file: models\animals\cutscene_bird\cutscene_bird-base1.lod
│   └── Animals\Cutscene_bird\cutscene_bird_lod1-base1.rbm
└── resource_file: animations\skeletons\cutscene_bird_reference.skeleton
```

The `.lod` is a text manifest of `.rbm` paths, not a skeleton manifest. The
referenced `cutscene_bird_lod1-base1.rbm` begins with RBMDL render-block type
`SkinnedGeneral` (`0x5E657F20`, version 3): **skinned geometry still uses
`.rbm`**. See [the RBMDL note](just_cause_2_rbmdl.md) for the mesh container.
The archive also contains `cutscene_bird_reference.bsk`, a Havok packfile with
`hkaSkeleton` metadata and header version `Havok-5.5.0-r1`. No `.skeleton` file
appears in the indexed JC2 archive names. Resolving the `.skeleton` resource
reference to that matching `.bsk` filename is **inferred, not traced through
the engine's resolver**.

That `.bsk` is a separate format, not a PCBB section: its first eight bytes
are Havok packfile magic `57 E0 E0 57 10 C0 C0 10`, and it has
`__classnames__`, nonempty `__types__`, and `__data__` sections. This is
Havok 5.5 reflection metadata, not the SDK 2016/2017 tagfile layout.

The entity file is not the only possible starting point: executable function
`CNavigationArrow::sub_7F8E60` directly requests three
`go170-arrow_01/02/03.lod` resources. The observed entity-to-model chain
must not be treated as universal.

## Executable loading path

| Address | Observed behavior |
|---|---|
| `0x8ACEE0` | Classifies the final `.epe` suffix as resource type 12, regardless of preceding `.ee`. |
| `0x8DA340` | `CEntityResourceLoader` type-12 branch tests four individual bytes of its input against `P`, `C`, `B`, `B` at `0x8DA3EA–0x8DA3FF`. This explains why a contiguous `PCBB` string need not appear in IDA. |
| `0x8696B0` | Copies a matching PCBB buffer to the holder's `+0x7C`, records size at `+0x80`, and marks it ready. **This step stages bytes; it does not parse records.** |
| `0x740D10` → `0x7F4A10` → `0x8C6DC0` → `0x85AED0` | When the resource is ready, obtains the holder and walks its staged PCBB chunks and property nodes. `0x85AED0` computes lookup3 hashes for `_class` and `name`, resolves relative node offsets, and advances by each chunk's length plus eight header bytes. |
| `0x7F4A10` | Looks up the serialized class and instance names, creates a game object through the registered factory, and dispatches its property-reading virtual method. `CResourceObject::sub_7433F0` occupies that virtual slot (`+0x18`); it reads a resource path and recognizes `.lod` and `.skeleton` as resource types 4 and 8. |

`CEntityResourceLoader::sub_8DA340` sends **non-PCBB** type-12 input through
`TPropertyFile::possible_ctor_or_dtor_2`; that fallback must not be called the
PCBB decoder. The traced `0x8A33E0` entry also unwraps a SARC-framed resource,
whereas the raw extracted files here start directly with PCBB. The exact
relationship between that outer runtime envelope and these extracted bytes has
not been independently established.

## PCBB chunk framing

An EPE file can contain multiple consecutive chunks; there is no extra file
header in the inspected assets. For a chunk beginning at file offset `c`:

| Offset | Field |
|---|---|
| `c+0x00` | Four literal bytes `PCBB`. |
| `c+0x04` | `u32 L`, size of the body following the eight-byte chunk header. |
| `c+0x08` | Body of `L` bytes, starting with a root property record. |
| `c+0x08+L` | Next chunk or EOF. |

The body includes trailing `0xDD` filler. Chunk bodies and property records in
these fixtures are 16-byte aligned; `0xDD` is padding, not another magic. For
example, `panay_picture_skinned.ee.epe` has chunks at file offsets `0x000`
(`L=0x170`) and `0x178` (`L=0x180`), ending at `0x300`. The cutscene-bird
entity has chunks at `0x000` and `0x188`, each with `L=0x180`, ending at
`0x310`. `f2m05.barrels.ee.epe` has twelve chunks, alternating larger
`0x2B50` and smaller `0x370` bodies.

## Property record graph

Let `B=c+8` be the start of a chunk's body and `p` the record's offset from
`B`. Every offset in this table is relative to `B`, **not** to the file:

| Record offset | Observed field |
|---|---|
| `+0x00` | `u32` property ID: lookup3 `hashlittle(property_name, seed=0)`. |
| `+0x04` | `u32` form: `1` = group with children; `2` = leaf with a value. |
| `+0x08` | `u32` consistently equal to `p+0x10`; purpose beyond this relationship unverified. |
| `+0x0C` | `u32` next sibling's body-relative offset, or `0xFFFFFFFF` for the last sibling. |
| `+0x10`, form 1 | `u32` first child's body-relative offset. Following 12 bytes are `0xDD` filler. |
| `+0x10`, form 2 | `u32` value kind, listed below. |
| `+0x14`, form 2 | Inline value, or body-relative payload offset for pointed kinds. Bytes `+0x18..+0x1F` are `0xDD` filler. |
| `+0x20`, pointed leaves | Payload. Its offset is also the value at record `+0x14`. |

The initial record is at `B+0`. For a group, follow its child offset before
following the group's next-sibling offset. Within a sibling chain, the last
record has `0xFFFFFFFF` at `+0x0C`. The inspected sibling chains have strictly
ascending, distinct property IDs. Form-1 groups are structural nesting, **not
additional embedded PCBB chunks**.

For the repository's 010 Editor template `pcbb.bt`, carry `B` into every
recursive `Prop`. Inferring it from `parentof` instead changes the base to the
enclosing group for child fields. After a chunk's records, resume at `B+L`,
not at the cursor left by the final property seek.

| Leaf value kind | Observed representation |
|---|---|
| `1` | Inline 32-bit value at `+0x14`; the individual property's meaning varies. |
| `2` | Inline IEEE-754 `float32` at `+0x14`. |
| `3` | Body-relative pointer at `+0x14` to a NUL-terminated byte string at the payload position (possibly empty). Remaining bytes are `0xDD`-padded. |
| `5` | Body-relative pointer to 12 bytes observed as three `float32` values, then four `0xDD` bytes. |
| `6` | Body-relative pointer to 16 bytes observed as four `float32` values. |
| `8` | Body-relative pointer to 64 bytes observed as sixteen `float32` values; matrix-like, but coordinate convention unverified. |

Inline leaves occupy 32 bytes in the inspected layout; kind-5/6 leaves occupy
48 and kind-8 leaves 96. Group children and all pointer targets must remain
inside the enclosing chunk body. Do not interpret a padding byte or an inline
32-bit value as another node.

The C++ `PropertyValue` stores a group as `std::monostate` (it has children,
not a leaf value) and a kind-1 leaf as raw `uint32`, matching the unsigned
word in `pcbb.bt`; whether a particular field is signed is property-specific.
`PropertySection::properties` and `Property::children` are vectors populated
by following each sibling's `next_ptr`. Their `[0]` entries are the first
properties shown in 010 Editor, not arbitrary hash-table entries. For
`geo.f1t17dps.ee.epe`, section 1 starts with group `0x0471EF87`.

### Identified property IDs

These preimages were reproduced using this repository's `StringHasher`
(lookup3/`hashlittle`, seed zero):

| Property ID | Name | `PropertyKey` | Observed value |
|---|---|---|---|
| `0x1473B179` | `_class` | `Class` | Class string such as `CResourceObject` or `CSimpleRigidObject`. Also hashed by the engine at `0x85AED0`. |
| `0x2ACF24F6` | `resource_file` | `ResourceFile` | A `CResourceObject` resource path, e.g. a `.lod`, `.skeleton`, or `.cdoll`. |
| `0x32B409E0` | `filename` | `Filename` | `.lod` path on inspected `CSimpleRigidObject` records (`rock.ee.epe`, `km03.walkietalkiemesh.ee.epe`). |
| `0xD31AB684` | `name` | `Name` | Instance string, e.g. `CResourceObject1` or `CResourceObject2`. Also hashed by the engine at `0x85AED0`. |

The event-looking value `t{...}::event` appears at property `0xA4968489`,
but that hash's name and full behavior are not established. Inline field
`0xCFFF8405` differs between the two resource objects of the same skinned
entity; its meaning is also unknown. Do not label either based only on the
shape of one value.

### C++ lookup API

Both `PropertySection` (top-level properties) and `Property` (group children)
offer `find`, `has`, `get<T>`, `is<T>`, and `get_string` overloads accepting
a 32-bit property ID, a name, or `PropertyKey`. The enum values are compile-time
lookup3 hashes of the four verified names above; other names are hashed at
runtime with seed zero. Lookups scan the ordered vectors without building a
second hash index.

```cpp
const auto &section = file.sections.at(1);
const std::string &class_name = section.get<std::string>(Apex::PCBB::PropertyKey::Class);
const auto *group = section.find(0x0471EF87);
const std::size_t child_count = group ? group->children.size() : 0;
```

`find` returns `nullptr` and `has` returns `false` when absent. As in RTPC,
`get<T>` throws `std::runtime_error` for an absent property or a value-type
mismatch; `is<T>` and `get_string` throw `std::out_of_range` when the property
is absent. `get_string` returns the stored string, tries
`find_lookup3_name` for a `uint32` value, or returns `std::nullopt` for
another value type or an unknown lookup3 name.

### JSON representation

`PropertyFile::to_json()` returns a top-level array of section objects in file
order. `PropertySection::to_json()` maps each property name to its JSON value;
`Property::to_json()` returns a leaf value or an object of nested group
properties. The four identified keys above have stable names without an asset
database; other keys use `find_lookup3_name` and fall back to their unsigned
decimal hash string when unresolved. Repeated keys in one object retain the
last property.

Kind-1 values remain unsigned integers; floats and strings remain scalars.
`vec3` and `vec4` are arrays of three and four numbers, and `mat4` is a flat
array of 16 numbers in column-major order. An empty group is `{}`.

```cpp
const nlohmann::json document = file.to_json();
const auto &class_name = document.at(1).at("_class");
```

## Cross-fixture checks

The raw samples were obtained from the installed JC2 archives using
`ApexPredator extract <game_root> <asset> --module just-cause-2 -r` and
examined independently of the game's parser:

| Asset | File bytes | PCBB chunks | Notable content |
|---|---:|---:|---|
| `panay_picture_skinned.ee.epe` | 768 | 2 | Separate `.lod` and `.skeleton` resource objects. |
| `cutscene_bird_skinned.ee.epe` | 784 | 2 | Separate `.lod` and `.skeleton` resource objects. |
| `pd_roachesbase2.ee.epe` | 504 | 1 | `resource_file` points to a `.cdoll`. |
| `rock.ee.epe` | 1,720 | 1 | Rigid-object `filename` points to a `.lod`. |
| `km03.walkietalkiemesh.ee.epe` | 2,504 | 1 | Nested property group and a four-float kind-6 value. |
| `f2m05.barrels.ee.epe` | 71,904 | 12 | Repeated chunks with up to four nested group levels. |

A boundary-checked recursive walker consumed **all 19 chunks and 1,696
records** across these files: 73 group records and 1,623 leaves. All 92
sibling chains terminated, all referenced payloads stayed within their chunk,
and no unexplained bytes remained outside the documented chunk framing. This
checks the representation; it does not prove the meaning of every property or
all possible EPE value kinds.

## Still unverified

- Complete property-name dictionary, especially `0xA4968489` and
  `0xCFFF8405`, and semantics of kind-5/6/8 numerical fields.
- The meaning of the record `+0x08` field beyond its consistent offset
  relationship; behavior of malformed/other-version PCBB data.
- Exact `.skeleton` resource-name resolution to an archive `.bsk`; no claim
  that every engine version uses the same rule.
- Whether an outer SARC envelope is used for these particular raw EPE assets
  on the traced runtime path. The standalone byte layout above is verified
  from extracted assets.
