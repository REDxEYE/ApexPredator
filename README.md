# ApexPredator

Asset extraction and conversion utilities for Avalanche’s Apex/Generation Zero data. The main CLI (`ApexPredator`) mounts game archives, resolves hashed paths, and exports models/animations/textures to glTF or raw binary. Helper tools live in `src/tools` for type generation, hash collection, and quick hashing.

## Downloads
Latest CI: 
- Linux x64 https://nightly.link/REDxEYE/ApexPredator/workflows/build/cpp_conversion/ApexPredator-linux-x64.zip
- Windows x64 https://nightly.link/REDxEYE/ApexPredator/workflows/build/cpp_conversion/ApexPredator-windows-x64.zip

## Prerequisites
- CMake 3.20+ and a C17/C++17 toolchain (MSVC 2022 or recent clang/gcc). Ninja or Visual Studio generators both work.
- Git (required for `FetchContent` dependencies) and Internet access on first configure.
- Windows (tested) or WSL; Tracy headers are fetched automatically. Windows Debug builds enable profiling; Windows Release, RelWithDebInfo, and MinSizeRel builds compile it out so game DLLs can unload safely.
- Game data: the `archives_win64` directory from Generation Zero (or another Apex-based title) plus an asset path database (`hashes.db`). You can generate the DB with `HashCollector` if you have the string lists.

## Configure & build
Pick an out-of-source build directory; adjust the generator to taste.

```powershell
# Configure (example: Ninja)
cmake -S . -B cmake-build-relwithdebinfo -G "Ninja"

# Build
cmake --build cmake-build-relwithdebinfo --config RelWithDebInfo
```

Visual Studio generators need the `--config` switch on build; single-config generators (Ninja, Unix Makefiles) ignore it. The resulting binaries (e.g., `ApexPredator.exe`, `HashCollector.exe`) live in the chosen `cmake-build-*` directory.

## Runtime inputs
- `game_root` points at `.../archives_win64`.
- `hashes.db` maps 32-bit hashes to paths. Place it next to the binary or pass `-d <path>`.
- Output defaults to `./extracted`; override with `-o <dir>`.

## ApexPredator CLI
`ApexPredator <subcommand> [options]`

- `extract <game_root> <paths...> [-o out] [-d hashes.db] [-n] [-r]`  
  Export one or more assets by path (or hash if present in `hashes.db`). `-n/--no_textures` skips texture export; `-r/--raw` writes the original bytes instead of glTF.
- `extract-anims <game_root> <skeleton> <animations...> [-o out] [-d hashes.db]`  
  Convert Havok animation containers to glTF using the provided skeleton container.
- `search <query> [-d hashes.db]`  
  Query `hashes.db` for paths or hashes (SQLite wildcards like `%` are supported).

Examples:
```powershell
# Extract a model to glTF
ApexPredator extract D:\Games\GenerationZero\archives_win64 env/terrain/mountains/model-01.amf -o exported

# Dump raw bytes by hash or full path
ApexPredator extract D:\Games\GenerationZero\archives_win64 0xDEADBEEF -r -o dumps

# Export animations
ApexPredator extract-anims D:\Games\GenerationZero\archives_win64 characters/skeletons/player.bsk characters/anims/run.ban -o exported\anims

# Look up hashes
ApexPredator search "%env/terrain%" -d hashes.db
```

## Helper tools (in `src/tools`)
- `GenerationZeroHashCollector <game_root>` / `Rage2HashCollector <game_root>`: walk game archives and collect strings, including paths and extensions from GTOC/STOC indexes.
  * GenZ reads `../gz_strings/*.txt` and writes `../hashes.db`
  * Rage 2 reads `../rage_strings/filelist.txt` and writes `../rage2_hashes.db`, relative to the working directory.
  * See [asset_database.md](docs/asset_database.md).
