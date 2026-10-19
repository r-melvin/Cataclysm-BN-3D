# Cataclysm: Bright Nights - Agent Guidelines

## HARD CONSTRAINTS (NEVER VIOLATE)

Before writing **ANY** code, verify:

| ❌ VIOLATION                           | ✅ REQUIRED                                                                      |
| -------------------------------------- | -------------------------------------------------------------------------------- |
| manual iterator loops (`it++`, `++it`) | `std::ranges::*`, `collection \| std::views::*`, or range-based `for` if clearer |
| `int foo()`                            | `auto foo() -> int`                                                              |
| `Type x = value`                       | `auto x = value`                                                                 |
| `void fn(a, b, c, d, e)`               | `void fn(options_struct)`                                                        |
| `[](){\n return 1; \n }`               | `[](){ return 1; }`                                                              |

**Prefer `std::ranges`/`std::views`/`std::ranges::to`/cata_algo.h for collection work. Avoid manual iterator increment loops unless required by mutation semantics.**

- Ranges preference means composable iterator/view pipelines. Do not replace a clear counted/nested loop with `views::iota` + `ranges::for_each` just to avoid loop syntax; use a plain `for` when index access, early break, mutation, or nested traversal is clearer.
- prefer function-local `using namespace std::views;` and use `transform`/`filter` unqualified.
- prefer function-local `namespace ranges = std::ranges;` and use `ranges::*` without `std::`
- prefer method/function references over lambdas whenever possible, e.g. `transform( &vpart_position::part_index )` instead of `transform( []( const auto &vp ) { return vp.part_index(); } )`.

## Coding Convention

```c++
const auto foo = 3; //< **MUST** use `auto` for type. `const` **MUST** come before `auto`.

auto bar() -> int; //< **MUST** use trailing return types.
using my_callback_t = std::function<auto( int ) -> bool>; //< **MUST** use trailing return types in type aliases.
auto baz() -> int&; // *NOPAD*  //< **MUST** append `// *NOPAD*` for references/pointer returns to prevent astyle bugs.
auto qux() -> int { return 42; } //< **MUST** use single-line functions whenever possible.

auto qux = my_struct{ .a = 1, .b = 2 }; //< **MUST** use designated initializers.
auto two_value() -> my_data; //< **MUST NOT** use `std::pair`/`std::tuple` for multiple return values. Create a struct instead.
auto may_have_value() -> std::optional<int>; //< **MUST** use `std::optional` for functions that may not return a value.
auto may_fail() -> std::expected<int, std::string>; //< **MUST** use `std::expected` for functions that may fail.

/// **MUST** use triple slash for doc comments like rust's.
/// **MUST** use snake_case for functions and variables.
struct comparable {
  int x;
  int y;
  auto operator<=>( const comparable & ) const = default; // *NOPAD* //< **MUST** use `<=>` for comparisons and append `// *NOPAD*` at the end to prevent astyle bugs.
}

auto values = xs
  | std::views::filter( []( const auto & v ) { return v.is_valid(); } ) //< **MUST** use single line expression if it's single line expression
  | std::views::transform( []( const auto & v ) { return v.get_value(); } ) //< **SHOULD** use `auto` for lambda params
  | std::ranges::to<std::vector>(); //< **MUST** use `std::ranges` over for loops for collections.

