# Project backlog

This is the single source of truth for outstanding fork work. Research documents
preserve evidence and rationale; their original recommendations are not separate
task lists. Update this file when work is completed, selected or deferred.
Listing an idea here does not authorize implementing it automatically.

Audited 2026-10-19 against `main` at `7767e98`, documentation since the initial BN
renderer port (`f81afc9`), and the three feature-branch guides. The separate asset
pipeline was checked at `b4e0493`.

## R3D-01: Asset loading and reload safety

**Status: implemented; broader GPU acceptance remains in R3D-07.** Checked,
bounded reads and per-file exception isolation preserve valid neighbors. A
cancellable CPU worker prepares profiles/models; the render thread uploads one
palette per frame and redraws idle gameplay without spending turns. Tileset
reload and device shutdown discard superseded work and release old resources.
Capture waits for settled assets and flushes logs before one-shot exit.

Validation: 103 renderer tests (28,286 assertions), nine capture-tool tests,
and an Xvfb/software-Vulkan one-shot comparison. The live log confirms an invalid
VOX is skipped while two vehicle models and one item model become ready. Tests
cover failed reads, valid neighbors, profile reload, cancellation and stale
results across device/reload requests. Native GPU recreation and the complete
visual matrix remain acceptance work, not claims established by these tests.
Implementation: `src/r3d_asset_loader.cpp` and `src/r3d.cpp`.

## R3D-02: Pin the working formatter in shared tooling

**Status: delivered.** Shared formatting accepts native astyle 3.4.7, otherwise
selects `astyle-py==1.1.0` through uvx or the active Python environment. The wrapper
pins the 3.4.7 engine and reports setup failures; incompatible system versions
are never used. Nested C++ and tests retain clang-format. Verified routing,
trailing-return indentation and repeated-format idempotence on representative
files, plus the missing-package diagnostic. No paid CI was added.
Both native and packaged engines use Python 3 protection for raw literal
tokens, preventing reindentation from changing embedded help, Lua, JSON or art.
Five tokenizer regressions pass; real formatter CLI output is idempotent and
compiled C++ emits the same help text before and after formatting (`69fd98c`).

Compiler warning cleanup (2026-10-19): the affected game/test translation units
build without warning suppression. Aggregate defaults, unused scaffolding and
parameters, redundant return qualifiers, explicit nested option initializers and
exception-handler order are corrected. Both targets build; 339 selected renderer,
onboarding, Lua, activity, magic, enchantment, reload, aiming and corpse cases
pass (56,152 assertions). This is evidence for the affected rebuild, not a claim
that every untouched upstream translation unit is warning-free.

Follow-up after rebasing `2f73690`: the newly reported warnings are cleared
without disabling compiler diagnostics. Constructor initializers follow member
order; unused spell/editor/vehicle-mapgen scaffolding and disabled stair-link
helpers are removed. The map-cache query now returns false for an out-of-range
level rather than falling off a non-void function. Both binaries build without
warnings in all nine affected translation units. The combined renderer,
onboarding, input, map/mapgen, vision, inventory, magic and recipe run passes
275 of 279 cases (60,523 passing assertions). Four vision cases fail 14 assertions;
rebuilding the pre-cleanup source and running the same command reproduces all
four with identical complete diagnostics (274 of 278 cases pass). These are
existing vision failures, not acceptance established by this cleanup. Evidence:
`out/validation/warning-{cleanup,baseline}-suite.log`; the bounds regression also
runs independently. No full-project clean-build claim is implied.

Sources: [agent guidelines](AGENTS.md),
[wrapper](tools/r3d_spike/astyle_347.py).

## R3D-03: Complete the presentation-profile shape and material contract

