# RAGE 2 embedded ADF inventory

Inspected the RAGE2 IDA instance on port **13338** on 2026-09-26. The IDB and original executable addresses agree for all validated payloads.

Executable: `/mnt/games/SteamLibrary/steamapps/common/RAGE 2/RAGE2.exe`

SHA-256: `824b303339d4f9424c500949517293fa584c0d307893dcff2908b82a0eaaa9b5`

- **1,224** validated ADF v4 files, all in `.rdata`.
- **1,178** contain type definitions and no instances; **46** contain instances.
- **1,222** distinct payloads; two pairs are byte-identical.
- **1,133,121 bytes** of ADF payloads in total.
- **3,706** type records, covering **2,079 distinct type hashes** (hash identity alone does not prove identical layouts).
- Every file has at least one direct IDA cross-reference.
- No payloads overlap; every version-4 candidate from the IDA scan validated.

## Files

- `manifest.csv`: compact complete index with address, RVA, executable offset, size, embedded source comment, counts and extraction filename.
- `manifest.json`: same index plus type names/hashes, instance names, all name-table strings, SHA-256 and direct IDA cross-references.
- `embedded-adf.zip`: all 1,224 exact binary payloads, named with address and embedded ADF source name.
- `extract.py`: standalone standard-library scanner, structural validator and extractor.
- `ida-candidates.json`: original IDA signature/header scan and direct references.

## Representative locations

| Address | Bytes | Embedded source comment |
|---|---:|---|
| `0x141b54b90` | 3267 | TerrainSystemTypes.adf in TerrainSystemTypes 14.0.0 |
| `0x141b55860` | 6410 | TerrainSystemRuntimeTypes.adf in TerrainSystem 96.0.5 |
| `0x141f1b810` | 5058 | pfx.adf in PfxUtils 24.0.3 |
| `0x1420e29b0` | 5950 | AvaModelFormat.adf in AMF 7.0.0 |
| `0x1421c7fa0` | 19092 | VegetationSystemTypes.adf in VegetationSystem 154.0.0 |
| `0x1421cd410` | 5864 | effect_adf.adf in Effect 74.2.8 |
| `0x142217040` | 1338 | StringLookup.adf in HEATWAVE  |
| `0x14221ef10` | 714 | all_light_objects.adf in HEATWAVE  |
| `0x1422605d0` | 929 | shader_library_format.adf in shadercompile 38.1.6 |
| `0x142264080` | 8790 | afsm_adf.adf in alc 13.0.2 |
| `0x142266a00` | 268 | LocationGameObjectAdf.adf in HEATWAVE  |
| `0x14226ce60` | 5950 | AvaModelFormat.adf in AMF 7.0.0 |
| `0x14226ef00` | 340 | PhysicsStreamPatchType.adf in phyterc 151.0.0 |
| `0x14226f420` | 733 | TerrainOccludersType.adf in TerrainSystemTypes 14.0.0 |
| `0x14226f8a0` | 901 | StreamPatchTypes.adf in StreamPatchTypes 14.0.2 |
| `0x14226feb0` | 493 | WorldSettingsType.adf in TerrainSystemTypes 14.0.0 |
| `0x142277810` | 3336 | WorldAudioSystemTypes.adf in WorldAudioSystem 28.0.0 |
| `0x142288790` | 1861 | modelcollection.adf in HEATWAVE  |
| `0x142289150` | 490 | Occluder.adf in Tm2 41.0.1 |
| `0x142290540` | 3279 | RoadGraphData.adf in RoadGraph 28.1.3 |

## Verification and scope

The scanner checked mapped IDA segments and independently scanned the original PE. Validation covers the ` FDA` magic, version, declared length, PE section boundaries, name-table strings, variable-length type records and member/enum name indices, instance ranges/names, and string-hash records. ZIP integrity was checked after extraction.

Representative call sites independently confirm embedded pointers and lengths: `sub_1400792C0` loads `0x142217040` with length 1338; `sub_140DD4180` loads `0x141B54B90` with length 3267 and `0x141B55860` with length 6410 via `sub_140DEBC40`.

The duplicate pairs are `AvaModelFormat.adf` at `0x1420e29b0` / `0x14226ce60`, and `bountypoint_types.adf` at `0x142292ed0` / `0x142339e60`.

This inventory covers complete, uncompressed ADF v4 payloads embedded in the executable. It does not scan external TAB/ARC archives or claim to identify compressed/encrypted resources without an exposed ADF header. Source names are from embedded comments, not guessed runtime asset paths. No IDA names or types were changed.

## Reproduce

```sh
python3 extract.py "/mnt/games/SteamLibrary/steamapps/common/RAGE 2/RAGE2.exe" ./output ./ida-candidates.json
```
