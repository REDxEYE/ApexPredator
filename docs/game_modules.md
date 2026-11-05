# Game modules

ApexPredator is a CLI host. It links the module loader and CLI11, then loads game support at runtime. Each module owns its generated ADF/Havok bindings, format adapters, archive mount and hash policy, database, and export state. Generated libraries are implementation dependencies of their owning module and development tools.

For another game, link the game-neutral raw/glTF operations and common file-export fallback, then select an existing format-family dispatcher or provide `modules/<game>/export/format_dispatch.cpp`. Link one archive-mount implementation and one hash-policy implementation; an existing shared family is appropriate only when its input layout and output behavior match. RedsCore's virtual-model serializer remains reusable; models are serialized inside the module, so C++ scene types do not cross the DLL/SO boundary. The ownership plan and its verification are in [game_module_refactor_plan.md](game_module_refactor_plan.md).

## Loading and selection

Building `ApexPredator` also builds `modules/generation_zero`, `modules/rage2`, `modules/just_cause_2`, and `modules/mad_max`, and `modules/second_extinction` on Linux or Windows. Keep the `modules` directory beside the executable. Discovery uses the executable's location, independent of the working directory. Multi-configuration generators place the module beneath the corresponding configuration's executable directory.

```sh
ApexPredator modules
ApexPredator modules /path/to/GenerationZero/archives_win64
ApexPredator extract /path/to/GenerationZero/archives_win64 asset.modelc -d hashes.db -o exported
ApexPredator extract /path/to/GenerationZero asset.modelc --module generation-zero -d hashes.db
ApexPredator extract /path/to/game asset --module /path/to/new_game.so -d hashes.db
ApexPredator extract /path/to/game asset --module-dir /path/to/additional/modules -d hashes.db
```

`--module-dir` augments default discovery. Auto-selection calls every compatible module's probe and chooses the highest positive confidence. Equal best scores require an explicit `--module`. Explicit selection still checks root support. Duplicate module IDs are rejected. Libraries with missing/incompatible entry points are reported as diagnostics and excluded from discovery.

`search` does not have a game root: with one installed module it selects that module; with several, specify `--module`. `extract` and `extract-anims` retain their existing asset and output options. The previously empty `extract-all` and `convert` implementations now report unsupported operations rather than returning success without doing work.

## C++ module interface

Host and modules are built together with the same compiler, standard library, runtime,
and compatible build settings. `include/modules/game_module_api.h` defines the
`Modules::GameModule` and `Modules::GameSession` virtual interfaces. Only the entry
symbol uses C linkage:

```cpp
extern "C" const Modules::GameModule *apex_game_module_v2(uint32_t requested_abi);
```

Return `nullptr` for unsupported interface versions. Version 2 intentionally rejects
old C-table modules. The entry returns a borrowed module singleton, valid until unload.
The version checks interface revisions, not arbitrary C++ toolchain compatibility.

- `probe(path)` returns a `ProbeResult` containing confidence and a `std::string` reason.
- `open(ApexAppState&)` validates the root and initializes module-specific resources,
  returning `std::unique_ptr<GameSession>`.
- `extract(ApexAppState&, asset)` and `animation(ApexAppState&, skeleton, asset)`
  receive the shared application state. `extract_raw`, `skip_textures`, `root_motion`,
  database/output paths, and the virtual-model scene live in that state; there are no
  integer flag masks or opaque session handles.
- `search(database, query)` returns `std::vector<std::string>` directly.
- Operations throw standard C++ exceptions on failure. Default animation/search
  methods report unsupported capabilities. Asset names are UTF-8 `std::string_view`;
  filesystem paths use `std::filesystem::path`.

The loader owns the shared state and session, retains the library, and destroys
both before unloading it. Direct callers must preserve that order, use the state
supplied to `open`, and avoid concurrent mutation of the same state.
Constructing `ApexAppState` alone mounts no data: each module calls its selected
`mount_game_archives(ApexAppState&)` implementation, as do standalone tools.
`ApexAppState` owns the resulting archive manager.

MSVC builds use the shared CRT (`/MD`, `/MDd`); `winbuild.sh` uses shared MinGW C++/
exception runtimes and copies their DLLs beside the executable. Do not mix runtime or
standard-library debug configurations across modules.

The Generation Zero adapter serializes legacy database-dependent operations and activates the correct session database for each call. Hash helpers no longer cache the first database pointer, so sequential or alternating sessions remain independent.