namespace { // **MUST** use anonymous namespace for internal linkage over `static`.

// **MUST** use options struct for functions with >3 parameters
struct button_options {
  point pos;
  std::string text;
  nc_color fg = c_white;
  nc_color bg = c_black;
  bool enabled = true;
};
auto print_button( const catacurses::window &w, const button_options &opts ) -> void;

} // namespace
```

- **SHOULD NOT** modify existing headers with >10 usages. Create new header with pure functions.
- **MUST** use modern C++23 features.
- **MUST** preserve unused parameter names as comments instead of deleting them, e.g. `bool /*is_avatar*/` not `bool`; applies to functions and lambdas.
- **MUST** keep Lua function parameters typed with EmmyLua/LuaLS annotations, including existing and local helper functions: `---@param` and table `---@class`/`---@field` shapes where parameters are tables. Do not require or add `---@return` solely for annotation enforcement when the return type is inferable. Before touching Lua, inspect the file's annotation style and preserve complete function typing.
- **MUST** fix missing Lua binding type declarations at the binding/doc-generation source; do not hard-code generated binding classes in `data/raw/generate_types.lua` as a shortcut.
- **MUST** test C++ Lua binding behavior with real bound objects when adding or changing bindings; Lua-only mocks may supplement but must not be the sole validation for binding correctness.
- **MUST** use options struct for functions with more than 3 parameters. Use designated initializers at call sites.
- **MUST NOT** manually write an options/struct type at a call site when the function parameter type makes it inferable; use `{ .field = value }` instead of `options_type{ .field = value }`.
- **SHOULD** search for existing solution because it's a large, legacy codebase.
- **MUST** verify helper-specific matching semantics before relying on string prefixes. For overmap terrain `OtMatchType.PREFIX` / `is_ot_match`, pass the base token without a trailing separator, e.g. `"robofachq"`, because the matcher itself requires the following character to be `_`.

## Workflow

### WHEN given a link to an issue

- **Context**: Fetch issue details via GitHub MCP.
- **Branch**: Use `coderabbitai/git-worktree-runner` to create branch: `git gtr new <type>/<issue-id>/<issue-slug>`
  - type MUST be one of: `feat`, `fix`, `refactor`, `chore`, `build`, `ci`
- **Code**: Refer to [code changes](#when-working-on-code-changes).
- **PR**: Use [Template](./.github/pull_request_template.md). **DO NOT ADD fluff**. create via `git push && gh pr create --web --fill`.
- After opening or updating a Cataclysm-BN PR, track `gh pr checks` until CI finishes or a concrete blocker is identified; inspect failing job logs, fix branch-owned failures, commit, and push before finalizing. For transient or infrastructure failures, rerun when permitted or report the exact failing job and evidence.
- Before running broad formatter targets, prefer file-scoped formatting for touched files when available; if only a broad target exists, inspect and revert unrelated formatter-only changes before continuing.

### WHEN working on code changes

- **Style**: Follow [Code Style](./docs/en/dev/explanation/code_style.md). Use `_( "text" )` for L10n.
- **Format**: Format code before building/testing.

```sh
# Format C++ code
cmake --build build --target format
# Format JSON files
cmake --build build --target style-json-parallel
# Format scripts
deno fmt
deno task dprint fmt
```

- **astyle version (all agents, including Codex)**: astyle 3.6.x mis-indents trailing-return function bodies. Use `bash build-scripts/format-cpp.sh <files>`: the helper and CMake format targets select native astyle 3.4.7 or the pinned `astyle-py==1.1.0` wrapper; nested C++ and tests retain clang-format. Install uv or the Python package for the fallback. Never run incompatible system astyle directly or commit a diff that moves function bodies to column 0.
- **Raw strings**: use the shared Python 3 wrapper even with native astyle 3.4.7. The formatter can reindent raw literal contents; the wrapper protects their tokens so embedded help, Lua, JSON and art retain their values. Inspect and revert unrelated formatter-only changes in legacy files.
- **GPU validation**: HLSL changes require shadercross recompilation; the precompiled fallback does not rebuild changed sources. For SPIR-V builds, install SPIRV-Tools and build `validate-shaders`. Preserve CPU/shader uniform offsets without relying on relaxed block layout. Run game smoke tests in a virtual window with private saves/configuration, as requested by the user.
- **Local build quirks**: if `mold` is missing, configure with `cmake --preset linux-full -DLINKER=lld` (re-running the preset without it resets the linker). If SDL_GPU device creation fails in tests, run them with `CATA_TEST_COMPUTE_ACCELERATION=cpu`.
- **No CI on this fork**: GitHub Actions does not run here. Before pushing renderer changes, run `CATA_TEST_COMPUTE_ACCELERATION=cpu ./out/build/linux-full/tests/cata_test-tiles "[r3d]"` locally and report the result; do not wait on `gh pr checks`.
- **r3d geometry snapshot**: `tests/r3d_geometry_snapshot_test.cpp` fingerprints seeded vehicle and item meshes against `tests/data/r3d_geometry_snapshot.txt`. A failure means renderer geometry changed: for a refactor that is a regression; for an intended change, regenerate with `R3D_UPDATE_GEOMETRY_SNAPSHOT=1` and name the changed scenes in the commit.
- **3D renderer findings**: see [r3d review and status](./docs/en/dev/research/r3d_review_2026_10.md) for known renderer defects, deferred work and `vehicle_profile` quirks before changing `src/r3d_*`.

- **Verify**: Build and fix any issues. Do not skip the game binary target when validating code changes; build `cataclysm-bn-tiles` together with tests.

```sh
# Build project and tests
cmake --preset linux-full
cmake --build --preset linux-full --target cataclysm-bn-tiles cata_test-tiles
```

- **Test**: Create/update relevant `tests/` (Catch2).

```sh
# Run Tests
./out/build/linux-full/tests/cata_test-tiles "[optional-filter]"

# Validate JSON
./build-scripts/lint-json.sh

# Check Mods (validates mod JSON files)
./out/build/linux-full/cataclysm-bn-tiles --check-mods