**Status: partially implemented; broader shape/material contract remains open.**
Size, support-anchor and depth profiles work. Solid walls and window/door surrounds support
per-tileset side/top donors and normalized source crops, with exact sprite
identity checks, preserved source-sheet ownership, fallback and profile-triggered
cache/chunk invalidation. The renderer guide documents the opt-in format and
comparison galleries list donor identities and crops. Both binaries build;
154 combined renderer/onboarding tests pass (37,051 assertions), including real
wall mesh sheet/crop checks, invalid donors and profile removal. All 14 tooling
tests pass. A private Xvfb/software-Vulkan profile-install smoke captures a frame
and exits 0 without validation errors (`/tmp/bn-face-profile-smoke.*`).
Material regions are still gallery annotations, not runtime palette mappings.
Opening overrides preserve their pane/leaf art and geometry and apply rotated
crops to sill, lintel and jamb sections. Native broken/empty window frames now
remain upright without adding gameplay flags. Tests cover eight opening states,
both axes and both wall thicknesses, including sheet/UV identity and unchanged
profiled geometry. Furniture box/slab faces now share the donor resolver:
`top`, non-front `side`, and explicit `front` overrides preserve native handles
and doors when no front donor is supplied. Footprints, row joins, wall fitting,
slabs, feet and undersides retain their native structure. Box/slab furniture now
consumes local width/depth/height and support anchors: exact authored spans rotate
and mirror with the piece, translate flush to thin walls, and update cached top
heights. Omitted dimensions preserve native row/depth fitting. Arbitrary geometry
families and upright/panel/natural-block profiles remain outstanding. Both binaries build; the combined renderer/onboarding run
passes 158 cases (37,778 assertions), including per-face donor checks for
lockers, chairs, bins and tables. A private Xvfb/Vulkan scene confirms upright
broken/empty windows and the open door leaf; a reusable native Lua scene builder
and instructions now live in `tools/r3d_spike/fixtures/`. The saved private scene
reloads and exits normally (0), with turn 1,327,697 and moves 100 unchanged and
no Vulkan validation errors. Native 2D and 3D references were inspected. Geometry-family overrides and equivalent upright/panel furniture and vehicle
face consumers from the original proposal remain outstanding. Furniture-dimension
validation builds both targets and passes 161 renderer/onboarding cases (53,088
assertions), including real emitted bounds, narrow legs, rotated/mirrored anchors,
wall contacts and profile removal. `fixtures/furniture.lua` supplies isolated,
joined and wall-backed native props for visual review. A private Xvfb/software-
Vulkan run executes that script with authored profiles, rotates the view, saves
and exits 0; turn 1,327,697 and moves 100 are unchanged. Evidence is under
`/tmp/bn-furniture-acceptance/`. Hoder chair transparency still limits inferred
solid materials; this check does not establish the complete tileset/art matrix.

Define those fields for a bin, chair, locker, window, door, tree, loose item and
vehicle corner. Bind sprite crops to a tileset/sprite identity; cover footprint,
face materials, anchors and fallback without changing gameplay flags. Validate
resolution, crop bounds, source texture sheet and missing/invalid overrides.
Do not restore unused runtime metadata without a rendering consumer.

Sources: [Dramatic Shape review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md),
[vehicle research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/vehicle_rendering_research.md).

Wall texture regressions: the elevation path now selects the east-west edge
(rotation 1), rather than stretching the north-south cut edge into vertical stripes.
Hoder also lacks native `t_wall_w` art. The renderer now
uses the wall atlas instead of treating the unknown glyph as an elevation, and
uses generic wall art if the atlas is absent. Ultica remains the recommended
selected tileset; enabling 3D preserves existing selections. The native missing-
art test reproduced five failing assertions before the fix. Both binaries build;
163 renderer/onboarding tests pass (53,171 assertions). A private Hoder virtual
run confirms solid wall faces replace the magenta pattern; evidence is under
`/tmp/bn-wall-regression/`. A second private Ultica run shows the intended grey
wall elevation (`/tmp/bn-wall-ultica/after.png`). The shelter follow-up now uses horizontal seat art for every intact bench row,
removing black patches caused by sampling vertical top-down variants. Native
flipped benches have closed overturned-seat geometry; intact neighbours join
without spanning the overturned state. Wreckage is a low pile of intersecting
metal pieces. Actual stair flights use closed treads spanning one storey, with
continuous segment heights across submap boundaries; ladders and hatches retain
their separate presentation. Locker depth is consistently half a tile, with
plain corner infill instead of stretching the door face. The earlier talking-doll
scale and connected trashcan profiles remain in place.