## Generation Zero detection

The module accepts the installation root or `archives_win64`. It checks:

1. An `initial` archive directory exists.
2. `GenerationZero_F.exe`, or a `steam_appid.txt` containing `704270`, identifies the installation beside the archive root (the marker can also be inside the archive root).
3. Available TAB files use the supported 2.1 header/entry layout and have matching ARC files.

`optional` and `supplemental` are mounted when present. A copied archive-only dataset needs the Steam app ID marker to identify its game; a generic Apex TAB folder is deliberately insufficient. This detects the supported installation/layout, not every possible future per-asset schema revision.

## Rage 2 registration

`Rage2Module` registers ID `rage2` and display name `Rage 2`. It accepts the installation root or `archives_win64`, recognizes `RAGE2.exe` or Steam app ID `548570`, and checks TAB 3.1 headers with matching ARC files. Language archives nested beneath `initial`/`supplemental` are included in detection. The layout was checked against the installed game.

```sh
ApexPredator modules "/mnt/games/SteamLibrary/steamapps/common/RAGE 2/"
ApexPredator modules "/mnt/games/SteamLibrary/steamapps/common/RAGE 2/archives_win64" --module rage2
```

The module supports raw extraction by path (MurmurHash3 x64/128, seed zero, first 64 bits) or a `0x`-prefixed 64-bit hash. `-r` removes archive compression and writes the asset payload; it does not convert asset formats. Nested language archives are mounted, and supplemental content takes precedence over initial content. No hash database is required. Unknown hashes are written as `0xHASH.bin`.

```sh
ApexPredator extract "/mnt/games/SteamLibrary/steamapps/common/RAGE 2/" text/master_eng.stringlookup -r -o extracted
```

TAB 3.1 supports stored data, zlib, and Oodle streams through the vendored GPL decoder in `external/ooz`. Rage 2 AMF model conversion maps `GeneralR2` and `Character` slots 0/1/2 to glTF base color, normal, and metallic-roughness textures. `Character` slots 8 and 9 are shader-specific blood/gib maps: converted images are saved under the export root at their asset paths with `.png` (or `.dds` for float textures) appended, and the model node's `extras.rage2CharacterTextures[materialName]["8"|"9"]` records those export-root-relative paths. They are not blended into the default glTF material. Generation Zero generated bindings are not linked into Rage 2. See [tab_v31.md](tab_v31.md) for the verified archive layout.

With multiple modules installed, commands without a game root require `--module`. For example, Generation Zero database searches need `--module generation-zero`.

## Mad Max archives, ADF and skeletons

`MadMaxModule` registers ID `mad-max` for the native Linux and Windows builds (Steam app ID `234140`). It accepts the installation root or its `archives_win64` folder. On Linux the archives live under `share/data/{archives_win64,patch_win64,feral_overrides,dlc_win64/*}` and the identity marker is `bin/MadMax` or `bin/steam_appid.txt`. On Windows these archive folders are directly under the game root (without `feral_overrides`); `MadMax.exe` or a root-level `steam_appid.txt` identifies the game. The Linux `share/data` path is also accepted as a root.

The shared reader in `src/apex/package/tab_v1.cpp` handles the legacy compressed TAB: a little-endian 8-byte header (`0x800` alignment and metadata-group count), variable-sized hash-keyed cumulative block-offset groups, and 16-byte entries containing basename Lookup3 hash, absolute ARC offset, packed size, and decoded size. A stored segment has equal sizes; other segments are independent raw-DEFLATE streams. `TabArchive` validates both this layout and Just Cause 2's 4-byte-header/12-byte-entry layout before selecting a reader; ambiguous or invalid 2048-aligned archives are rejected. Top-level TAB lookup uses the basename without case folding, or a `0x`-prefixed 32-bit hash. Nested SARC v2 members use the full NUL-terminated path (excluding its alignment padding). Raw extraction writes the decoded bytes as `XXXXXXXX.bin` when no name is known.

`modules/mad_max/archive.cpp` has a standalone reader with numeric `gameN` and Feral/patch/base priority. The active module uses `modules/mad_max/archive_mount.cpp` and the shared archive manager instead; because its mounts are stored in an unordered map, collision precedence is not guaranteed. The probe checks game identity and base TAB/ARC headers; opening validates table framing and ARC bounds.

