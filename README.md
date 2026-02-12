# ApexPredator

Asset extraction and conversion utilities for Avalanche’s Apex/Generation Zero data. The main CLI (`ApexPredator`) mounts game archives, resolves hashed paths, and exports models/animations/textures to glTF or raw binary. Helper tools live in `src/tools` for type generation, hash collection, and quick hashing.


## Prerequisites
- CMake 3.20+ and a C17/C++17 toolchain (MSVC 2022 or recent clang/gcc). Ninja or Visual Studio generators both work.
- Git (required for `FetchContent` dependencies) and Internet access on first configure.
- Windows (tested) or WSL; Tracy client headers are fetched automatically and can stay disabled at runtime.
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

# Dump raw bytes by hash (currently not supported)
ApexPredator extract D:\Games\GenerationZero\archives_win64 0xDEADBEEF -r -o dumps

# Export animations
ApexPredator extract-anims D:\Games\GenerationZero\archives_win64 characters/skeletons/player.bsk characters/anims/run.ban -o exported\anims

# Look up hashes
ApexPredator search "%env/terrain%" -d hashes.db
```

## Helper tools (in `src/tools`)
- `HashCollector <game_root>`: walks archives, ingests strings from `gz_strings/*.txt`, and populates `hashes.db` in the working directory.
- `AdfTypeGenerator <game_root>`: generates ADF type bindings. **Note:** output paths are hardcoded to `D:/projects/cpp/ApexPredator/include/...` and `src/...`; adjust before running.
- `HavokTypeGenerator <game_root>`: generates Havok type bindings; paths are likewise hardcoded to the repository root—update them for your environment.
- `StringHasher`: read strings from stdin and prints their 32-bit hash.

## Tips
- Normalize input paths to forward slashes; hashes are computed on the normalized form.
- Keep `hashes.db` under version control’s ignore list; it is a generated helper database.
- See `LICENSE` for licensing details.
