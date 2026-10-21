# Repo trim audit 02: assets (gfx, sound, fonts, binaries, .git history)

Read-only audit, branch chore/repo-trim-audit, 2026-10-21. Sizes are working-tree du unless noted.

## Summary table

| path | size | verdict | source repo | licence | evidence |
|---|---|---|---|---|---|
| gfx/UltimateCataclysm | 3.2 MB | KEEP (tests need it); optionally FETCH-AT-BUILD later | I-am-Erk/CDDA-Tilesets | CC-BY-SA 3.0 (per upstream CI credits text; repo GitHub licence = NOASSERTION) | 26 `load_tileset("UltimateCataclysm"` calls in tests/r3d_*; src/r3d.cpp compare default |
| gfx/HoderTileset | 0.8 MB | KEEP (1 test) | originally CDDA tileset, bundled by upstream BN; not in I-am-Erk/CDDA-Tilesets (UNVERIFIED origin) | UNVERIFIED | tests/r3d_presentation_profiles_test.cpp:483 `load_tileset("hoder"`; src/options.cpp:1469 |
| gfx/ASCIITileset | 56 KB | KEEP | upstream BN / CDDA | UNVERIFIED | tests/tile_helpers.cpp loads "ASCIITiles" as the test fixture default |
| gfx/tinytile.png, tile_config.json, tile_config_template.json | small | KEEP | upstream | - | src/path_info.cpp:218 default tileset name |
| gfx/MSX++UnDeadPeopleEdition | 20 MB | DELETE | UndeadPeople tileset (SomeDeadGuy), vendored by BN (UNVERIFIED) | UNVERIFIED (CC-BY-SA likely) | no reference in src/tests/CMake |
| gfx/ChestHole16Tileset | 3.0 MB | DELETE | upstream BN vendored | UNVERIFIED | no references |
| gfx/RetroDaysTileset + RetroDays+Tileset | 0.97 MB | DELETE (fetchable: Retrodays in I-am-Erk) | I-am-Erk/CDDA-Tilesets (RetroDaysTileset.zip release asset) | CC-BY-SA 3.0 | no references |
| gfx/BrownLikeBears | 0.77 MB | DELETE (fetchable) | I-am-Erk/CDDA-Tilesets (BrownLikeBears.zip) | CC-BY-SA 3.0 | no references |
| gfx/HitButton_iso | 0.41 MB | DELETE or FETCH | I-am-Erk/CDDA-Tilesets (dir HitButton_iso; release has SmashButton_iso.zip) | CC-BY-SA 3.0 | only env default in src/r3d.cpp:1691 and tools/r3d_spike/compare_iso.sh (dev comparison, env overridable) |
| data/sound (Basic) | 28 KB | KEEP | in-tree upstream | AGPL-ish project licence (UNVERIFIED) | trivial, game loads it |
| data/font (unifont.ttf 12 MB, Terminus.ttf, terminus.fon, map_font_LARWICK.png) | 14 MB | KEEP (see below) | unifoundry / Terminus | GPLv2+ font exception; OFL (LICENSE.txt, LICENSE-OFL-Terminus-Font.txt) | game fallback fonts; CMake copies data/font |
| data/title | 24 KB | KEEP/ignore | upstream | - | text |
| data/raw/r3d | 72 KB | KEEP | this fork (own code assets, .vox, wall_sides) | project | src/r3d.cpp:382 loads wall_sides.{json,png} |
| data/shaders, src/shaders | 236 KB, 124 KB | KEEP | this fork | project | code |
| data/XDG, data/cataicon.ico | 336 KB, 148 KB | KEEP (Linux packaging) / ico DELETE if no Windows | upstream | - | metadata for packaging |
| android/app/deps.zip | 44 MB (blob in history twice, 44.6+40.9 MB) | DELETE with the android/ tree | upstream | - | largest tracked file; not relevant to SDL_GPU desktop |
| build-data/osx/*.icns | ~1.9 MB | DELETE if macOS packaging dropped | upstream | - | - |
| docs/*/**/Melee Weapon Evaluation.ods (x2), docs/ko png | 1.45 MB x2 + | out of scope (docs agent) | - | - | - |
| data/json/external_tileset, data/mods/UDP_*, UDP_BN_FAKE_SNOW pngs | ~8 MB | out of scope; mods art, keep with game data pulled from upstream | - | - | listed in biggest-files |

## Key finding: gfx matches upstream, and upstream already fetches tilesets