Missing-art furniture and items stay visible as upright glyph/category markers;
they are not extruded into question-mark cubes. Solid vehicle faces now sample
an opaque texel from the same part sprite, avoiding transparent material holes.
Both binaries build without warnings in the affected rebuild. The local
renderer/onboarding suite passes 172 cases / 54,739 assertions. Native fixtures
cover bench seams, flipped state, stair continuity, locker dimensions, marker
retention and the source sheet/opacity of vehicle materials. A software-Vulkan
Xvfb comparison confirms the shelter fixture in both renderers; evidence is in
ignored `out/validation/dan-recovery/shelter_{2d,3d}.png` and test logs in
`out/validation/`. The desktop restart removed the earlier `/tmp` world snapshot;
Dan's last alive character checkpoint was recovered from the native graveyard
into `out/validation/dan-recovery/character-checkpoint/`, but current Meredith
world files have been regenerated and are not that original world.

The exact outdoor screenshot's vehicles and unknown species still require a
matching saved scene; broader vehicle layer/state presentation and cross-tileset
material acceptance remain in R3D-04/07. These shelter regressions are fixed;
that does not mark the broader presentation-profile contract complete.

## R3D-04: Finish vehicle layer, material and state presentation

**Status: implementation/visual validation; depends on R3D-03/07.** Role lists,
stock-door classification, deterministic panel corners, faceted/authored wheels,
roof/body heights and the shared windshield path exist. Geometry still relies
heavily on one main sprite and generic surfaces.

Resolve remaining installed layers into explicit paint/body, glass, rubber,
trim, light, interior and roof contributions. Extend reusable frame, panel,
sill, hood/bumper, roof and window templates where the gallery demonstrates a
need; frame-only mounts should not read as solid body cubes. Verify vehicle
door hinges/swings and intact/open/broken/removed/folded/accessory states;
terrain-door tests do not establish vehicle-door correctness. Preserve arbitrary
assembly, pivots, paint, damage, interactions and attachment seams.