# Generate Lua Documentation (if conflicts with lua_annotations.lua or docs/en/mod/lua/reference/lua.md)
deno task docs:gen
```

- **Commit**: Commit **ATOMICALLY**. **MUST** Follow [Conventional Commits](./docs/en/contribute/changelog_guidelines.md). **MUST NOT** add body/footer unless critical.

## WHEN working on i18n / PO context

- **MUST NOT** reduce requested string/context coverage for review risk or churn. If the user names a word and its meanings, handle every named meaning.
- If adding JSON context requires loader support, add loader support instead of leaving a source uncontexted.
- **MUST** run `msgfmt -f -c -o /tmp/ko.mo lang/po/ko.po` after touching Korean PO files and fix reported errors before PR.
- **MUST** run `./tools/check_po_printf_format.py` after touching PO files and fix reported errors before PR.
- Do not call PO/printf errors pre-existing to skip them when the task touches that locale or validation path.
- If a mistake is found during the task, update AGENTS/skill immediately and fix the current branch before summarizing.

## WHEN translating docs

When translating, MUST search for correct glossary, e.g

```sh
rg -C2 -i '<<TARGET>>' lang/po/<<LANG>>.po | rg -v '^(#:|--)' | head -n 20
rg -C2 -i 'speedway' lang/po/ko.po | rg -v '^(#:|--)' | head -n 20
```

## References

- **Docs**: [Building](./docs/en/dev/guides/building/cmake.md), [Formatting](./docs/en/dev/guides/formatting.md), [Dev Index](./docs/en/dev/).
- **Project backlog**: [Backlog](./BACKLOG.md).
- **Outstanding-work tracking**: `BACKLOG.md` is the single current task list. Research/review documents
  retain historical findings and rationale; link to the backlog rather than maintaining parallel open
  lists. Update the backlog when selecting, completing or deferring work, and verify old findings against
  current source before reintroducing them.
- **Review**: [LLM Guide](./.github/llm_review_guide.md).

- When fixing a bug, preserve requested behavior and visible content unless the user explicitly asks to remove it; fix the underlying issue instead of suppressing the affected feature.
- UI regression tests must follow the actual refresh path; do not manually prepare layout or paging after an action unless production does so, since that can hide stale display ordering.
- When reviewing PRs that stop tracking generated or externally pulled files, verify ignore rules by running the generator/pull command or checking `git status --ignored`; do not assume removed tracked files are ignored.
- When generated or externally pulled files are removed from tracking, verify all CI and release consumers still receive required files or directories.

- When splitting or importing tests, carry their fixture/helper declarations with them. Automated `auto` style edits must preserve typed function parameters, especially defaulted parameters; deduced defaults cannot infer omitted template arguments.

- After adding a new test source, reconfigure CMake: the test-source glob is not watched. Verify the new cases appear with `--list-test-names-only` before counting their assertions as validation.
- Test-only inspection of selected saved JSON fields must call `allow_omitted_members()` on each partially inspected object. Keep production deserialization strict; unexpected debug-log errors still fail validation.
- When removing a profile/schema field, search all renderer, gallery, documentation and test consumers for remaining references before rebuilding.
- Window presentation must cover native broken/empty frames as well as `WINDOW`-flagged terrain. These frames deliberately lack that gameplay flag; preserve native flags and validate the actual open/broken terrain IDs.
- Lua fixture tools must use verified binding names rather than assuming C++ methods are exported. Preflight all map bounds before mutating any fixture tile. Paste fixture source into the sandboxed console; `dofile` is unavailable.
- A private `--datadir` also relocates tileset discovery to `<datadir>/gfx/`; provide that directory (or a symlink to the installed `gfx`) when testing private renderer assets.
- Before pasting a Lua fixture, inspect the console/editor UI after startup; queued startup input can miss the console and send pasted source to ordinary gameplay bindings.
- Renderer texture checks must include missing-art coverage on a smaller bundled tileset (for example Hoder's `t_wall_w`), alongside Ultica; valid unknown sprites must not be mistaken for wall elevations/materials.
- Native multitile edge rotation 1 is east-west; rotation 0 is north-south. Wall elevations must use the horizontal face art, verified against both the bound source sheet and crop.
- Warning cleanup edits must be scoped to the compiler-reported function, not the first matching declaration in a file; repeated local names and parameters can be used in neighboring functions.

- Terrain/furniture JSON symbols accept one byte or the named line symbols, not arbitrary UTF-8 or embedded NUL characters. Validate new renderer fixtures through native data loading before launching broad tests. Pin a real-art tileset for solid-prop tests; missing-art glyphs should remain readable upright markers.
- Renderer tile caches include subtile and rotation in their keys. Mesh tests should compare the bound art and behavior rather than require pointer identity across differently oriented cache entries.
- Automated Lua-console input can coalesce Return events. Preserve statement boundaries explicitly (or verify editor line breaks) before executing a pasted fixture.
- Vehicle sprite footprint checks must include oversized/tall art (for example the 64-pixel hatch). Convert opacity fractions with the sprite size and anchor offsets before treating them as native tile-space geometry.

- Camera mouse events must not render within `SDL_PollEvent` draining. Apply all deltas, redraw once per batch, and finish through `refresh_display()` before another camera redraw. Validate captured motion and released middle-button/Shift dragging in a private virtual game; geometry fingerprints cannot detect GPU command accumulation.
