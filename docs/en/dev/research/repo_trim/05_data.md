# 05 - Game data audit (data/, Lua layer)

Read-only audit, branch chore/repo-trim-audit. Upstream = upstream/main fetched 2026-10-21 (a463bd2220). Sizes are `du -k` on disk (4K block rounding inflates tiny files); "tracked" sizes via git ls-files.

## 1. Summary table

| path | files (tracked) | size | fork-modified? | verdict | evidence | breaks if removed |
|---|---|---|---|---|---|---|
| data/json (excl. onboarding) | 2007 | ~36 MB | no (0 diff vs upstream) | FETCH-FROM-UPSTREAM | `git diff --name-status upstream/main main -- data/` lists no data/json path except onboarding/ | Game cannot load; all tests |
| data/json/onboarding | 5 | 48K | fork-only (all A) | OVERLAY | lessons*.json, screens.json, soft_start.json; loader `add("onboarding_lesson")` in src/init.cpp:415 | onboarding feature, JSON load of those types |
| data/mods/bn | 1 | 4K | no | FETCH (core) | only `core:true` mod | everything |
| data/mods/TEST_DATA | 28 | 424K | 1 file fork-only (r3d_missing_art.json) | OVERLAY (or KEEP, see 5) | tests/test_main.cpp:161 force-loads `test_data`; tests/r3d tests use `test_r3d_missing_art_*` | test suite |
| data/mods/Arcana_BN | 185 | 6.2M | tools.json differs only because fork is 1 upstream commit behind (#10676), not a fork change | FETCH | `git diff main...upstream/main -- data` = that one file | none for 3D |
| other mods (see 3) | ~920 | ~11M | no | FETCH or DROP | per-mod table below | CI all-mods run only |
| data/mods/default.json, replacements.json, external.json | 3 | small | no | FETCH | path_info.cpp:349-353 | mod manager load |
| data/raw/r3d | 7 | 72K | fork-only | KEEP | wall_sides.{json,png}, presentation_profiles.json, items/*.vox, vehicle_parts/*.vox; src/r3d.cpp:382 | 3D walls, profiles, voxel parts |
| data/raw/keybindings/keybindings.json | 1 | - | modified (+r3d_toggle, r3d_rotate_left/right) | OVERLAY (patch) | diff shows only appended r3d entries | 3D toggle/rotate keys |
| rest of data/raw (colors, fonts, languages, tips, sokoban, other keybindings, *.lua docgen, on_game_start.lua) | 13 | ~250K | no | FETCH | path_info.cpp | startup |
| data/shaders | 0 tracked (gitignored /data/shaders/) | 236K local | generated | DROP from git (already) | .gitignore:28; CMakeLists.txt:454-512 generates via shadercross; fallback "precompiled shaders from data/shaders" | GPU renderer if shader build disabled |
| data/names | 6 | 4.7M | no | FETCH | path_info names_file | new-character names |
| data/font | 4 | 14M | no | FETCH (could DROP unifont.ttf 12M?? unverified use) | path_info fontdata | text rendering |
| data/sound/Basic | 5 | 28K | no | FETCH | path_info data_sound | sound |
| data/help, motd, title, credits | 1/10/6/3 | 68K/64K/24K/12K | no | FETCH; DROP non-en motd/title/credits (cosmetic) | path_info find_translated_file | menu text |
| data/lua | 3 | 12K | no | FETCH | data/lua/lib/ui.lua | Lua mods using `require` |
| data/XDG | 3 | 336K | no | DROP (excluded from install; Linux desktop packaging only) | data/CMakeLists.txt PATTERN "XDG" EXCLUDE | desktop launcher |
| data/changelog.txt, cataicon.ico | 2 | 176K/148K | no | DROP/FETCH (cataicon: Windows) | - | none functional (unverified: grep for changelog.txt use) |
| data/CMakeLists.txt | 1 | 4K | no | KEEP | install(DIRECTORY ...) | release install |

## 2. Fork-only vs upstream (item 1)

`git diff --name-status upstream/main main -- data/` : 14 Added, 2 Modified, 0 Deleted. Everything else (about 3,100 tracked data files, ~17.6 MB on tracked-size basis for mods, 36 MB json) is byte-identical. The fork's real data delta is 16 files, ~1,020 added lines:

- data/json/onboarding/* (5 files)
- data/mods/TEST_DATA/r3d_missing_art.json
- data/raw/r3d/* (7 files: presentation_profiles.json, wall_sides.json/.png, 2 READMEs, 3 .vox)
- data/raw/keybindings/keybindings.json (modified, +3 bindings)
- data/mods/Arcana_BN/items/tools.json (artefact of being 1 upstream commit behind; `git rev-list main..upstream/main` = 1; no fork edit)

No sidebar/UI JSON exists in data/ as fork-only; the sidebar is C++/Lua (src/lua_sidebar_widgets.*, src/lua_action_menu.*). No imgui leftovers found in data/ (`grep -l imgui data` empty; no tracked file name contains imgui). Not checked: imgui in src/third-party or CMake.

## 3. Mods (item 3)

Only `bn` is core. default.json (the list enabled by default for new worlds) = bn, bn_lua, no_npc_food, novitamins, No_Rail_Stations, no_reviving_zombies, limit_fungal_growth, udp_redux, pride_flags, tablet_ebook, change_hairstyle, cbm_slots. Note `bn_lua` has no directory in data/mods (it is a core Lua mod provided from data/json/lua, unverified); new-world default would warn if a listed id is missing, so default.json must stay consistent with whatever mods are shipped.

Sizes are du-K and tracked file count. Categories from modinfo.json.

| mod | files | size | note | verdict for 3D game |
|---|---|---|---|---|
| bn | 1 | 4K | core | KEEP/FETCH |
| TEST_DATA (test_data) | 28 | 424K | force-loaded by tests | KEEP for tests, exclude from release |
| Aftershock | 165 | 1.1M | total content, deps exotic_ammo | optional |
| Arcana_BN | 185 | 6.2M | magic content | optional |
| MagicalNights (Magiclysm) | 161 | 1.9M | magic; in replacements.json | optional, popular |
| DinoMod | 132 | 1.7M | creatures | optional |
| UDP_Redux | 13 | 1.8M | tileset-art mod, default-on | DROP for 3D unless ultica tileset is used (tileset-specific, unverified) |
| UDP_BN_FAKE_SNOW | 4 | 1.1M | tileset art | DROP (tileset-specific) |
| MoreVehicleColors | 3 | 192K | graphical | optional |
| alt_map_key | 3 | 184K | graphical | optional |
| exotic_ammo | 71 | 372K | dependency of Aftershock, CheesyInnaWoodsFixes, Fuji_Mil_Prof | keep if those kept |
| CRT_EXPANSION 52 / 464K, Civilians 32 / 288K, My_Sweet_Cataclysm 26 / 108K, innawoods 8 / 64K, desert_region 12 / 104K, CheesyInnaWoodsFixes 17 / 108K | | | content mods | optional |
| pride_flags 11 / 132K, change_hairstyle, cbm_slots, tablet_ebook(dir ebook_lua), novitamins, no_npc_food, limit_fungal_growth, no_reviving, No_Rail_Stations | small | | default-on | keep if default.json unchanged |
| No_* / Only_Wildlife / classic_zombies (No_Acid_Zombies, No_Anthills[obsolete], No_Blobs, No_Cockroaches, No_encum_backpacks, No_Explosive_Zombies, No_Feral_Humans, No_Fungi, No_Global_Uniques, No_Lift_Requirements, No_NPC_Food, No_Rail_Stations, No_Reviving, No_Wasps, No_Zombify, NoVehicleColors, Only_Wildlife) | 1-5 each | 4-20K | monster_exclude / rebalance toggles, tiny | droppable; ~20 dirs, ~100K total |
| Lua demo/test mods: callback_lua_test, NPC_lua_hook_test, saveload_lua_test, Spawn_Hook_Test, Test_for_Lua_hooks, test_lua_conditions, test_lua_require, lua_ai_attitude_debug, lua_special_attack_demo, gunchair, nuclear_tear, smart_house_remotes (all `obsolete:true` except lua_ai_attitude_debug, lua_special_attack_demo, ebook_lua) | ~3-9 | 8-56K | dev/test; obsolete ones are hidden from the UI | DROP, unless Lua tests reference them (see below) |
| rpg_system 8 / 96K | | | Lua example/gameplay mod | optional |
| Others (Color_Expansion, Dynamic_Character, Fuji_Mil_Prof, JP_Weather_Variants, Jump Pad, lethal_zeds, rocky_zombies, ManualBionicInstall, MMA, Monster_Girls, Old_Mutations, pathfinding_stockfish, resurrection_mod, skills_through_kills, speedydex, stats_through_kills, StatsThroughSkills, teleportation_mod, underground_dynamic_temperature, Dangerous Fungi, flowerpots_plus, CrazyCataclysm) | | | rebalance/content | optional |

I checked categories/obsolete/dependencies from modinfo.json programmatically; I did not review gameplay contents of each mod beyond that. Total mods on disk 18M; tracked ~17.6M.

What depends on mods:
- Tests: tests/test_main.cpp:161 adds `test_data` (dep `bn`) to every run; no tests/*.cpp references Aftershock/Magiclysm/Arcana/etc. by id (grep: only src/character.cpp, src/npc.cpp, src/dependency_tree.cpp mention such names; dependency_tree is a string match, not data load). tests/catalua_test.cpp uses embedded Lua, not mod dirs (unverified for every case).
- Build/CI: build-scripts/build.sh:88-98 and .github/workflows/matrix.yml:241-246 run all non-obsolete, non-blacklisted mods via get_all_mods.py reading `data/mods/*/modinfo.json`. This CI job is the only dependence on the optional mods. AGENTS.md says no CI runs on this fork, so it is effectively unused; deleting mods is safe wrt this script (it globs).
- build-scripts/lint-json.sh: `find .` over entire repo, no mod dependency.
- Release: .github/workflows/build.yml bundles registry mods via build-scripts/bundle-registry-mods.ts (network); CMake macOS bundle copies data/mods wholesale (CMakeLists.txt:976).

## 4. Fork-specific data the renderer needs (item 4)

Must stay in the fork: data/raw/r3d/**, data/json/onboarding/**, the three r3d keybindings, data/mods/TEST_DATA/r3d_missing_art.json (tests only), tests/data/r3d_geometry_snapshot.txt. Shader sources are outside data/ (CMake generates data/shaders at build time into the source tree; ignored by git), so a build needs shadercross or a precompiled set placed in data/shaders; with a fetched data tree, the shader output directory (CMakeLists.txt:454, 484, 562) must still point at the final datadir.

## 5. Tests and minimal data set (item 5)

- tests/ is 3.6M; tests/data is 100K (item_factory.json, name.json, r3d_geometry_snapshot.txt, save/broken_load_save).
- Test binary loads real game data: `init_global_game_state` (test_main.cpp:419) loads datadir json + mods bn + test_data. Datadir defaults to relative `data/` (path_info.cpp:128, `--datadir` override at main.cpp:531; same for tests: unverified whether test_main accepts --datadir, only main.cpp confirmed). Tests run from the build tree with `data/` found relative to cwd, so the working directory must contain data/.
- Minimal set for tests = data/json (all), data/raw (colors, fonts, keybindings, languages, tips), data/mods/{bn,TEST_DATA,default.json,replacements.json}, data/names, data/help, data/font, data/sound if sound tests run, plus data/raw/r3d and onboarding. You cannot shrink data/json meaningfully: items, mapgen (10M), monsters etc. are cross-referenced and loaded in full. External tileset PNGs in data/json/external_tileset (4.7M, 48 files) could be skipped only if the loader tolerates absence (unverified).
- Check Lua tests: catalua tests that load mod dirs would need the Lua test mods; not confirmed.

## 6. Lua layer (item 2/scope)

- src/lua: 65 files, 780K: vendored Lua C sources (lapi.c etc.). src/catalua*.cpp/h (bindings), lua_action_menu, lua_sidebar_widgets are fork/upstream engine code, not data; KEEP.
- docs/en/mod/lua: 84K (explanation/guides/reference/tutorial, hooks.md), generated via `deno task docs:gen` and data/raw/generate_docs.lua (src/main.cpp:875 uses datadir/raw/generate_docs.lua). Keep generator scripts in data/raw; docs are optional/regenerable.
- data/json/lua/*.lua (cooking, iuse/*, mapgen/*, nuclear_tear, minirose) are upstream content under data/json: FETCH.

## 7. Options for "fetch from upstream" (item 2)

How the game finds data: PATH_INFO::datadir() = `<base>/data/` (or `<prefix>/share/cataclysm-bn/` with DATA_DIR_PREFIX), overridable by `--datadir`; everything else (mods/, raw/, json/, names/, font/, sound/, shaders/) is a subpath of this one dir. Install rule: data/CMakeLists.txt installs the whole dir. So a single datadir that is a merge of upstream data plus overlay is sufficient; no code change needed.

| option | pros | cons |
|---|---|---|
| git submodule/subtree of upstream at `third_party/upstream` plus overlay dir | exact pin, offline builds after clone; subtree retains history-free tree | submodule pulls entire upstream repo (src, docs, lang: large, unless sparse); subtree reintroduces the bytes you want gone |
| Sparse checkout (data/ only) in a script-managed dir | small | not a git-tracked pin unless script records a SHA; fork must record commit hash in a file |
| Release tarball pinned by URL+sha256, CMake `file(DOWNLOAD)`/ExternalProject | reproducible, CI-friendly | needs network at configure time; releases may not include test-only mod TEST_DATA (unverified, tarballs usually ship only shipped mods) so tests need source archive instead |
| Source archive of a pinned commit (`archive/<sha>.tar.gz`, extract data/ only) | includes TEST_DATA; exact | network; big download (whole repo) each fresh build |

Recommendation (unverified in practice): pin a SHA in `data-upstream.lock`, a script/CMake step that does sparse-checkout of upstream `data/` (and `lang/`? not in scope) into `build/upstream-data/`, then copies overlay on top into the staged `build/data/` which becomes `--datadir`.

What the overlay contains: data/raw/r3d/, data/json/onboarding/, keybindings patch, TEST_DATA/r3d_missing_art.json, (shaders output). Keybindings.json is a whole-file replacement of upstream's, so a merge would conflict on upstream edits; better to move r3d keybindings into a separate new JSON file in data/raw/keybindings/ (loader reads that directory, path_info.cpp:250, unverified whether it globs all files or only named ones) to avoid a patch.

What breaks: (a) CMake install/OSX bundle and Windows steps reference `${CMAKE_SOURCE_DIR}/data/...` directly (CMakeLists.txt:454-983) and need a DATA_STAGE variable; (b) scripts that glob `data/mods/*/modinfo.json` and `lint-json.sh` need staged tree; (c) developers lose in-tree grep/edit of data; (d) upstream data/Lua and C++ must stay version-compatible: fork src/ must be synced with the pinned upstream data SHA (JSON schema changes land with C++ changes). That coupling is the largest risk: today data is identical to upstream except 16 files, but src/ diverges. (e) Git-ignored data/shaders written into source tree by CMake.

## 8. Proposed layout (frontend + overlay)

```
data-upstream.lock         # upstream SHA (+ optional sha256 of tarball)
data-overlay/
  raw/r3d/...
  raw/keybindings/r3d.json         # split from keybindings.json
  json/onboarding/...
  mods/TEST_DATA/r3d_missing_art.json
  mods/default.json                # only if default mod list is changed
tools/stage_data.{sh,py}           # fetch pinned upstream data/, rsync overlay -> build/data
data/CMakeLists.txt -> uses ${CATA_DATA_STAGE}
tests/data/ (keep, 100K)
```

Release data drops: data/XDG, changelog.txt, non-en motd/title/credits (optional), obsolete/dev mods, UDP tileset mods unless the ultica tileset is bundled.

## 9. Totals

Tracked data: ~3,350 files (json 2012, mods 1137, others ~200), ~55 MB on disk (json 36M, mods 18M, font 14M... note du sums include untracked shaders and block rounding; font and names are tracked in git). Fork-only: 16 files, ~100K. Overlay ~100K plus ~430K if TEST_DATA copied whole. FETCH candidates: ~99.5% of data bytes. Droppable outright regardless of strategy: ~30 test/dev/No_* mods (~700K), XDG (336K), non-en text (~100K): small; the savings come from FETCH, not DROP.

## 10. Risks

1. src/data version coupling (see 7d); a fresh pinned SHA must be tested with `cata_test-tiles` and `--check-mods`.
2. Tests need TEST_DATA, which release tarballs may omit.
3. Build needs network/lockfile; offline dev loses data.
4. Overlay of keybindings.json is a conflict magnet until split.
5. Fork is 1 commit behind upstream in data/ (Arcana tools.json); trivial but shows drift.
6. default.json lists bn_lua, udp_redux etc.; dropping mods without editing default.json produces missing-mod errors on world creation (unverified behavior).
7. Unverified: font/unifont.ttf usage (12M?), external_tileset consumers, changelog.txt use, whether test_main honors --datadir.

## 11. Owner questions

1. Is the fork's src/ intended to track upstream src closely enough to pin data to the same SHA, or is data-from-upstream only for a subset (json/mods)?
2. Ship Magiclysm/Aftershock/Arcana/DinoMod, or core `bn` + default-on mods only?
3. Is offline build a requirement (rules out configure-time download)?
4. Which tileset does the 3D game bundle (decides UDP_* mods and external_tileset)?
5. OK to split keybindings into a new file (needs verifying the loader globs the dir)?
6. Do you want Lua demo/test mods kept as examples for scripting docs?
