# Repo trim audit 03: docs, READMEs, lang, help text (read-only audit)

Sizes are tracked bytes (git ls-files). Upstream = cataclysmbn/Cataclysm-BN, checked via `gh api` (directory listings only; file-level equality NOT diffed, so "duplicate" = same path exists upstream).

## Summary table

| path | files | size | verdict | evidence | breaks if removed |
|---|---|---|---|---|---|
| docs/ (whole) | 519 | 11.2 MB | trim to ~25 files | upstream has same lume site layout (_config.ts, en/ja/ko/ru/de, plugins, tools) | `deno task docs`, .github/workflows/docs.yml, docs/deno.jsonc site build |
| docs/en/mod/json | 84 | 2.66 MB | DELETE (upstream) | inherited modding docs | nothing in src/tests; only inbound md links from docs/en/dev/reference/tooling.md, contributing.md |
| docs/en/mod/lua (guides) | 8 | 69 KB | DELETE (upstream) | inherited; reference/ holds only .gitkeep | `deno task docs:gen:lua` writes docs/en/mod/lua/reference/lua.md (generated, not tracked); build.yml:413-423 and docs.yml:77 expect that path |
| docs/en/i18n | 13 | 242 KB | DELETE (upstream) | translation workflow docs | links from contributing.md already broken (extensionless) |
| docs/en/contribute | 12 | 1.95 MB (1.93 MB is img/) | DELETE (upstream) except nothing | contributing, changelog_guidelines, docs.md | AGENTS.md:135 (changelog_guidelines), README.md:192/269-271 (docs.md, readme-bug*.png) |
| docs/en/game (manual, lore, tutorials) | 10 | ~73 KB | MOVE to wiki OR DELETE (upstream) | player guides, not fork specific | none in code |
| docs/en/game/changelog | 8 | 182 KB | DELETE (upstream) | stable-0.1..0.12 upstream release notes | none |
| docs/en/install | 1 | 4 KB | KEEP only if rewritten for 3D | install.md | none |
| docs/en/dev/guides/building | 7 | ~60 KB | KEEP cmake.md (+msys/vs_vcpkg if used); drop cygwin, atomic_cmake, vs_external_tool_wsl | CMakeLists.txt:641 cites cmake.md; AGENTS.md:157; README.md:45,171-173 | CMake fatal-error message points at missing file |
| docs/en/dev/guides: formatting, testing, ranges, items, tracy, failed_to_create_glyph_error, working_with_both_bn_and_dda_in_git | 7 | ~40 KB | KEEP formatting, testing, ranges (AGENTS refs); rest MOVE to wiki / DELETE if upstream | upstream has all seven | AGENTS.md:157 (formatting) |
| docs/en/dev/guides: renderer_3d, first_person, ui_overhaul, guided_onboarding | 4 | 19 KB | KEEP (fork-only, created 2026-10-18) | README.md:26,34,37,39; BACKLOG.md; src/onboarding*.cpp comments; tools/r3d_spike/fixtures/README.md:43 | 4 README links, 3 source comments, backlog links |
| docs/en/dev/explanation (code_style, best_practices, points_coordinates, game_objects, accessibility, design_document, user_interface) | 7 | ~? | KEEP code_style (AGENTS.md:90 hard dependency); rest DELETE (upstream) | upstream has design/explanation/guides/reference | AGENTS.md:90 |
| docs/en/dev/reference (faq, tooling, tileset_web_tool + img) | 4+ | ~1.0 MB w/ img (estimated, unverified) | DELETE faq/tooling (upstream); tileset_web_tool.md KEEP only if docs/tools/tileset_web_tool.js stays (unverified relevance to 3D) | cli_options.md is generated into this dir (docs:gen:cli) | docs.yml:74, build.yml:413 refs |
| docs/en/dev/design/vehicle_tiles.md | 1 | 10 KB | MOVE to archive branch or merge into renderer_3d.md | fork-only (added 2026-10-18) | none found |
| docs/en/dev/research/* (5 files) | 5 | 112 KB | MOVE to archive branch `docs-archive` | all fork-specific; first_person_plan 63 KB | AGENTS.md:109 (r3d_review), BACKLOG.md ~15 links |
| docs/en/dev/research/ui_redesign, repo_trim | n/a | n/a | OUT OF SCOPE (untracked WIP) | | |
| docs/ko | 148 | 3.44 MB | DELETE (upstream, translation) | upstream has ko/ | READMEs (README.ko.md only) |
| docs/ja | 145 | 1.84 MB | DELETE (upstream) | upstream has ja/ | README.ja.md |
| docs/ru, docs/de | 17+17 | 184 KB + 117 KB | DELETE (upstream) | upstream has ru/ de/ | none |
| docs/plugins, tools, _config.ts, _cms.ts, deno.jsonc, redirect.js, media.css, favicon, 404.md, LICENSE, README.md | ~15 | ~60 KB | DELETE with the site if docs site moves to wiki; else KEEP | identical file list upstream | docs.yml deploy; root deno.jsonc `docs` task |
| README.md / README.ja.md / README.ko.md | 3 | 15 / 12.5 / 7.4 KB | KEEP README.md, DELETE ja/ko (upstream-style translations) | READMEs link docs/en/... | n/a |
| AGENTS.md | 1 | 15.9 KB | KEEP | agent contract; links 5 docs paths | fix links if docs move |
| BACKLOG.md | 1 | 34 KB | KEEP | single task list per AGENTS.md; 25+ links to research docs | links dead if research moves; repoint to archive branch URL |
| CODE_OF_CONDUCT.md, SIGNOFF | 2 | 1.9 + 2.8 KB | KEEP (licensing hygiene), or DELETE SIGNOFF if DCO not used (unverified) | | |
| .github/llm_review_guide.md | 1 | 0.6 KB | KEEP (AGENTS.md:163) | | |
| .github/ISSUE_TEMPLATE | 3 | 0.7 KB | KEEP but edit config.yml (points at upstream Discord) | | |
| lang/ | 15 | 77 KB | MOVE logic upstream-pull; see lang section | lang/CMakeLists.txt included at root CMakeLists.txt:800 | CMake configure (needs LANGUAGES=none stub or keep dir) |
| data/help/texts.json | 1 | 66 KB | KEEP | loaded as JSON help type at runtime | in-game help |
| data/credits/, data/motd/ | 3 + 10 | 35 KB motd | KEEP | main_menu.cpp:463 loads PATH_INFO::motd(); credits loaded similarly | main menu MOTD/Credits |
| data/changelog.txt | 1 | 179 KB | DELETE or keep as game file (unverified: no src grep hit for "changelog.txt"; data/ is probably upstream-pulled) | 0.1.. history | probably none |
| doxygen_doc/ | 2 | 91 KB | DELETE | Doxyfile+pages.h, only mentioned in matrix.yml paths_ignore | CI path filters only |

## Link integrity

- 514 relative md links checked (docs/, root md, .github), 91 not resolvable. Most are lume-site extensionless links (`../reference/tooling`, `mod/json/tutorial/modding`) that resolve on the built site, not on disk: false positives for GitHub rendering but broken in raw repo view.
- Genuinely broken today: README.ja/ko -> docs/en/dev/guides/building/makefile.md (file missing); docs/en/dev/guides/tracy.md -> `../../../../../assets/img/tracy/*.png` (no assets/); de/ru translations link to pages that do not exist in those languages (15 each).
- Inbound links that a trim would break: AGENTS.md (code_style, r3d_review, changelog_guidelines, cmake, formatting, docs/en/dev/, llm_review_guide, pull_request_template); BACKLOG.md (5 research docs, 4 guides, tools/r3d_spike/*); README.md (4 guides, cmake/msys/vs_vcpkg, contribute/docs.md, 4 contribute/img PNGs); src/onboarding.cpp:199, onboarding.h:11, onboarding_core.h:14 (guided_onboarding.md); CMakeLists.txt:641 (cmake.md); tools/r3d_spike/fixtures/README.md (backlog, renderer_3d.md); tools/ui_redesign_docs/build_html.py (research path, WIP); .github/pull_request_template.md and labeler.yml (`docs/**` generic, harmless).

## Code/build coupling

- Generated, not tracked: docs/en/dev/reference/cli_options.md and docs/en/mod/lua/reference/lua.md (+ lua_annotations.lua) via root `deno task docs:gen` (deno.jsonc:7-13). No test in tests/ compares them (grep: no hits). AGENTS.md says to regenerate on conflicts. Removing docs/en/mod/lua/reference/ (only `.gitkeep`) makes gen fail unless dir is recreated.
- scripts/docs_translation_warning*.ts and git_changed_files_test.ts use docs/en paths as test fixtures only (strings); they belong to the docs-translation CI and can go with it.
- .github/workflows/docs.yml (lume deploy), build.yml:413-423 (docs summary), matrix.yml paths_ignore reference docs/doxygen_doc. Fork notes say no CI runs here, so these are dead weight.
- root deno.jsonc fmt include has "docs".

## lang/

- lang/po is empty in git (only `_DO_NOT_EDIT...` marker); `/lang/po/*.po` is gitignored (.gitignore:123), so the 3D fork tracks zero translations. lang/ is 15 files, 77 KB of POT-extraction scripts (extract_json_strings.py, update_pot.sh, merge_po.sh, compile_mo.sh, etc.) plus lang/CMakeLists.txt, which root CMakeLists.txt:800 includes; it needs msgfmt unless `-DLANGUAGES=none`. Runtime loads compiled lang/mo (not present locally).
- Upstream has the identical lang/ file list (verified via gh listing). Recommendation: translations should come from upstream (Transifex-fed .po / built .mo). Keep lang/CMakeLists.txt (+ compile_mo.sh) so builds can compile pulled .po; extraction/merge scripts are upstream's job. Fork-added strings (3D UI) would need upstream-side extraction or a small fork extraction step (owner decision). AGENTS.md has i18n/PO rules (ko.po, tools/check_po_printf_format.py) which assume local PO files: rewrite or remove if lang is trimmed.

## Proposed target docs/ layout (in-repo, ~25 files)

```
README.md, AGENTS.md, BACKLOG.md, CODE_OF_CONDUCT.md, LICENSE
docs/en/dev/guides/building/cmake.md (+msys.md, vs_vcpkg.md if supported)
docs/en/dev/guides/{formatting,testing,ranges}.md
docs/en/dev/guides/{renderer_3d,first_person,ui_overhaul,guided_onboarding}.md
docs/en/dev/explanation/code_style.md
docs/en/contribute/changelog_guidelines.md  (or link to upstream URL)
docs/en/contribute/img/readme-bug*.png (4) only if README keeps them
.github/{pull_request_template.md,llm_review_guide.md,ISSUE_TEMPLATE/}
```
Wiki: install, new_player_guide, first_day, tutorials, lore (if the 3D fork wants player docs), building extras (cygwin, WSL), tracy, FAQ.
Branch `docs-archive`: docs/en/dev/research/*, docs/en/dev/design/vehicle_tiles.md, ko/ja/ru/de trees (or just delete: upstream has them), docs site tooling (_config.ts, plugins, tools). Do this by branching before deleting so history stays reachable.

## Totals

Tracked docs/ 519 files 11.18 MB. Delete as upstream-duplicate: ko 148 + ja 145 + ru 17 + de 17 = 327 files, 5.59 MB; en mod/json+lua+i18n+contribute(non-needed)+game changelog = ~125 files, ~5 MB. Remaining target: ~25 files, <0.3 MB. Also removable: README.ja/ko, doxygen_doc (2 files), lang extraction scripts (~12 files).

## Owner questions

1. Is a docs site (lume/GitHub Pages) wanted at all? If not, the docs tooling, docs.yml and root `docs` deno tasks go.
2. Should `docs:gen:lua` / `docs:gen:cli` outputs still be produced in-repo (needed for Lua binding work per AGENTS.md)? If yes keep docs/en/mod/lua/reference/ and docs/en/dev/reference/ dirs.
3. Link strategy for BACKLOG.md/AGENTS.md after research moves: absolute URLs to the archive branch?
4. Player-facing docs: wiki, or just point at upstream?
5. Where do fork-added translatable strings get extracted/translated if lang/ scripts are dropped?
6. data/changelog.txt and data/credits: tracked here, presumably duplicates of upstream data pulled in; confirm they will not be pulled from upstream (unverified).
7. SIGNOFF / CODE_OF_CONDUCT: keep?
8. README.md size (15 KB) mixes fork pitch and upstream boilerplate (bug-report screenshots, Discord): rewrite?

Unverified: content equality with upstream files (only paths compared); sizes of dev/explanation and dev/reference subtrees; whether data/changelog.txt/credits are read by code.