The closed [hatchback report #6](https://github.com/r-melvin/Cataclysm-BN-3D/issues/6)
adds a concrete fixture: dark side-door/window faces, an elevated rear hatch and
cargo markers are visible in its screenshot. BN's `car_hatch` has separate
side-door/window cells, four inner roof tiles and rear hatch cargo cells.
Validate the actual installed roles and mesher output before adopting the
attached generated proposal. Preserve per-part paint and cargo information;
a blanket vehicle-wide colour average or deletion of markers would lose them.
No save, logs or capture archive were attached. The reporter closed it as
completed; its visual findings remain part of this presentation work.

Vehicle footprint follow-up: opacity bounds now convert through native sprite
size and anchor offsets before rotation/mirroring. Ultica's oversized hatch art
therefore fills its native tile instead of shrinking adjoining panels. A real
two-hatch assembly reproduced three failed boundary checks before the fix.
The rebased hatch blueprint and bounds fix build together; 180 renderer,
onboarding and input cases pass (54,851 assertions). Geometry snapshots were
regenerated for car, pickup, SUV, bus and semi-truck at 0/90/225 degrees; both
item scenes are unchanged. This establishes native mount bounds, not the full
vehicle appearance/state acceptance matrix. A private software-Vulkan virtual-window capture shows the rebased hatchback
with adjoining body/window panels (`out/validation/dan-recovery/rebased-screen.png`).
The automatic one-shot did not reach its frame threshold while idle; the virtual
window was captured directly. This is one fixture, not the original outdoor save.

Source and acceptance matrix:
[vehicle research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/vehicle_rendering_research.md).

## R3D-05: Expand authored props and run asset-pipeline trials

**Status: authoring/integration follow-up; external tooling exists.** Standard
and wide wheels and the CO2 tank are bundled. Solid seating/natural props,
generic item extrusion and VOX loading/greedy meshing are already implemented.

Next author one useful vehicle panel/fender and representative props such as
bins, shelves, lamps or machinery. Compare with fallbacks at gameplay scale and
eight angles. Keep editable sources, provenance, tool versions/settings,
palette, pivot and scale.

Use the separate [Go asset CLI](https://github.com/r-melvin/Cataclysm-BN-3D-Asset-Pipeline):
its adapted four-view voxelizer outputs VOX directly, so an OBJ importer is not
a prerequisite. Its optional MelonLoader adapter exports existing Kenney Shape
projects. Install [Kenney Shape](https://kenney.nl/tools/kenney-shape) separately;
do not ship its archive/application or couple BN to Unity hooks. Native
Windows/macOS Kenney export is unverified in the tool's README and belongs to
that project's validation.

Procedure and visual gates:
[sprite-to-voxel pipeline](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/sprite_to_voxel_pipeline.md).

## R3D-06: Directional characters with equipment layers

**Status: art/renderer design; depends on R3D-05's four-view trial.** Characters
use layered camera-facing sprites with left/right mirroring. The inspected
built-in art does not supply all four views.

Define travel/facing relative to the camera and provide a representative
left/front/right/back sheet or 3D model. Preserve body, hair, clothing, armor and
wielded layers, with sprite fallback for missing directional assets. Check
walking toward/away/sideways, stopping, camera rotation and mixed equipment.
Mirroring one image cannot synthesize a correct face or back.

Sources: [asset trial](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/sprite_to_voxel_pipeline.md),
[Dramatic Shape review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md).

## R3D-07: Reproducible visual fixtures and GPU acceptance checks

**Status: validation/tooling follow-up.** Live sessions already bundle saves,
logs, source galleries, support markers, 2D and eight 3D views. The state matrix
reports available/observed sprites, not automatic captures of every state.
The 17 seeded geometry snapshots cover positions/light, not UVs, textures,
shaders or the final GPU image.

An opening-state scene builder is now checked in at
[fixtures/openings.lua](tools/r3d_spike/fixtures/openings.lua), with
[instructions](tools/r3d_spike/fixtures/README.md). It constructs eight native
states and was executed in a private virtual game; its saved scene and images
are under `/tmp/bn-opening-acceptance/`. This is one repeatable fixture, not the
complete state/interaction matrix or a bundled user save.

Create saved fixtures for R3D-03's props and a sedan, van, pickup/flatbed,
bicycle/motorcycle and unusual player-built assembly. Cover intact/open/broken/
removed states, roofs/accessories, walls, partial visibility, daylight/darkness,
both shipped tilesets, zoom, pitch and storey scale. Add focused front/side/top
asset views and a labelled human acceptance sheet beside whole-scene captures.

Review window/foot contact shading at chunk seams and avatar silhouettes with
clothing, overhead levels and occluders. Add targeted texture-sheet/UV and
normal-refresh-path tests where useful. Retain real saves/captures for visual
judgment; fingerprints alone do not establish appearance or visibility. Run
locally; paid GitHub CI is explicitly out of scope.

2026-10-19 Linux virtual-window validation delivered these fixes:

- Direct saved-world loading exposed a framebuffer initialization crash, fixed
  in `84fd8ea`; malformed VOX assets also retain valid neighbors.
- Vulkan conformance fixes in `10b9b34` enable required depth clipping and replace
  misaligned vector uniform padding with scalars at the existing CPU offsets.
  All 25 SPIR-V modules validate, all three revised shaders cross-compile to
  MSL/DXIL, and all 104 renderer tests pass with CPU and software GPU compute.
- The eight-angle crash was reproduced on Lavapipe 26.2.4 and official 26.2.5.
  It exhausted the process's 1,048,576 kernel mappings (8.35 GB RSS): capture
  settling queued draws without completing normal presentation, and direct
  `SDL_RenderPresent` failed while a texture target was active. Capture now uses
  BN's real `refresh_display()` path (`7ee9db2`). A complete eight-angle run plus restored
  view held about 39,225 mappings during the sweep and 46,008 after restoration,
  with no standard/synchronization validation errors.
- The supported launcher then completed two eight-angle checkpoints in one
  session, quit normally and produced a 17.3 MB ZIP. Extraction, both SQLite
  saves, galleries, images and logs passed integrity checks. It now passes an
  explicit private `--configdir` on XDG builds; `--configdir` can also select
  source settings outside the source user directory. Backend/font/panel settings
  are preserved per checkpoint. The normal quit sentinel previously returned
  status 25; the corrected menu exit (`923b619`) was verified to return 0.
  Failed explicit world loads retain a nonzero exit status (verified as 1).

Mouse-camera crash follow-up: the native `2f73690` crash log aborts while
acquiring a GPU command buffer from the mouse-event redraw path. Rendering
inside `SDL_PollEvent` queued 19 command buffers before any presentation for a
bounded 16-motion burst. Camera deltas now accumulate through native handling,
with one redraw after event draining and a normal `refresh_display()` completion.
A private Xvfb/software-Vulkan test shows no drawing during event draining;
4,096 captured motions and 2,048 Alt-released middle-button/Shift-pan motions
complete with normal exit. Process mappings stabilize after warm-up rather than
growing per motion. Both binaries build without warnings; 180 renderer,
onboarding and input cases pass (54,851 assertions). Evidence and the native SDL
instrumentation are in ignored `out/validation/mouse*`; the repeatable acceptance
procedure is in the fixture guide. The reported hardware is an RTX 5080; this
validation uses software Vulkan and does not claim a direct NVIDIA reproduction.

These are Linux/software-Vulkan checks, not the complete vehicle/prop fixture
matrix or native Metal/Direct3D acceptance. Current private evidence is in
`/tmp/bn-assets-live/` and `/tmp/bn-capture-e2e-20261019/` (temporary, not durable
shared fixtures). The extracted report is under the latter directory's
`extracted/` subdirectory. Five formatter and nine capture-tool tests pass.

Sources: [comparison guide](tools/r3d_spike/COMPARE.md),
[Dramatic Shape review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md),
[vehicle validation matrix](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/vehicle_rendering_research.md).

## R3D-08: Measure frame cost and decide chunk scheduling

**Status: measurement first, conditional optimization.** Chunk timing/queue
logging exists. Initial visible chunks build synchronously; later rebuilds use
a count budget, not a time allowance. Diagnostic-only mesh, upload, light and frame clocks are now gated by
`CATA_R3D_DEBUG_CHUNKS`; capture and asset deadlines remain independent.
Silhouettes avoid copying the entity view but still build a separate
avatar-only mesh.

A private Xvfb/Lavapipe comparison after gating recorded a 41-chunk cold
scene at 17.0 ms renderer CPU time (10.1 ms meshing, 0.47 ms geometry upload,
1.55 ms lighting); a tileset rebuild was 18.0 ms, and settled samples were
3.9–5.0 ms. These single-scene samples are evidence, not a broad performance
guarantee or GPU execution timings. Both game/test targets build and all 104
renderer tests pass (28,312 assertions). Temporary evidence:
`/tmp/bn-diagnostic-gating/`.

Measure cold start, travel, destruction and tileset changes, including GPU
upload/draw and silhouette cost. Gate diagnostic-only timing as appropriate.
If justified, prioritize nearby visible chunks and bound preparation by time;
keep a safe complete/interim view and redraw while idle. A budget between whole
chunks cannot bound one expensive chunk. Off-thread work needs immutable
snapshots and stale-generation rejection.

Do not reopen vehicle mesh caching by default: the recorded five-vehicle CPU
benchmark was 0.46 ms for vehicles and 0.21 ms for entities. GPU cost remains
unmeasured. Reuse avatar geometry or cache only with evidence of worthwhile
savings and an invalidation plan.

Sources: [diagnostics](docs/en/dev/guides/renderer_3d.md),
[measurements](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/r3d_review_2026_10.md),
[chunk research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md).

## R3D-09: Audit complete-vehicle visibility and knowledge boundaries

**Status: partial correctness fix implemented; acceptance/information policy
still open.** Live vehicle activation now uses BN's `map::pl_sees` perception
cache, rather than presentation RGB. Memory, unexplored-room tints and detectable
bright light cannot activate an otherwise unseen vehicle. Perceived low-light
mounts can activate it independently of tint. Cargo-content markers require
current perception at their own mount. The explicit fullbright debug override
is retained. A real map/vehicle regression test covers these distinctions.

The existing complete rigid-body model and dim unlit mounts are preserved once
a mount is perceived. This does not prove that previously unknown structural
details behind blockers cannot be disclosed. Review that continuity policy
alongside native 2D's live-vehicle-outside-FOV presentation before changing it;
the new activation/content fixes do not close this audit.

Distinguish dark-but-visible, remembered and unknown tiles using BN knowledge
contracts. Check opaque blockers, map edges, partial sight, night vision and
unseen contents with R3D-07 fixtures. Preserve legitimate model continuity
without exposing unavailable information. Do not equate black light with
unknown terrain or silently change game rules.

The seeded combined `[r3d],[vision]` run exposed four native vision failures:
`vision_wall_obstructs_light`, `vision_single_tile_skylight`,
`vision_see_out_of_vehicle`, and `vision_see_into_vehicle`. Rebuilding committed
`698f3c7` without the visibility changes reproduced the same 14 failed assertions
in the same four cases (`--rng-seed 0`). Investigate those native visibility
expectations independently; do not suppress them or change BN perception to
make the renderer test green. Private logs: `/tmp/bn-backlog-visibility-baseline-tests.log`
and `/tmp/bn-backlog-renderer-vision-tests.log` (temporary evidence).

Source: [vehicle visibility research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/vehicle_rendering_research.md).

## R3D-10: Native macOS and Windows validation

**Status: requires platform access.** Linux builds, renderer tests and reports
have local evidence. Native macOS/Windows game execution is unverified.

Build/play on Metal and Direct3D 12 with the required shaders. Check GPU/2D
fallback, scaling/ultrawide layout, controls, options, save/load and capture/
package/restore; repeat relevant checks for integrated feature ports. Cross-builds
and path-resolution tests do not replace native runs.

Sources: [README](README.md), [renderer guide](docs/en/dev/guides/renderer_3d.md).

## R3D-11: Integrate the native UI/HUD branch

**Status: integrated on main; native platform acceptance remains R3D-10.**
The native menu theme, F4 report, HUD and weather/sky effects from `ce6d386`
are integrated with current asset preparation, voxel items, vehicle visibility,
silhouettes, shared dynamic buffers and capture restoration. Weather particles
require native tile perception. HUD shapes/text are clipped to the map, SDL
renderer state is restored, and comparison redraws cannot emit weather notices.
F4 is registered for both native tiles and curses input. Rain/snow and transient
HUD fades request idle redraws; an Xvfb snow trial changed frames without input,
held at 23,554 kernel mappings, and retained the source save's turn 1,327,697
and 100 moves after normal save/quit. Evidence: `/tmp/bn-ui-idle/`.

Validation: both game/test targets build, all 113 renderer tests pass (33,208
assertions), 43 standalone math cases pass, and 14 tooling tests pass. All 25
SPIR-V shaders validate; both revised block shaders cross-compile to MSL/DXIL.
A private Xvfb/Lavapipe run exercised F4, eight camera angles, view restoration,
2D fallback and normal save/quit; it produced an integrity-checked 8.7 MB ZIP
with no Vulkan validation errors. Temporary evidence:
`/tmp/bn-ui-integration-capture/`. These checks do not establish every HUD
condition or native Metal/Direct3D appearance; broader visual fixtures remain
R3D-07. See the [UI guide](docs/en/dev/guides/ui_overhaul.md).

## R3D-12: Integrate the first-person branch

**Status: integrated on main; broader scene acceptance remains R3D-07/10.**
The camera, discrete controls, look/target/direction integration, saved heading,
optional easing, ceilings and heading-up minimap from `ae02e6f` are integrated
with current assets, vehicle geometry, visibility, weather and captures. The
math helpers have separate signed-bearing names, avoiding collisions with the
orbit HUD. All 17 seeded geometry snapshots remain unchanged.

Main’s native render-state guard clips both overlays and restores SDL state.
Easing uses the normal idle presentation path. FP captures share settled asset
loading, normal frame presentation and RAII view restoration; they preserve RNG
and message-read state. Current labels use native perception; remembered labels
use recorded terrain/overlay IDs and honor the memory toggle instead of reading
unseen live furniture. Weather quads use the actual view direction in first
person, and sun/moon sprites remain within its far plane.

Linux validation: both targets and 25 SPIR-V modules build/validate. Both revised
shaders cross-compile to MSL/DXIL. The combined renderer, native map-memory and
seen-cache run passes 143 cases (176,312 assertions), including 136 renderer
cases. The standalone suite passes 64 cases (35,796 assertions). A private
Xvfb/Lavapipe session exercised turning/pitch, look/crosshair selection, threat
facing, menus, camera switching and first-person restoration after the eight
standard orbit poses; normal quit produced an integrity-checked 8.6 MB ZIP.
Turn 1,327,697 and 100 moves were unchanged. Downward first-person captures cover
all eight headings; upward captures exercise the revised particle/sky path.
These are not claims that every indoor, targeting, creature and occluder
combination has received human visual acceptance. Temporary evidence:
`/tmp/bn-first-person-acceptance/`, `/tmp/bn-first-person-shots/`, and
`/tmp/bn-first-person-up/`. See the [first-person guide](docs/en/dev/guides/first_person.md).

## R3D-13: Integrate guided onboarding

Guided Play has its own New Game entry. Ordinary Play Now does not opt into
guided lessons or the starter kit; global hints remain an explicit option.

**Status: integrated on main; broader UI/platform acceptance remains R3D-07/10.**
Lessons, F1 checklist, context-specific F2 explanations, progress persistence,
optional guided start kit and translation extraction are included. Hints-only
mode finishes logged popup lessons, including active popups loaded from saves
or carried across an option change, so the welcome lesson cannot block later
steps. Hints and camera/menu inspection do not spend turns.

Both game and test targets build. The combined onboarding/renderer/memory/aiming
run passes 165 cases and 176,496 assertions. Native adapter tests cover option modes,
start-kit opt-out, save/reset behavior and turn/move preservation. Translation
extraction includes all 168 unique lesson, screen and conditional-variant strings.
An Xvfb/software-Vulkan private old-save check exercised the checklist and main
and inventory explanations, saved progress and reloaded it. Game turn and moves
remained unchanged. The final reload shows welcome completed and movement active.
Temporary evidence: `/tmp/bn-onboarding-acceptance/`.
The complete new-character/Tutorial Game and screen-layout matrix remains
R3D-07; native macOS/Windows remains R3D-10. Tutorial hints retain their explicit
native game-type/mod exclusion; this smoke test does not establish that whole flow.

Guide: [guided onboarding](docs/en/dev/guides/guided_onboarding.md).

## R3D-14: Fog of war and a time-of-day sky

**Status: existing effects evaluated and integrated through R3D-11; broader
scene acceptance remains R3D-07.**
Distinguish unseen terrain from visible darkness and replace the uniformly black
exposed background with a game-time/weather-driven sky or horizon. The integrated UI implementation provides calendar/weather-driven sky colour,
sun/moon sprites, visible outdoor precipitation and distance fog. Its fog is
a presentation effect: native BN visibility still controls what may appear.
Unknown/memorized geometry retains the existing knowledge treatment; it does
not become visible merely through fog. Evaluate these effects with the saved
fixture matrix before adding another weather renderer. Cover indoor/underground scenes, weather,
cutaways, first person and remembered terrain. Keep BN lighting/knowledge
in control. See the [Dramatic Shape constraints](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md)
and R3D-11's branch guide.

## R3D-15: Improve turn-based driving presentation

**Status: research/prototype; preserve discrete simulation.** Instrument redraws
during vehicle substeps; evaluate renderer-only interpolation of committed
position/heading and camera target. Snap on collisions, map shifts, portals,
save/load and discontinuities. Review `VEHICLE_CONTROL_SCALE` and turn-based
control affordances if necessary. Real-time physics/input remains a final resort
requiring a separate decision on scheduling, fairness, action costs and collisions.

Relevant paths: `src/vehicle/vehicle_move.cpp`, `src/vehicle/vehicle.cpp`,
`src/map/map.cpp`, `src/game.cpp`, and `src/r3d.cpp`.

## R3D-16: Multi-level vehicles

**Status: explicitly deferred.** Mounts have z coordinates, but cells use mount
x/y and one vehicle-wide base height. Key by full mount, offset geometry by
mount z, and make wall/trim/bumper/roof neighbors level-aware. Verify parts at
the same x/y on different levels never overwrite each other.

Source: [renderer review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/r3d_review_2026_10.md).

## R3D-17: Vehicle-free play mode

**Status: explicitly parked.** An opt-in mode would remove vehicles as playable
content, not merely their models. Define spawns, construction, parts, driving/
interaction and existing-save behavior first. Establish data/Lua versus core
hook coverage; a partial mod must not imply every vehicle activity is disabled.
Keep default BN and 2D behavior unchanged.

## R3D-18: Shadow maps, reflections and tilt-shift

**Status: deliberately deferred.** Revisit after core presentation/validation.
Shadow/reflection passes must respect BN visibility and not introduce gameplay
light. R3D-12 covers camera-only first person; these effects imply no real-time
movement. Source: [Dramatic Shape review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md).

## UI-01: One guided start (tutorial as chapter one)

**Status: decided, not started.** Combine the built-in Tutorial Game and Guided Play
into one path, with guided play as the baseline.

Decisions (owner, 2026-10-21):
- The Tutorial becomes chapter one of the guided start; the separate Tutorial and
  Guided Play New Game entries go away. The same character continues into the real
  world with the guided lessons.
- Guided play opens on the first New Game. A dialog offers to turn it off; once off it
  stays off for later New Games (stored with the profile, not the character).

Known obstacles to settle before building:
- The tutorial is a separate special world (`TUTORIAL`, `special_game_type::TUTORIAL`,
  `gamemode_tutorial.cpp`). A character cannot move between worlds, so chapter one
  must run inside the player's own world, e.g. the tutorial complex as a start location
  or overmap special, with `g->gamemode` active only until it is finished. Check how
  much of `tutorial_game` depends on owning the whole world.
- Once turned off there must be a way back on: keep one replay route (an option or a
  single New Game toggle) so removing the entries does not remove the feature.
- The tutorial's lessons (`tut_lesson`) and the guided lessons (`onboarding_core`,
  `data/json/onboarding/`) overlap; merge them into the JSON lesson set rather than
  running two systems. Existing guided progress saves must keep loading.
- Translations of the removed menu entries and lesson text change.
- Coordinate with the UI redesign (`docs-archive:docs/en/dev/research/ui_redesign/`) for the checklist
  and hint presentation.

## UI-02: Retire the curses interface

**Status: decided, not started.** Owner decision (2026-10-21): retire the curses build and
the character-cell drawing model (`CURSES` option, `ncurses_def.cpp`, `wincurse.cpp`,
`cursesport.cpp` as the screen model). The new UI (`docs-archive:docs/en/dev/research/ui_redesign/`)
needs no character-cell backend. Open to moving to a different 3D frontend if that
improves accessibility; not yet evaluated.

Consequences to cost before starting:
- Screen-reader support can no longer lean on a terminal; the self-voicing/accessibility
  tree work in the redesign plan becomes the only path.
- Every screen is still written against the cell grid (`cursesport`); retirement happens
  screen by screen as the new widget layer replaces them, not as one deletion.
- Fork-only decision: upstream keeps curses, so expect merge friction in the touched files.

## UI-03: Frontend decision and repo scope

**Status: decided (owner, 2026-10-21).** Recorded here so the UI redesign and the repo trim
share one set of constraints.
- Stay on SDL3 + SDL_GPU. The M1 gate in `docs-archive:docs/en/dev/research/ui_redesign/` (an `accesskit-c`
  SDL spike) decides whether to fall back to Godot; the fallback conditions in 06_frontend_options
  are accepted.
- Keep Windows, macOS and Android build support (their packaging files stay).
- Keep deno and the docs site: dropping them was tried (PR #7) and reverted as more effort than it is worth.
- Keep translations, tilesets and sound. Research docs go to an archive branch and player guides
  to the wiki.
- Upstream merges take game logic only, not renderer work: see
  docs/en/dev/guides/upstream_sync.md and `tools/upstream_sync/check.sh`.

## Audit references and completed foundations

- [Renderer review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/r3d_review_2026_10.md): wheel/depth/roof/
  shared-edge/cargo-floor/lighting/camera/classification fixes are delivered;
  billboard/material/grid dead code and windshield duplication are removed.
  Asset loading and shared formatter changes are recorded in R3D-01/02 above;
  remaining acceptance and findings map to R3D-07/08/09/16.
- [Dramatic Shape review](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/dramatic_shape_voxel_review.md):
  size/anchor/depth profiles, galleries/support markers, local contact shading,
  chunk diagnostics and avatar-only silhouettes exist. Extensions/acceptance
  map to R3D-03/05/06/07/08/18.
- [Vehicle research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/vehicle_rendering_research.md): role
  lists, shaped panels, faceted wheels and bundled wheel models supersede its
  no-layer-list/box-wheel findings. Remaining work is R3D-03/04/05/07/09/16.
- [Asset pipeline research](https://github.com/r-melvin/Cataclysm-BN-3D/blob/docs-archive/docs/en/dev/research/sprite_to_voxel_pipeline.md):
  VOX ingestion and the separate Go CLI supersede the old OBJ-export obstacle.
  R3D-05/06 cover remaining asset trials and character integration.
- [Renderer](docs/en/dev/guides/renderer_3d.md),
  [comparison](tools/r3d_spike/COMPARE.md), and
  [item](data/raw/r3d/items/README.md)/[vehicle-part](data/raw/r3d/vehicle_parts/README.md)
  authoring guides describe shipped behavior, not additional task queues.

No paid CI, Unity/MelonLoader runtime dependency, whole-vehicle static replacement,
or simulation change is introduced by this backlog. External art/code still
needs compatible licensing and per-asset provenance.
