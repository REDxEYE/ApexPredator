# Game modules

ApexPredator is a CLI host. It links the module loader and CLI11, and loads game support at runtime. `GenerationZeroModule` owns the current generated ADF/Havok types, ADF/Havok/RTPC/AMF/texture/audio exporters, archive readers, hash database and export state. `GenerationZeroAdfLib` and `GenerationZeroHavokLib` are private implementation libraries, also used by the existing development tools.

For the next game, duplicate or adapt the game-specific sources in a new module. No common engine-reader/exporter abstraction is required at this stage. RedsCore's virtual model serializer remains reusable; each module statically links its own RedsCore implementation. Models are currently serialized inside the module, so C++ scene types do not cross the DLL/SO boundary.

## Loading and selection

Building `ApexPredator` also builds `modules/generation_zero`, `modules/rage2`, and `modules/just_cause_2`, and `modules/second_extinction` on Linux or Windows. Keep the `modules` directory beside the executable. Discovery uses the executable's location, independent of the working directory. Multi-configuration generators place the module beneath the corresponding configuration's executable directory.

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

The loader wrapper owns the shared state and session, retains the library, and destroys
both before unloading it. Callers using the interface directly must preserve that order,
use the state supplied to `open`, and avoid concurrent mutation of the same state.
Generation Zero initializes its archive manager in the module; Rage 2 initializes its
TAB 3.1 archive collection there. Constructing `ApexAppState` alone mounts no game data.
Standalone Generation Zero tools explicitly call `mount_archives()`.

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

With both shipped modules installed, use `--module generation-zero` or `--module rage2` for commands that lack a game root. In particular, existing Generation Zero database searches now need `--module generation-zero`.

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

1. Add a separate `MODULE` target and private include/source directories. Copy/adapt its readers, generated types and exporters as needed.
2. Implement the C++ interfaces with a unique ID and a game-specific root probe. Keep registries, globals and sessions private to that library.
3. Define `APEX_BUILD_GAME_MODULE` for the entry-point export on Windows. Export only `apex_game_module_v2`; use the existing ELF version script or macOS export list as a template. This prevents identically named generated types/globals from colliding across games.
4. Build static dependencies with position-independent code and the same runtime as the host. Allocations use the standard runtime; Windows Debug builds include Tracy, while release configurations disable it to avoid joining profiler threads during DLL unload.
5. Place the result in `modules/` or pass its path/directory. No host code changes are needed for the existing operations.

Existing type-generation/hash utilities still link the Generation Zero implementation directly. Migrating those developer tools to an extended module API is separate from runtime module loading.

## Validation

```sh
cmake -S . -B build -DAPEX_BUILD_MODULE_TESTS=ON -DREDSCORE_BUILD_MODEL_TESTS=ON
cmake --build build --target ApexPredator ApexModuleFixtures RedsCoreModelTests
ctest --test-dir build -R '^(Apex.GameModules|Apex.Rage2Module|Apex.TabV31|Apex.VirtualModelCLI|RedsCore.VirtualModel)$' --output-on-failure
```

The fixture modules live in `test-modules/`, outside normal discovery. Tests cover selecting either module, explicit paths/IDs, working-directory independence, ambiguous roots, unsupported operations, incompatible ABIs, duplicate IDs, Generation Zero identification/version checks, two independent live sessions, and ADF-to-virtual-model-to-glTF output. They do not require installed game data.