```sh
ApexPredator modules "/path/to/Mad Max" --module mad-max
ApexPredator extract "/path/to/Mad Max" 0xB444264E --module mad-max --raw -d mad_max_hashes.db -o dumps
```

The Linux executable string corpus is `strings/mad_max/mad_max_exe_strings.txt` (37,198 sorted ASCII/UTF-16LE candidates). Regenerate it from the installed game with:

```sh
python3 scripts/extract_exe_strings.py "/path/to/Mad Max/bin/MadMax" strings/mad_max/mad_max_exe_strings.txt
```

`MadMaxHashCollector` links the shared scanner with the Mad Max collector, archive-mount, and hash policies. Run it from a build directory directly under the repository so it reads `../strings/mad_max/mad_max_exe_strings.txt` and writes `../mad_max_hashes.db`:

```sh
cmake --build cmake-build-debug --target MadMaxHashCollector
(cd cmake-build-debug && ./MadMaxHashCollector "/path/to/Mad Max")
```

The collector mounts the Linux `share/data` archives or the Windows root archive folders and scans their entries, including bare SARC inner archives through the shared collector path. The extracted executable strings are hash candidates, not verified filenames. Pass an existing database with `-d` even when extracting by numeric hash; the module opens it when creating a session.

The native Linux `bin/MadMax` also contains 54 embedded full ADF v4 blobs. Their exact bytes and lengths are in `include/apex/adf/mad_max_builtin_adf.hpp` as `adf_<virtual address>` arrays and a `builtin_adfs` table. `MadMaxAdfTypeGenerator` combines these with types found in mounted archives and writes Mad Max bindings under `modules/mad_max/{include,src}/apex/adf/generated`. `MadMaxAdfLib` compiles those files with the shared ADF instance reader, and the module initializes the Mad Max type registry once on open. Non-raw extraction decodes full or small ADF instances to JSON; unknown types fail explicitly. Other formats besides supported skeleton packfiles require `--raw`; Second Extinction's AMF/terrain exporters cannot consume Mad Max's generated schema.

To regenerate the game-specific ADF bindings before rebuilding the module:

```sh
cmake --build cmake-build-debug --target MadMaxAdfTypeGenerator
(cd cmake-build-debug && ./MadMaxAdfTypeGenerator "/path/to/Mad Max" ../mad_max_hashes.db)
cmake --build cmake-build-debug --target MadMaxModule
```

`MadMaxHavokTypeGenerator` uses the shared Second Extinction `TAG0` reader/code generator, with output under `modules/mad_max/{include,src}/havok/tag_generated`. This does not overwrite the binary-packfile bindings in `havok/generated`. Run it from a repository build directory with the game root and an existing database path:

```sh
(cd cmake-build-debug && ./MadMaxHavokTypeGenerator "/path/to/Mad Max" ../mad_max_hashes.db)
```

The installed native Linux archives yield no `TAG0` Havok types through this scanner. In that case the generator exits with an error and leaves generated bindings untouched; Second Extinction's generated type declarations are not Mad Max definitions. The archive skeletons are instead Havok **2013.2 binary packfiles** with magic `57 E0 E0 57 10 C0 C0 10`, a class-name section, and an empty `__types__` section.

`modules/mad_max/{include,src}/havok/generated/havok_types.*` provides the registered `hkRootLevelContainer`, `hkaAnimationContainer`, and `hkaSkeleton` decoders for 64-bit little-endian `hk_2013.2.0-r1` packfiles. `MadMaxHavokLib` builds these bindings, and the module initializes their type registry once on open. The Mad Max format dispatcher recognizes the binary packfile magic and calls `src/exporter/mad_max/havok_export.cpp`, which writes skeleton JSON and populates the virtual-model scene for glTF skin export. The bindings preserve the skeleton name, parent indices, bone names/translation locks, reference poses, reference floats/slots, and partitions. Unsupported nonempty local frames or packfiles without a linked skeleton fail explicitly; other Havok objects and animations remain unsupported.

RTPC `.epe` entities can reference a virtual `.skeleton` path; the physical skeleton is the corresponding `.bsk` member of a nested SARC. For example, `a00_vehicle_empty_gastown_light.epe` names `animations/skeletons/vehicles/gastown001light/gastown001light.skeleton`, and the matching `.bsk` member has hash `0xB3D63E83`. With `mad_max_hashes.db`, export its skeleton as `.json` and skin-only `.gltf` alongside the asset path:

