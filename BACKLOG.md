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

Sources: [agent guidelines](AGENTS.md),
[wrapper](tools/r3d_spike/astyle_347.py).

## R3D-03: Complete the presentation-profile shape and material contract

**Status: design/implementation follow-up.** Size, support-anchor and depth
profiles work. Material regions are gallery annotations, not runtime material
mappings. Geometry-family overrides, source face regions and per-tileset texture
donors from the original proposal are not implemented.

Define those fields for a bin, chair, locker, window, door, tree, loose item and
vehicle corner. Bind sprite crops to a tileset/sprite identity; cover footprint,
face materials, anchors and fallback without changing gameplay flags. Validate
resolution, crop bounds, source texture sheet and missing/invalid overrides.
Do not restore unused runtime metadata without a rendering consumer.

Sources: [Dramatic Shape review](docs/en/dev/research/dramatic_shape_voxel_review.md),
[vehicle research](docs/en/dev/research/vehicle_rendering_research.md).

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

Source and acceptance matrix:
[vehicle research](docs/en/dev/research/vehicle_rendering_research.md).

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
[sprite-to-voxel pipeline](docs/en/dev/research/sprite_to_voxel_pipeline.md).

## R3D-06: Directional characters with equipment layers

**Status: art/renderer design; depends on R3D-05's four-view trial.** Characters
use layered camera-facing sprites with left/right mirroring. The inspected
built-in art does not supply all four views.

Define travel/facing relative to the camera and provide a representative
left/front/right/back sheet or 3D model. Preserve body, hair, clothing, armor and
wielded layers, with sprite fallback for missing directional assets. Check
walking toward/away/sideways, stopping, camera rotation and mixed equipment.
Mirroring one image cannot synthesize a correct face or back.

Sources: [asset trial](docs/en/dev/research/sprite_to_voxel_pipeline.md),
[Dramatic Shape review](docs/en/dev/research/dramatic_shape_voxel_review.md).

## R3D-07: Reproducible visual fixtures and GPU acceptance checks

**Status: validation/tooling follow-up.** Live sessions already bundle saves,
logs, source galleries, support markers, 2D and eight 3D views. The state matrix
reports available/observed sprites, not automatic captures of every state.
The 17 seeded geometry snapshots cover positions/light, not UVs, textures,
shaders or the final GPU image.

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

2026-10-19 Linux virtual-window check: direct saved-world loading exposed a
loading-screen framebuffer initialization crash, fixed in `84fd8ea`. The paired
one-shot capture now succeeds with software Vulkan and malformed-asset fallback.
The full eight-angle session subsequently faults in a Lavapipe 26.2.4 worker after
two yaw views. Reproduce with a stable software driver and native GPU, distinguish
driver failure from invalid renderer submissions, and complete view-restoration
validation before closing this item. Current private evidence is under
`/tmp/bn-assets-live/` (temporary, not a durable/shared fixture).

Sources: [comparison guide](tools/r3d_spike/COMPARE.md),
[Dramatic Shape review](docs/en/dev/research/dramatic_shape_voxel_review.md),
[vehicle validation matrix](docs/en/dev/research/vehicle_rendering_research.md).

## R3D-08: Measure frame cost and decide chunk scheduling

**Status: measurement first, conditional optimization.** Chunk timing/queue
logging exists. Initial visible chunks build synchronously; later rebuilds use
a count budget, not a time allowance. Some timing calls run with diagnostics
disabled. Silhouettes avoid copying the entity view but still build a separate
avatar-only mesh.

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
[measurements](docs/en/dev/research/r3d_review_2026_10.md),
[chunk research](docs/en/dev/research/dramatic_shape_voxel_review.md).

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

Source: [vehicle visibility research](docs/en/dev/research/vehicle_rendering_research.md).

## R3D-10: Native macOS and Windows validation

**Status: requires platform access.** Linux builds, renderer tests and reports
have local evidence. Native macOS/Windows game execution is unverified.

Build/play on Metal and Direct3D 12 with the required shaders. Check GPU/2D
fallback, scaling/ultrawide layout, controls, options, save/load and capture/
package/restore; repeat relevant checks for integrated feature ports. Cross-builds
and path-resolution tests do not replace native runs.