- `git diff main upstream/main -- gfx data/sound` is empty: this repo ships exactly upstream BN's 9 tilesets. Real upstream BN (cataclysmbn/Cataclysm-BN) tracks them in-tree too.
- There is NO separate BN tileset or soundpack repo. Verified 404: cataclysmbn/CDDA-tilesets, tilesets, soundpacks, Cataclysm-BN-tilesets. Org repos are only: Cataclysm-BN, blog, cataclysmbn.org, registry, registry-index, translations.
- Upstream CI (.github/workflows/build.yml:163, input `bundle-tilesets`) downloads release assets `UltimateCataclysm.zip` and `ChibiUltica.zip` from I-am-Erk/CDDA-Tilesets release `2026-05-03` with `gh release download` and unzips over gfx/. Mods and soundpacks are bundled from cataclysmbn/registry via build-scripts/bundle-registry-mods.ts. So a fetch mechanism already exists, as a script.
- Third-party source: I-am-Erk/CDDA-Tilesets (verified via gh api; GitHub licence field NOASSERTION; the BN CI credits text states CC-BY-SA 3.0). Release 2026-05-03 asset UltimateCataclysm.zip is 4.7 MB; newest release tags are 2026-10-18 and 2026-10-11. Tileset directories in that repo: ASCII_Overmap, Altica, BrownLikeBears, Chibi_Ultica, GiantDays, HitButton_iso, HollowMoon, IsoMSX+, Larwick_Overmap, MShockXotto+, Mushroom-Dream, NeoDays, PenAndPaper, Retrodays, SurveyorsMap, Ultica_iso, UltimateCataclysm. No Hoder, no ASCIITileset, no MSX++UnDeadPeople, no ChestHole.

## Who REQUIRES what

- Tests (grep of tests/ and src/): UltimateCataclysm (26 loads across r3d_entities, r3d_presentation_profiles, r3d_furniture, r3d_vehicle_rules, r3d_geometry_snapshot, r3d_sprite_bounds tests), "hoder" (1 load), "ASCIITiles" (tests/tile_helpers.cpp fixture). No other tileset id appears in tests/ or CMake.
- tests/data/r3d_geometry_snapshot.txt fingerprints meshes built from UltimateCataclysm sprites. The in-tree UltimateCataclysm (3.2 MB) differs in size from the 2026-05-03 release (4.7 MB zip, compressed), so the snapshot is pinned to this exact in-tree version. A fetched tileset at a different ref can silently break the snapshot. (I did not diff the contents: UNVERIFIED how they differ.)
- CMake: only install()/copy_directory of gfx and data/font, data/sound (CMakeLists.txt lines ~870, 916, 924, 974). Nothing requires a specific tileset at build time.
- Offline builds: any FETCH-AT-BUILD breaks them and the test suite unless there is a cache/fallback.

## Per-group notes and recommendations

### Tilesets (gfx/)
Minimal set that tests and development truly need: UltimateCataclysm, HoderTileset, ASCIITileset, tinytile.png and tile_config*.json = about 4.1 MB. Everything else (MSX++, ChestHole16, RetroDays x2, BrownLikeBears, HitButton_iso) = about 25.6 MB is unreferenced by code or tests. Delete them from the tree; users who want them install from the CDDA-Tilesets releases, the same way real BN players do.
Do NOT make UltimateCataclysm/Hoder/ASCII fetch-at-build for tests: three of the tests are the renderer's core regression net, and the geometry snapshot is content-pinned. If the owner wants it anyway: CMake `FetchContent_Declare` with `URL https://github.com/I-am-Erk/CDDA-Tilesets/releases/download/<tag>/UltimateCataclysm.zip` and `URL_HASH SHA256=...`, but that changes snapshot inputs and kills offline builds. My recommendation is KEEP in-tree; pin hash in a docs note.
Hoder and ASCII have no upstream repo beyond cataclysmbn/Cataclysm-BN itself (so "fetch" means sparse checkout of upstream, which is just the game-logic pull again). Keep.
Licence caution: shipping CC-BY-SA art requires attribution; the credits file is only generated by upstream CI. Keep each tileset.txt/credits intact.

### Sound
data/sound/Basic is 28 KB (menu sounds). Keep. Extra soundpacks live in cataclysmbn/registry (not in tree). Nothing to trim.

### Fonts
unifont.ttf 12 MB is 87 percent of data/font. It is the CJK/Unicode fallback. FETCH is possible from unifoundry (UNVERIFIED URL/version), but it is needed to run, GPL+font exception, and removing it breaks non-Latin text. Keep unless the owner restricts to English; then the 12 MB could be dropped (check src font fallback loading first, UNVERIFIED).

### r3d assets, shaders
Our own code and data, tiny. Keep. Shaders are code.

### Android/macOS
android/app/deps.zip (44.6 MB) is the single biggest tracked file; the android tree totals ~45 MB. Remove with the Android platform (belongs to the platform audit). build-data/osx ~1.9 MB likewise.

## Totals (working tree)

