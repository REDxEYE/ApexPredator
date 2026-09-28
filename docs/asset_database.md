# Asset database hashes

Both `kv` and `files` store `lookup3` (32-bit lookup3/hashlittle, seed zero) and
`murmur` (the first 64-bit output of MurmurHash3_x64_128, seed zero). Generation
Zero writes zero to `murmur`; Rage 2 computes both. `string_hashes(string_view)`
is the shared game-specific implementation used by the collectors and ADF string
storage.

```sql
CREATE TABLE kv (
    lookup3 INTEGER NOT NULL,
    murmur INTEGER NOT NULL DEFAULT 0,
    v TEXT NOT NULL,
    PRIMARY KEY (lookup3, murmur)
) WITHOUT ROWID;
CREATE TABLE files (
    lookup3 INTEGER NOT NULL,
    murmur INTEGER NOT NULL DEFAULT 0,
    name TEXT,
    size INTEGER NOT NULL,
    parent INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (lookup3, murmur)
) WITHOUT ROWID;
```

The primary keys index lookup3; partial indexes index nonzero Murmur values.
The files parent index is retained. Distinct hash pairs are retained even when
lookup3 collides. Lookup by a single colliding hash returns the first pair in
numeric SQLite order; deletion by a hash removes all matching pairs. Zero Murmur
means unused and is excluded from queries/deletes. SQLite stores 64-bit hashes
as signed INTEGER bit patterns; convert negative results to unsigned when using
external SQL tools. Parent remains the containing archive's game-specific hash.

`kv_put(StringHashes, value)` and `files_put(StringHashes, name, size, parent)`
require both hash fields. Read/delete APIs accept `AssetDB::HashType::Lookup3`,
`Murmur`, or the default `Game` (lookup3 for GenZ, Murmur for Rage 2). RTPC name
resolution explicitly uses lookup3 through `find_lookup3_name`.

Opening an old `kv(k,v)` / `files(hash,name,size,parent)` database migrates both
tables in one transaction. Existing game keys, names, sizes, and parents are
preserved; Rage 2 lookup3 hashes are computed from stored strings, while GenZ
Murmur values remain zero. For unnamed Rage 2
files, lookup3 is zero because no string is available. A failure rolls back the
migration. Use the matching game's build when opening a legacy database; its
old schema does not identify the hash algorithm. Existing user databases are
not rewritten by the build itself; migration runs when an application opens one.

Collectors use `../hashes.db` for GenZ and `../rage2_hashes.db` for Rage 2,
relative to their working directory. All collector string/file insertion paths
supply both hashes. GenZ archive enumeration now visits TAB 2.1 entries.

## GTOC/STOC ingestion

Both collectors recognize `GT0C` by content, including files whose outer archive
path is still unknown. `.gtoc` global indexes and `.stoc` per-archive sidecars use
the same reader, `GTOCFile` in `include/apex/gtoc.h`.

The little-endian layout is:

- Header: `GT0C`, followed by a 32-bit archive count.
- Archive: 32-bit path hash, validation tag, member count, then member references.
- Member: 32-bit metadata displacement and payload offset. The displacement is
  unsigned and relative to its own field. Payload offset zero means an external
  resource, not an absent member.
- Metadata: 32-bit path hash, extension hash, byte size, then a NUL-terminated
  path. Metadata starts are 4-byte aligned and may be shared across members.

The reader exposes archives, their member offsets/file indices, and unique
referenced metadata records. It owns the bytes backing name views, supports moves
but not copies, and rejects invalid counts, references, and unterminated names.
Views remain valid while their owning reader is alive.

Collection inserts each referenced path, including external resources, and its
dot-prefixed extension into `kv`. Extension strings are deduplicated per TOC.
Hashes are computed through `string_hashes`: lookup3 for GenZ, lookup3 and Murmur
for Rage 2. Stored TOC hashes are lookup3 in either game; they are not Murmur keys.
GTOC archive enumeration returns game-specific asset-path hashes (Murmur for Rage 2),
not the stored lookup3 hashes, so its keys can be passed to the archive manager.

TOC member paths also create or update `files` rows with their game-specific hash
pair and recorded byte size, including external resources. Existing parent
values are preserved; new rows use parent `0` because an archive's lookup3 key
alone cannot recover a Rage 2 Murmur parent key. The TOC's own hash is not used as
the payload parent. Extension-only strings remain in `kv`, not `files`.
The TOC file itself retains normal archive-file registration when its path is
known. This adds TOC metadata collection, not mounting or extracting RAGE 2's
externally indexed small-archive payloads.

Tests cover both game configurations, legacy migration/reopening, hash
collisions, unsigned 64-bit keys/parents, zero Murmur handling, malformed TOC
boundaries, shared metadata, and running both collector executables against
synthetic RTPC/GTOC/STOC archives:

```sh
cmake --build cmake-build-debug --target GenerationZeroHashCollector Rage2HashCollector ApexGtocTests
ctest --test-dir cmake-build-debug -R '^Apex\.(Gtoc|HashCollectors)$' --output-on-failure
```

The real `sarc.0.gtoc` sample was also ingested through isolated TAB fixtures by
both collectors: all 69,251 member paths were registered in both `kv` and `files`
with matching hashes and sizes, and 54 extensions were collected in `kv`,
without modifying the project's databases.
