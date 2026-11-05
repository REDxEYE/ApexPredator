# Game-module exporter refactor plan

## Goal and invariant

Remove the closed `GAME` preprocessor list from runtime exporters, archive/hash policy, and development tools. Retain the current `GameModule`/`GameSession` ABI v2, CLI behavior, per-module generated ADF/Havok registries, and independent session databases. A future game should add its module and one explicit CMake registration without modifying existing game exporters or the host CLI.

Game-neutral code belongs under `src/exporter/` (and other existing shared-source directories); generated-schema and game behavior belong under `modules/<game>/export/` or the corresponding game-named source directory. Share format-family code only where input schema and output semantics match. Prefer direct per-module format dispatch over a new runtime registry. Do not put a generated game type in a game-neutral interface.

## Agent contract

Each item below is independently mergeable; finish all callers, target wiring, and relevant docs for the touched boundary. Before editing, inspect current symbols and callers because earlier items may have moved them. Build affected targets; run relevant existing fixture tests and a CLI/fixture scenario that exercises output; report commands/results, moved symbols, removed branches, and remaining `GAME` uses in the touched area. Preserve buffer ownership, output paths, errors, unsupported formats, and file-only exports that legitimately return no scene node. No compatibility aliases or half-migrated paths.

One integration owner merges shared-header/CMake edits. Independent format work may happen in separate branches after item 3, but merge and verify sequentially.

## Work packets

0. **Baseline (read-only):** Inventory `GAME` branches in `src`, `include`, and `src/tools`; record existing tests and fixture/CLI output for applicable raw, glTF, JSON, file-only, animation, and unsupported paths. Use a fresh build for module discovery: the previously observed debug artifact reported an unresolved `export_adf_file_from_buffer` symbol loading `mad_max.so`, which is not yet attributed to current source. Deliver a behavior matrix, not a refactor.

1. **Shared raw extraction:** Replace five copies of `raw_export` in `modules/*/export_operations.cpp` with one game-neutral implementation. Keep name fallback, path, bytes, and missing-asset behavior. Wire all five module targets. Done when raw fixtures and errors match baseline and duplicate bodies are gone. Do not change normal export.

2. **Shared glTF operation:** Extract scene reset, conversion, add-root, and save orchestration from Generation Zero, Rage 2, Second Extinction, and Just Cause 2. Each module supplies conversion. Keep Mad Max ADF-to-JSON policy local. Done when glTF paths/content and no-scene behavior match baseline. Depends on 1 for non-overlapping module-file edits.

3. **Module-selected format routing:** Replace `GAME` dispatch in `src/exporter/common_export.cpp` with exactly one dispatcher linked per module: use a shared format-family source where behavior is identical, or `modules/<game>/export/format_dispatch.cpp` for game-specific formats. JC2 handles RBMDL/PCBB; other modules offer only linked formats. Keep common lookup/unknown `.bin`, safe short-buffer and offset-4 magic probes, and move-only buffers. Distinguish an unrecognized payload from a handled file-only export. Update the dispatch interface and CMake; remove the old branch. Done when applicable formats and unknown-file fixtures match baseline. Depends on 2.

4. **ADF routing seam:** In `src/exporter/adf_export.cpp`, preserve reusable parsing, small-ADF alignment, and instance traversal; move generated-type actions to module-local adapters. Express unhandled versus handled-without-scene versus handled-with-scene, preserving instance cardinality, terrain, AMF header/buffer pairing, and fallback. No AMF decoder rewrite. Done when ADF/terrain/model/fallback fixtures match and shared routing contains no `GAME`. Depends on 3.

5. **ADF game exceptions:** Move Generation Zero XLSBook, Rage 2/Second Extinction variant/dialogue differences, and Mad Max ADF JSON policy to owning directories. A generated type's existence is not proof that a route is supported. Done when XLS is GZ-only and dialogue/unsupported outputs match baseline. Depends on 4.

6. **AMF geometry families:** Split `src/exporter/amf_export.cpp`'s separate vertex/index buffer LOD and merged-buffer LOD decoding into layout-owned sources. Keep common VM/attribute helpers only for identical contracts. Preserve offset units, high-res resolution, LOD fallback, and hash resolution. Done when AMF fixtures including missing high-res data match baseline. Depends on 4; may develop alongside 5 after interfaces settle.