- gfx deletions: about 25.6 MB (MSX++ 20 + ChestHole16 3.0 + RetroDays 0.97 + BrownLikeBears 0.77 + HitButton_iso 0.41 = 25.2, plus rounding; du shows 30 MB gfx total, 4.1 MB kept).
- android/: ~45 MB. build-data/osx: ~1.9 MB.
- Optional: unifont 12 MB.
- Realistic removable: ~25 MB (gfx) + ~47 MB (android/osx) = ~72 MB; up to ~84 MB with unifont. Total tracked tree is ~78 MB of the listed file sizes (du of ls-files in chunks; imprecise).

## .git history (7 GiB)

- `git count-objects -vH`: size-pack 7.00 GiB (3 packs, 603k objects), loose 22.7 MiB, plus **garbage: .git/objects/pack/tmp_pack_q0VdVp, 3.51 GiB** (a crashed/aborted repack from 18:55 today). That temp file is not history; it is pure waste and safe to delete once nothing is running a repack (I did not touch it; verify no git process holds it first). `prune-packable: 1370`.
- The repo is flagged shallow (.git/shallow has 1 entry) yet has 90,567 commits across 1288 refs, so it is a mixed/deepened clone with many refs.
- Dominant content, by on-disk packed size of unique blobs (my aggregation by path, approximate): lang/po about 5.5 GiB (12.6k blob versions; ru_RU.po alone is ~15 MB per version, dozens of versions), src 291 MB, then old tileset history: ChestHole32Tileset 140 MB, MSX++DeadPeopleEdition 138, MSX++UnDeadPeopleEdition 136, android/app 133 (deps.zip), ChestHoleTileset 110, ChestHole32Tileset_iso 80, MShock32TilesetModded 68, MShock24TilesetModded 53, MShock32 34, MShock24 31, ChestHole16 26, Coleen32 17, plus data/json 95 MB. Current-tree gfx is therefore a minor part of history (~1 GiB of historic tilesets, many already deleted upstream).
- Biggest single blobs: android/app/deps.zip 44.6 and 40.9 MB, gfx/Coleen32Tileset/Coleen.png 27 MB, lang/po/ru_RU.po ~15.7 MB (many versions), Coleen24 15 MB.
- Conclusion: translations (lang/po) are the history hog (~78 percent), not graphics. Trimming gfx in the working tree saves almost nothing in .git. What helps:
  1. Delete the 3.51 GiB tmp_pack garbage (immediate, ~3.5 GiB).
  2. Fresh `git clone --filter=blob:none` (blobless) or `--depth` for contributors/CI: fetches blobs on demand; cuts the clone dramatically. Cheap, no rewrite.
  3. If lang/ is dropped from the lean repo (remove lang/po, use cataclysmbn/translations, 7 GB itself), history still carries it; only a history rewrite (git filter-repo on lang/po, android/app/deps.zip, dead tilesets) would shrink it, losing hash compatibility with upstream and breaking `merge upstream/main`. Not recommended while you pull from upstream. git-lfs would not help existing history and adds hosting cost; only useful for future binary changes, and the lean tree has few.
  4. Alternative lean strategy: a new orphan/squashed repo that consumes upstream as a submodule or fetched tarball, losing history (owner decision).

## Risks

- Deleting UltimateCataclysm/Hoder/ASCII breaks 27+ renderer tests and the geometry snapshot. Do not.
- Deleting gfx dirs diverges from upstream: every `merge upstream/main` touching them yields modify/delete conflicts (upstream rarely edits those dirs; low but nonzero), and the no-data-loss rule applies when merging.
- Upstream tile_config of UltimateCataclysm in tree may be older/newer than the release; unpinned fetch changes sprite names that tests/r3d_presentation_profiles JSON reference (e.g. "f_locker").
- Licence: CC-BY-SA art shipped without a credits file; the bundle-tilesets CI step writes one only for its own downloads.
- src/options.cpp and tileset-selection UI list built-in tilesets; verify nothing hard-fails if a listed tileset directory is missing (UNVERIFIED).

## Owner questions

1. Is Windows/macOS/Android support dropped? (decides android/ 45 MB, osx icons, cataicon.ico)
2. Acceptable to lose MSX++, ChestHole16, RetroDays, BrownLikeBears, HitButton_iso from the tree? (HitButton_iso is the default 2D "compare" tileset in tools/r3d_spike/compare_iso.sh; switch default to ASCII/Hoder or keep.)
3. Keep unifont (12 MB) for non-English text?
4. Is rewriting history (breaking upstream merges) on the table, or only blobless/shallow clones and deleting the tmp_pack garbage?
5. Is lang/ remaining in-tree (it dominates history)? That is the 02-adjacent question for the i18n audit.
