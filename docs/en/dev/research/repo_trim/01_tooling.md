# Repo trim audit 01: non-C++ tooling

Read-only audit, branch chore/repo-trim-audit. Sizes are tracked-file bytes (`git ls-files`). "Unverified" is marked where I did not trace it.

## Direct answers

- **Do we need deno?** Not to build or test. Only three things use it: `src/CMakeLists.txt:193-199` (POST_BUILD `deno task docs:gen`, auto-ON only if `find_program(deno)` succeeds, `CMakeLists.txt:60-66`), `build-scripts/fmt.sh:53,58,101-102` (deno fmt / dprint for md/json/ts/lua), and AGENTS.md:99-100,132. Hard requirement: none. Real function: regenerating Lua docs and `lua_annotations.lua`. Note `docs:gen:lua` is just `$CATA_EXE --lua-doc ... --lua-types ...` (deno.jsonc), so it needs the game binary, not deno. Only `docs:gen:cli` needs `scripts/gen_cli_docs.ts`. Neither `lua_annotations.lua` nor `docs/en/mod/lua/reference/lua.md` currently exists in the tree (not generated yet here).
- **Any JS/TS?** 48 .ts + 2 .js tracked repo-wide (verified by `git ls-files`). 17 are in scripts/, 3 in build-scripts/ (`bundle-registry-mods*.ts`, `run-linux-test-shards.ts`), the rest in docs/ (docs/_cms.ts, docs/plugins/*.ts, docs/redirect.js, docs/tools/tileset_web_tool.js, out of my scope, belongs to the docs site). No C++ build or Catch2 test touches any of them (grep of src/tests/CMake: only src/CMakeLists.txt:193-199).
- **Any Python?** Build: none. Tests: none (tests/CMakeLists.txt has no python/script refs), except the optional clang-tidy plugin lit tests (`tools/clang-tidy-plugin/test/check_clang_tidy.py`, `CATA_CHECK_CLANG_TIDY`, CMakeLists.txt:74). Dev workflow: `build-scripts/format-cpp.sh:80-88` hard-calls `tools/r3d_spike/astyle_347.py` (AGENTS.md mandates this path), `tools/check_po_printf_format.py` (AGENTS.md:142), `tools/dialogue_validator.py` (justfile, lint-all.sh). `build-scripts/lint-json.sh:3` embeds `python3 -c`. Everything else is upstream legacy.
- **Needed by build**: `build-scripts/{Shadercross.cmake, ValidateShaders.cmake, generate-shaders.sh, WindowsLavapipe.cmake, shadercross/CMakeLists.txt}` (CMakeLists.txt:451,526-529,563,571), `CMakeModules/*`, `lang/CMakeLists.txt` (add_subdirectory(lang), CMakeLists.txt:800; only needs msgfmt + .po), `tools/format` (JSON_FORMAT=ON in linux-full preset, CMakePresets.json:~114), `tools/clang-tidy-plugin` (CATA_CLANG_TIDY_PLUGIN=ON in linux-full, CMakePresets.json:~124). MSVC.cmake / preset-named .cmake only for the Windows preset (CMakePresets.json:16-17).
- **Needed by tests**: nothing beyond the above; clang-tidy plugin `test/` only if you run plugin lit tests.
- **Needed by dev workflow (this fork)**: format-cpp.sh + `tools/r3d_spike/astyle_347.py` (+ its test), fmt.sh/format-*.sh, lint-json.sh, `tools/r3d_spike/*` (run_pure_tests.sh, run_ingame.sh, compare_*, fixtures: r3d-specific, AGENTS.md refers to Lua fixtures), `tools/check_po_printf_format.py`, justfile, `.agents/skills` (Claude/Codex skills).
- **Needed by neither**: all of .github/, android/, msvc-full-features/, build-data/osx, doxygen_doc/, most of scripts/, utilities/, legacy tools/.

## Summary table

| path | size | verdict | evidence | breaks if removed |
|---|---|---|---|---|
| android/ (78 files, incl. deps.zip, keystore) | 45.55 MB | DELETE (or MOVE to branch `android-archive`) | No reference in CMakeLists.txt/src/CMakeLists.txt/tests/CMakePresets (grep). Own gradle + jni/CMakeLists.txt. 3D renderer is SDL_GPU desktop | Nothing in build. 35 src files have `ANDROID` ifdefs (dead code, out of my scope). `scripts/playtest_android_test.ts` goes too |
| build-data/osx (14 files: icns, iconset, dmgsettings.py, Info.plist, Cataclysm.sh) | 2.16 MB | DELETE if macOS dropped | Only CMakeLists.txt:969-1002 (osx bundle/dmg block, inside an APPLE conditional) | CMakeLists.txt:969-1002 block and `osx-*` presets must be deleted with it |
| .github/workflows (13), actions (3), labeler, semantic.yml, ISSUE_TEMPLATE, vcpkg_ports/triplets | ~0.1 MB total .github | DELETE workflows/actions/labeler/semantic/ISSUE_TEMPLATE; KEEP-or-DELETE `pull_request_template.md`, `llm_review_guide.md` | AGENTS.md says no CI on fork. vcpkg_ports/triplets used only by workflows and msvc (unverified for CMakePresets vcpkg overlay, see questions) | AGENTS.md:57-59 (PR template, `gh pr checks`) text; `deno task semantic` writes `.github/semantic.yml` |
| .agents/skills (12 files) | 56 KB | KEEP; prune 4: changelog-gen, release-gen, deno-script, translate | Agent skill docs, deno.jsonc lists `.agents` in test/lint/fmt include | Nothing built; remove deno-script only if deno goes |
| deno.jsonc, deno.lock | 3 KB, 51 KB | KEEP only if you want `docs:gen`/fmt; else DELETE (decision A below) | CMake auto-detect, fmt.sh, AGENTS.md:99-100,132 | AGENTS.md lines 99-100,132; fmt.sh docs/ts branches; CMake option LUA_DOCS_ON_BUILD |
| dprint.json | 0.3 KB | DELETE with deno (it is `deno task dprint`) | deno.jsonc `dprint` task; fmt.sh:58 | AGENTS.md:100 |
| scripts/ (43 files: 23 .ts, tests, fixtures) | 182 KB | KEEP `gen_cli_docs.ts`, `dax.ts`, `utils.ts`, `deep_sort.ts`, `valibot_utils.ts`, README (only if deno kept); DELETE the rest | changelog/, aside_to_gfm/, docs_translation_warning, semantic, playtest(+android test), affected_files, git_changed_files, tileset*, migrations/, monster_girls_mutations, prices, group_by_type, to_stackable are upstream release/docs/data-migration tools. No build ref | `deno task migrate-unit/changelog:reddit/semantic/docs:*/affected-files` become dead; `just playtest` dead |
| scripts/migrations (8) | ~20 KB | MOVE/DELETE (one-shot data migrations of already-migrated issues #3312 etc.) | not referenced except deno.jsonc `migrate-unit` | task `migrate-unit` |
| utilities/ (7 files, building-utility, make_iso.py) | 87 KB | DELETE | Zero refs anywhere (grep) | nothing |
| tools/clang-tidy-plugin (50 files) | 226 KB | KEEP (decision B) | linux-full preset turns plugin ON; CMakeLists.txt:848-849 | linux-full preset fails to configure unless CATA_CLANG_TIDY_PLUGIN set OFF; src/ NOLINT conventions lose their checker |
| tools/format (json_formatter) | 17 KB | KEEP | JSON_FORMAT=ON in linux-full; CMakeLists.txt:845-846; used by `fmt.sh json` (unverified) | JSON formatting; set JSON_FORMAT=OFF |
| tools/r3d_spike (14 files) | 85 KB | KEEP (r3d-owned) ; rename dir eventually | format-cpp.sh:84-88 depends on astyle_347.py; run_pure_tests.sh is r3d test harness | AGENTS.md formatting workflow, format-cpp.sh |
| tools/check_po_printf_format.py | 4.6 KB | KEEP only if PO files stay | AGENTS.md:142 | AGENTS.md i18n section |
| tools/dialogue_validator.py | 9 KB | KEEP (cheap, JSON lint) | justfile lint-dialogue, lint-all.sh:7 | `just lint` |
| tools/json_tools (8+3 jq), gfx_tools (3), merge_maps.py, palettize.py, vehicleDef.py, copy_from.py, copy_mac_libs.py, windows_limit_memory.py, fix-compilation-database.py, reduce_tests*.sh, repeat_clang_tidy*.sh, run_iwyu.sh, iwyu/, setup[dh]gamelaunch.sh, graphs/damage.r, README | ~330 KB | DELETE (gfx_tools compose/decompose: MOVE if tilesets get edited) | tools/README.md says deprecated. No build/test refs (grep). fix-compilation-database.py unverified | nothing in build; gfx_tools needed only for tileset packing |
| build-scripts: format-cpp.sh, fmt.sh, format-all.sh, format-staged.sh, format-json.sh, format-trailing-return.sh (+test), lint-json.sh, lint-all.sh | ~15 KB | KEEP | FormatSource.cmake:3; justfile; pre-commit | `cmake --build --target format`, AGENTS.md:92-96 |
| build-scripts: Shadercross.cmake, ValidateShaders.cmake, generate-shaders.sh/.ps1, WindowsLavapipe.cmake, shadercross/ | ~20 KB | KEEP | CMakeLists.txt:451,526,563,571 | GPU shader build |
| build-scripts: MSVC.cmake, VsDevCmd.cmake, windows-tiles-sounds-x64-msvc.cmake, x64-windows-static.cmake, *.sha256 (SDL2 mingw, libbacktrace) | ~20 KB | KEEP if Windows supported else DELETE | CMakePresets.json:16-17; sha256 files are SDL2/mingw legacy (unverified consumer) | windows presets |
| build-scripts: build.sh, gha_compile_only.sh, gha_test_only.sh, run-linux-test-shards.ts, bundle-registry-mods(.ts,_test), get_all_mods.py, mod_test_blacklist, requirements.sh, problem-matchers/, CMakeUserPresets.json.in, linux-test-profile.md, clang-tidy-wrapper.sh, build-clang-tidy-plugin.sh, run-clang-tidy-plugin.sh | ~50 KB | DELETE (clang-tidy 3 scripts: KEEP if you run plugin from CLI; only referenced by workflows) | referenced only by .github/workflows and each other (grep) | nothing local |
| msvc-full-features/ (12) | 42 KB | DELETE | Legacy VS project; refs only inside itself; CMakeSettings.json is the modern path | docs/.../vs_cmake.md unaffected |
| cmake-build.ps1, cmake-build.bat, setup.ps1 | 72 KB | DELETE (or KEEP with Windows) | No refs in CMake; Windows convenience wrappers | none on Linux |
| CMakeSettings.json, launch.vs.json | 9 KB | DELETE with Windows | VS IDE only (docs vs_cmake.md:17) | VS open-folder |
| doxygen_doc/ | 91 KB | DELETE | .gitignore only; no CMake target (grep) | nothing |
| pch/main-pch.hpp, update-pch.sh | 1.5 KB | KEEP pch/ only if referenced (unverified; no grep hit in CMakeLists); update-pch.sh DELETE | | |
| cataclysm-launcher | 1 KB | DELETE (upstream Linux tarball launcher) | no refs | none |
| lang/ scripts (extract_json_strings.py 58 KB, update_pot.sh, merge_po.sh, compile_mo.sh, concat/dedup, unicode_check.py, *.bat, update_stats.sh, discard_invalid_po.sh, bn_extract...) | ~120 KB | DELETE scripts; KEEP lang/CMakeLists.txt + po/ | Upstream Crowdin/pot workflow (.tx/config, push-translation-template.yml). Build only needs msgfmt (lang/CMakeLists.txt:60-65) | `LANGUAGES=none` still works; pot extraction gone (pull .pot from upstream) |
| .tx/config | 0.2 KB | DELETE | Transifex/Crowdin | none |
| justfile | 0.8 KB | KEEP (trim `playtest`, deno recipes) | fmt/lint wrappers | |
| .pre-commit-config.yaml | 0.2 KB | KEEP | calls fmt.sh | |
| SIGNOFF, json_blacklist, .gitmessage, .vscode, .zed | small | KEEP (editor/dev) except SIGNOFF (unverified, likely upstream) | | |

## Sections

### Build-critical (do not touch)
The linux-full preset (CMakePresets.json:103-135) enables `JSON_FORMAT` and `CATA_CLANG_TIDY_PLUGIN`, so `tools/format` and `tools/clang-tidy-plugin` are required for the documented build command in AGENTS.md. If you want lean, flip those two to OFF in the preset; otherwise keep both dirs.

### Deno decision
- A (drop deno): delete deno.jsonc/lock, dprint.json, scripts/, `.agents/skills/deno-script`, fmt.sh docs/json/ts branches; remove CMakeLists.txt:60-66 and src/CMakeLists.txt:193-199; edit AGENTS.md 99-100 and 132 and replace with direct `cataclysm-bn-tiles --lua-doc docs/en/mod/lua/reference/lua.md --lua-types lua_annotations.lua`. Loses markdown/JSON(deno) formatting and `docs:gen:cli`. This also drops the docs/ site tooling (docs/deno.jsonc, docs/*.ts) which is out of my scope but depends on it.
- B (keep minimal deno): keep deno.jsonc trimmed to `docs:gen*` and `dprint`, plus scripts/gen_cli_docs.ts, utils.ts, dax.ts, deep_sort.ts, valibot_utils.ts. Everything else in scripts/ goes. Caveat: `scripts/utils.ts:7-8` imports `jsr:@std/async/unstable-mux-async-iterator` and `deno.land/std@0.208.0/fs/_util.ts` by URL; fragile, and gen_cli_docs.ts only imports `@std/collections` so can be made standalone (verified imports lines 1-6).
- Recommendation: A, with a Lua doc one-liner. Python is already required for astyle wrapper, so deno is the only extra runtime.

### Merge hazard (blunt)
Fork pulls from upstream. Every deleted upstream-owned file (workflows, scripts, android, lang scripts) will produce modify/delete conflicts whenever upstream edits it. Mitigation: keep a `.gitattributes`/merge strategy (`git merge -X ours` does not solve modify/delete), or a post-merge script that re-deletes a path list. Per ~/.claude/rules/no-data-loss.md: do the deletion on a branch, back up untracked work first, never `git clean`. Not verified how often upstream touches these paths.

## Totals
Removable (DELETE verdicts, excluding conditional Windows/macOS/deno-B): android 45.55 MB + build-data 2.16 MB + .github ~0.1 MB + msvc 0.04 + doxygen 0.09 + utilities 0.09 + legacy tools ~0.33 + scripts ~0.17 + build-scripts CI ~0.05 + lang scripts ~0.12 + misc ~0.1 = about **48.8 MB across roughly 340 files** (android 78, build-data 14, .github ~34, msvc 12, doxygen 2, utilities 7, scripts ~38, tools legacy ~40, build-scripts ~20, lang ~14, misc ~15; file counts approximate, sizes from `git ls-files` byte sums). Conditional extra if Windows dropped: ~0.2 MB. Cleanup is dominated by android/ (93%) plus build-data/osx (4%); the rest is rounding error for repo size but matters for clutter.

## Questions for the owner
1. Windows support: keep (MSVC.cmake, WindowsLavapipe, VsDevCmd, windows preset, CMakeSettings, setup.ps1) or Linux-only? macOS (osx presets, build-data/osx, CMakeLists.txt:969-1002)?
2. Deno: option A (drop) or B (minimal docs:gen)? Does the docs/ site stay in this repo?
3. Keep clang-tidy plugin and json_formatter in the linux-full preset, or set both OFF?
4. Ship `lang/po` and Crowdin tooling, or build with `LANGUAGES=none` and pull translations from upstream?
5. Is android/ wanted on an archive branch, or just deleted (history keeps it)?
6. `.github/vcpkg_ports/freetype` + triplets: is any local vcpkg overlay pointing at them (not checked outside CMakePresets/CMakeLists grep)?
7. Which merge strategy for upstream conflicts on deleted paths?