7. **AMF materials:** Move render-block/material handling to game/family code. Keep Rage 2-only Character auxiliary texture export in Rage 2. Preserve `skip_textures`, channel mapping, extras. Done when texture fixtures match baseline. Depends on 6.

8. **RTPC behavior:** Keep common traversal, node/extras scaffolding, and JSON in `src/exporter/rtpc_export.cpp`; move string-path/`.bsk` versus hashed skeleton/model resolution and different character-node construction to game adapters. Link only where supported. Done when character, secondary attachment, and rigid-object fixtures match, with no exporter `GAME`. Depends on 3; independent of 4–7.

9. **Havok formats:** Separate tagfile/spline animation and Mad Max 2013.2 packfile skeleton paths in `src/exporter/havok_export.cpp` and `include/exporter/havok_export.h`. Keep generated transform/quaternion conversion format-local; share only genuinely equivalent VM assembly. Done when tagfile animation/root motion and Mad Max skeleton JSON/glTF match, and unsupported animation reports failure. Depends on 3; independent of 4–8.

10. **Archive mount policy:** Move `src/platform/app_state.cpp::mount_archives` game branches into module-owned session/tool setup while preserving `ApexAppState` ownership of its manager. Migrate standalone tool callers in the same item. Done when probe/open, archive precedence, and raw lookup fixtures match, with no app-state `GAME`. Independent of format packets after 2.

11. **Hash and DB policy:** Replace `GAME` selection in `src/utils/hash_helper.cpp` and legacy migration in `src/apex/asset_db.cpp` with explicit module-supplied policy. Preserve Lookup3/Murmur semantics, Mad Max basename rules, compatibility, and `ActiveDatabase` alternating-session isolation; avoid a global mutable policy. Done when asset DB/hash/session fixtures pass and shared files contain no game selection. Depends on 10.

12. **Hash collector:** Keep shared archive scanning in `src/tools/hash_collector.cpp`; move DB defaults, string corpora, initial GTOCs, and Second Extinction deferred-GTOC/location behavior to game-owned tool configuration/hooks. Wire collector targets; do not modify runtime ABI. Done when per-game collector fixtures and archive ordering match baseline. Depends on 10–11.

13. **Type generators:** Move built-in ADF selection and generated destinations from `src/tools/adf_type_gen.cpp` and `havok_type_gen.cpp` to game-owned target/configuration. Keep shared scanning/codegen. Do not confuse Mad Max packfile bindings with TAG0 generation. Done when each generator writes to only its existing game paths and generator tests pass. Depends on 10; independent of 12.

14. **Final build/documentation cutover:** Consolidate repeated source groups in `modules/*/module.cmake`, keep generated libraries private, remove remaining `PUBLIC GAME=...` and `include/games.hpp` after the last user, and retain explicit module registration in `CMakeLists.txt`. Update `docs/game_modules.md`'s future-game instructions. Done when five modules and tools build, discovery plus format smoke scenarios pass, and no production `GAME` preprocessor branch remains. Depends on all earlier packets.

## Merge discipline

After item 3, ADF (4–5), RTPC (8), and Havok (9) may be developed independently. AMF (6–7) follows the ADF interface. Archive/hash/tools (10–13) form another lane. Shared CMake or header conflicts are resolved by the integration owner, not by simultaneous edits. If a packet cannot preserve a game's existing behavior, leave that whole format on its current path and report the exact missing fixture or schema assumption rather than merging a lossy generic route.

## Progress

- [x] 0. Baseline — supported modules' fixtures recorded; Mad Max WIP build/load failures are an accepted exception.
- [x] 1. Shared raw extraction — one implementation wired into five modules; four supported modules built and exercised, Mad Max changed objects compiled (runtime verification exempt while WIP).
- [x] 2. Shared glTF operation — four modules link `src/exporter/gltf_export.cpp`; Mad Max retains its JSON/packfile output policy.
- [x] 3. Module-selected format routing — Apex family, JC2 and Mad Max dispatchers linked by their targets; `common_export.cpp` has no `GAME` branch.
- [x] 4. ADF routing seam
- [x] 5. ADF game exceptions
- [x] 6. AMF geometry families
- [x] 7. AMF materials
- [x] 8. RTPC behavior
- [x] 9. Havok formats
- [x] 10. Archive mount policy
- [x] 11. Hash and DB policy
- [x] 12. Hash collector
- [x] 13. Type generators
- [x] 14. Final build/documentation cutover