- `AdfTypeGenerator <game_root>`: generates ADF type bindings. **Note:** output paths are hardcoded, adjust before running.
- `HavokTypeGenerator <game_root>`: generates Havok type bindings; paths are likewise hardcoded to the repository root—update them for your environment.
- `StringHasher`: read strings from stdin and prints their 32-bit hash.

## Tips
- Normalize input paths to forward slashes; hashes are computed on the normalized form.
- Keep `hashes.db` and `rage2_hashes.db` under version control’s ignore list; it is a generated helper database.
- See `LICENSE` for licensing details.

Validation:

```sh
cmake -S . -B cmake-build-debug -DREDSCORE_BUILD_MODEL_TESTS=ON
cmake --build cmake-build-debug --target ApexPredator RedsCoreModelTests
ctest --test-dir cmake-build-debug/_deps/RedsCore-build -R '^RedsCore.VirtualModel$' --output-on-failure
python tests/test_virtual_model_cli.py cmake-build-debug/ApexPredator
python tests/test_rage2_amf_cli.py cmake-build-debug/ApexPredator
```

## Loadable game modules

The CLI now loads game support from DLL/SO modules. Build `ApexPredator` and keep its `modules/` directory beside the executable; the build produces the Generation Zero and Rage 2 modules automatically. The host does not link the game readers or generated types. Modules are built with the app and share C++ interfaces and `ApexAppState`; only the loader entry symbol uses `extern "C"`.

```sh
ApexPredator modules /path/to/GenerationZero/archives_win64
ApexPredator extract /path/to/GenerationZero/archives_win64 asset.modelc --module generation-zero -d hashes.db -o exported
```

Omit `--module` to auto-detect the game, pass a module file path to select a specific library, or add `--module-dir` for another discovery directory. Each module exposes a read-only game-root probe. Generation Zero accepts installation/archive roots with its executable or Steam app ID marker and supported TAB 2.1 archives.

See [game_modules.md](docs/game_modules.md) for the ABI, root detection rules, tests and adding another game by duplicating its private readers/exporters.

Rage 2 is registered as `rage2` and recognizes its installation/archive root using `RAGE2.exe` (or Steam app ID `548570`) and TAB 3.1 archives:

```sh
ApexPredator modules "/mnt/games/SteamLibrary/steamapps/common/RAGE 2/" --module rage2
```

Rage 2 supports raw TAB 3.1 extraction (`extract ROOT ASSET -r -o OUTPUT`) by path or hash, including zlib and Oodle payloads. To export an AMF model or mesh to glTF, omit `-r`, select `--module rage2`, and supply `-d rage2_hashes.db` so lookup3 mesh/material references resolve to resource names:

```sh
ApexPredator extract "/path/to/RAGE 2" models/props/cable/horizontal_03.modelc --module rage2 -d rage2_hashes.db -o exported/models
ApexPredator extract "/path/to/RAGE 2" models/props/cable/horizontal_03.meshc --module rage2 -d rage2_hashes.db -o exported/meshes
```

## CI build artifacts

The GitHub Actions build workflow produces Linux x64 and Windows x64 artifacts containing `ApexPredator`, `modules/generation_zero`, `modules/rage2`, `hashes.db`, and `rage2_hashes.db`. It builds the `ApexPredator` target (which builds both modules) without running or building the optional test targets.
The Windows CI job uses Visual Studio 2022/MSVC; `winbuild.sh` uses Clang/MinGW and does not validate MSVC compatibility.

CI unpacks the committed `hashes.db.tar.xz` and `rage2_hashes.db.tar.xz` archives into each artifact. After changing either local database, regenerate and commit both archives:

```sh
cmake -S . -B build
cmake --build build --target CompressDatabases
git add hashes.db.tar.xz rage2_hashes.db.tar.xz
git commit -m "Update compressed game hash databases" -- hashes.db.tar.xz rage2_hashes.db.tar.xz
```

`CompressDatabases` uses SQLite's backup API, so committed changes in an active `-wal` file are included without changing the live databases. The archives contain only the database files, not the WAL or SHM files.