```sh
ApexPredator extract "/path/to/Mad Max" 0xB3D63E83 --module mad-max -d mad_max_hashes.db -o exported
```


## Second Extinction preparation

`SecondExtinctionModule` uses ID `second-extinction` and the Rage 2 engine's TAB 3.1 layout. Its root probe identifies `SecondExtinction_F.exe` or Steam app ID `1024380`; the installed archives include Zstd codec 3 entries. The 1,314 built-in ADF v4 records extracted from the executable are in `modules/second_extinction/include/apex/adf/second_extinction_builtin_adf.hpp`. Build `SecondExtinctionAdfTypeGenerator` and `SecondExtinctionHavokTypeGenerator` to produce game-specific bindings; the module and its own `second_extinction_hashes.db` collector become available after the generated ADF/Havok sources are supplied in `modules/second_extinction/`. Rage 2 generated bindings and hashes are not substitutes.

Second Extinction DDSC textures retain `AVTX` version 1 but use eight 20-byte
stream records (192-byte header), rather than the eight 12-byte records
(128-byte header) in Generation Zero and Rage 2. The shared AVTX loader uses
the first stream's payload offset (`0xC0` versus `0x80`) to select the layout;
version and tag do not distinguish Rage 2 from Second Extinction.

ADF files also occur with a small eight-byte header: `\0FDA` followed by a
little-endian root type hash. Unlike full ` FDA` files, these have no embedded
type or name tables. The root instance starts at `max(8, type alignment)` and
occupies the rest of the file; extraction uses the selected game's generated
ADF type registry to resolve the hash and alignment. A root type absent from
that registry cannot be decoded.

## Adding another game

1. Add a separate `MODULE` target and private include/source directories. Link `src/exporter/raw_export.cpp` and, for glTF-producing games, `src/exporter/gltf_export.cpp` with the game-neutral `src/exporter/common_export.cpp`. Supply one `dispatch_file` implementation by linking an applicable family dispatcher or writing a game-specific one; add only supported format exporters and their generated-schema adapters.
2. Implement the C++ interfaces with a unique ID and game-specific root probe. In `open`, call the module-linked `mount_game_archives(ApexAppState&)`; keep registries, globals, and sessions private to that library.
3. Select an archive-mount source and exactly one hash-policy source for the module and its tools. `asset_path_hash` and `legacy_hash_domain` are supplied by that policy, not a process-global mutable game selector. Wire collector and type-generator configuration sources only into their owning tool targets; generation paths and built-in ADF blobs must remain game-owned.
4. Define `APEX_BUILD_GAME_MODULE` for the entry-point export on Windows. Export only `apex_game_module_v2`; use the existing ELF version script or macOS export list as a template. This prevents identically named generated types/globals from colliding across games.
5. Build static dependencies with position-independent code and the same runtime as the host. Allocations use the standard runtime; Windows Debug builds include Tracy, while release configurations disable it to avoid joining profiler threads during DLL unload.
6. Register the module target once in `CMakeLists.txt`; place its library in `modules/` or pass its path/directory. No host code changes are needed for existing operations.

Development tools link the selected game's archive/hash/collector/generator sources directly; they do not extend the runtime module ABI.

## Validation

```sh
cmake -S . -B build -DAPEX_BUILD_MODULE_TESTS=ON -DREDSCORE_BUILD_MODEL_TESTS=ON
cmake --build build --target ApexPredator ApexModuleFixtures RedsCoreModelTests
ctest --test-dir build -R '^(Apex.GameModules|Apex.Rage2Module|Apex.TabV31|Apex.VirtualModelCLI|Apex.RtpcJsonOutput|Apex.JustCause2Module|Apex.MadMaxSkeletonModule|RedsCore.VirtualModel)$' --output-on-failure
```

The fixture modules live in `test-modules/`, outside normal discovery. Tests cover selecting either module, explicit paths/IDs, working-directory independence, ambiguous roots, unsupported operations, incompatible ABIs, duplicate IDs, Generation Zero identification/version checks, two independent live sessions, and ADF-to-virtual-model-to-glTF output. They do not require installed game data.

The RTPC fixture checks nested JSON paths and output errors. The Just Cause 2 mixed fixture supplies an isolated database and distinct base/patch/DLC assets; archive collision priority is not specified by this test. The Mad Max skeleton fixture checks packfile virtual-class resolution through JSON and glTF export.