### Initial execution evidence

- Before the extraction, `test_tab_v31.py`, `test_tab_v1_dispatch.py`, and `test_virtual_model_cli.py` passed. The existing debug `mad_max.so` failed to load because `export_adf_file_from_buffer` was unresolved. A fresh `ApexPredator` build also stopped compiling `MadMaxModule`'s `adf_export.cpp` because the Mad Max generated schema lacks the referenced AMF and terrain types.
- After the extraction, Generation Zero, Rage 2, Second Extinction, and Just Cause 2 module targets built. TAB 3.1 and TAB v1 raw fixtures passed; direct CLI smoke scenarios verified Generation Zero's fallback filename/bytes and Second Extinction's database-resolved filename/bytes. The Mad Max `raw_export.cpp`, `module.cpp`, and `export_operations.cpp` object files compiled, but the full target remains blocked by the pre-existing ADF source mismatch.
- `test_just_cause_2_module.py` currently fails at session open with `unable to open database file` because that fixture invokes `extract` without a database path. `test_tab_v1_dispatch.py` supplies a fixture database and passes. Treat this separately from the raw-export move.
- Mad Max is work in progress; its known build/load failures do not block the remaining migration packets. Continue to update its source wiring where feasible, but do not conflate those failures with regressions in the supported modules.
- Packets 2–3: all five module targets built and `ApexPredator modules` discovered five libraries. Generation Zero ADF/glTF, Rage 2 AMF, JC2 RBMDL, and direct JC2 PCBB JSON/glTF CLI scenarios passed. A short unknown asset fell back to `.bin`, while an MKV payload wrote `.mkv` without a glTF scene. Mad Max no longer links incompatible shared ADF/AMF exporters; its own normal ADF-to-JSON operation remains module-local and is not claimed runtime-verified.
- The existing JC2 mixed fixture also assumes extraction from `archives_win32`, but `ApexAppState::mount_archives()` appends that folder again for JC2; its expected unnamed glTF path differs from the current numeric-hash fallback. Leave this fixture unchanged and address root normalization with packet 10 rather than altering export semantics during packet 3.

### Final cutover evidence

- `ApexPredator`, all five module targets, five collectors, and eight type generators built. `ApexPredator modules` discovered all five modules. Shared source groups are in `cmake/apex_common_sources.cmake`; no production `GAME` preprocessor branch or target definition remains, and `include/games.hpp` is removed.
- The focused 19-test CTest run passed, covering asset DB migration, GTOC and archive enumeration, generators, module sessions, raw/ADF/AMF/glTF fixtures, hash collectors, and Just Cause 2 paths. Direct Mad Max ADF JSON and the supported games' format CLI fixtures passed. Throwaway generator smoke confirmed four ADF generators write only their game-owned paths; empty Mad Max TAG0 input leaves packfile bindings untouched.
- RTPC CLI smoke exercised both `.skeleton` to `.bsk` lookup for Generation Zero and hash-based lookup for Rage 2: each wrote its RTPC JSON before reporting the expected missing skeleton. These fixtures intentionally have no skeleton archive.
- Follow-up: `Apex.RtpcJsonOutput`, `Apex.JustCause2Module`, and `Apex.MadMaxSkeletonModule` pass. RTPC creates nested JSON directories and reports open/write failures. The JC2 mixed fixture now supplies a database, uses current unnamed output paths, aligns its PCBB payload, and tests distinct base/patch/DLC assets without assuming an archive-collision priority. Mad Max virtual-class names now view packfile-owned bytes instead of dangling temporary strings; its synthetic skeleton exports JSON and glTF. The separate Mad Max mixed module fixture still lacks a database path.