Sources: [README](README.md), [renderer guide](docs/en/dev/guides/renderer_3d.md).

## R3D-11: Integrate the native UI/HUD branch

**Status: implemented on a branch, not merged.** Local `feat/ui-overhaul` at
`ce6d386` has menu themes, the F4 report, HUD indicators and weather/sky effects.
Its guide is `docs/en/dev/guides/ui_overhaul.md` on that branch.

Refresh against current main; resolve camera/visibility overlap and validate
native UI refresh, perception, input, capture and 2D fallback before merging.
Coordinate existing sky/fog effects with R3D-14 instead of starting a separate
weather renderer.

## R3D-12: Integrate the first-person branch

**Status: implemented on a branch, not merged.** Local `feat/first-person` at
`ae02e6f` has the BN camera/overlays and a branch guide at
`docs/en/dev/guides/first_person.md`.

Refresh against main and validate eye-level geometry, visibility, targeting,
direction prompts, menus, minimap/alerts, saved heading, camera switching and
comparison restoration. Preserve discrete BN movement/turns; analog or real-time
movement is not part of integration.

## R3D-13: Integrate guided onboarding

**Status: implemented on a branch, not merged.** Local
`feat/guided-onboarding` at `44dcdea` includes lessons, screen explanations,
progress persistence and translation extraction. Its branch guide is
`docs/en/dev/guides/guided_onboarding.md`.

Refresh against main, run `[onboarding]` and affected normal UI tests, and check
option modes, actions, old/new saves, translation coverage and the independent
Tutorial Game flow. Keep the guided start kit optional; hints must not spend
turns or alter normal gameplay rules.

## R3D-14: Fog of war and a time-of-day sky

**Status: research requested; related effects exist on R3D-11's branch.**
Distinguish unseen terrain from visible darkness and replace the uniformly black
exposed background with a game-time/weather-driven sky or horizon. Evaluate
existing UI-branch effects first. Cover indoor/underground scenes, weather,
cutaways, first person and remembered terrain. Keep BN lighting/knowledge
in control. See the [Dramatic Shape constraints](docs/en/dev/research/dramatic_shape_voxel_review.md)
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

Source: [renderer review](docs/en/dev/research/r3d_review_2026_10.md).

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
movement. Source: [Dramatic Shape review](docs/en/dev/research/dramatic_shape_voxel_review.md).

## Audit references and completed foundations

- [Renderer review](docs/en/dev/research/r3d_review_2026_10.md): wheel/depth/roof/
  shared-edge/cargo-floor/lighting/camera/classification fixes are delivered;
  billboard/material/grid dead code and windshield duplication are removed.
  Asset loading and shared formatter changes are recorded in R3D-01/02 above;
  remaining acceptance and findings map to R3D-07/08/09/16.
- [Dramatic Shape review](docs/en/dev/research/dramatic_shape_voxel_review.md):
  size/anchor/depth profiles, galleries/support markers, local contact shading,
  chunk diagnostics and avatar-only silhouettes exist. Extensions/acceptance
  map to R3D-03/05/06/07/08/18.
- [Vehicle research](docs/en/dev/research/vehicle_rendering_research.md): role
  lists, shaped panels, faceted wheels and bundled wheel models supersede its
  no-layer-list/box-wheel findings. Remaining work is R3D-03/04/05/07/09/16.
- [Asset pipeline research](docs/en/dev/research/sprite_to_voxel_pipeline.md):
  VOX ingestion and the separate Go CLI supersede the old OBJ-export obstacle.
  R3D-05/06 cover remaining asset trials and character integration.
- [Renderer](docs/en/dev/guides/renderer_3d.md),
  [comparison](tools/r3d_spike/COMPARE.md), and
  [item](data/raw/r3d/items/README.md)/[vehicle-part](data/raw/r3d/vehicle_parts/README.md)
  authoring guides describe shipped behavior, not additional task queues.

No paid CI, Unity/MelonLoader runtime dependency, whole-vehicle static replacement,
or simulation change is introduced by this backlog. External art/code still
needs compatible licensing and per-asset provenance.
