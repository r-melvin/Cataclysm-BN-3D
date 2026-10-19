# Project backlog

Ideas parked for later work. These are not active tasks and should not be
implemented unless requested.

## Vehicle-free play mode for the 3D renderer

Explore an optional mode for players who want to use the 3D renderer while
vehicle visuals are being improved. This would temporarily remove vehicles as
playable content, rather than hide their models in the renderer.

Before implementation, define how the mode handles vehicle world spawns,
construction and parts, driving and interaction, and vehicles already present
in existing saves. Prefer an opt-in, clearly labelled mode that does not change
the default game or the 2D renderer. Determine which parts can be safely
controlled by data or Lua and which require core C++ hooks; a partial mod must
not imply that all vehicle gameplay is disabled.

## Improve the feel of turn-based driving in the 3D renderer

Keep BN's discrete vehicle simulation, collision checks, action costs, and turn
economy. First instrument how often the renderer is redrawn during vehicle
substeps, then prototype renderer-only interpolation of the vehicle's committed
position and heading and the camera target between those states. Snap on
collisions, map shifts, portals, save/load, and any transition where an
interpolated path would be misleading. If control still feels coarse, review
the existing `VEHICLE_CONTROL_SCALE` option and turn-based driving affordances.
Treat real-time vehicle physics/input as a final resort because it would require
broader changes to world-turn scheduling, actor fairness, action costs, and
collision semantics.

Relevant paths: `src/vehicle/vehicle_move.cpp` (`pldrive()` and `act_on_map()`),
`src/vehicle/vehicle.cpp` (`gain_moves()`), `src/map/map.cpp` (`vehmove()`),
`src/game.cpp` (world-turn processing), and `src/r3d.cpp` (camera heading).

## Prototype sprite-to-voxel assets for vehicles and characters

Authored `.vox` models for the standard wheel, wide wheel and carbon dioxide
tank are bundled under `data/raw/r3d/`, with sprite and procedural fallbacks for
everything else. Next: author one vehicle panel, then use 2d-to-3d-voxelizer's
four-view mode to prototype a character with left, front, right, and back views.
Resolve the voxelizer's OBJ output into the renderer's asset path, while
preserving game-driven vehicle assembly and character equipment layers. The
detailed trial and success gates are in
[`sprite_to_voxel_pipeline.md`](docs/en/dev/research/sprite_to_voxel_pipeline.md).

## Add directional character models to the 3D renderer

The current renderer draws player and NPC layers as camera-facing sprites. The
built-in tilesets inspected so far provide a single character image rather than
front, back, and side views, so mirroring or rotating that image cannot produce
correct directional poses. Define a direction convention from character travel
relative to the camera, then provide 3D character geometry or authored four-way
pixel-art views. Preserve the existing layered clothing and wielded-item look,
and fall back to the current sprite when a tileset has no directional assets.
The four-view voxel prototype is tracked in
[`sprite_to_voxel_pipeline.md`](docs/en/dev/research/sprite_to_voxel_pipeline.md).

## Add fog of war and a time-of-day sky to the 3D renderer

Research how to use the game's visibility and lighting state to render fog of
war that distinguishes unseen areas from visible darkness. Explore a sky and
ambient horizon lighting driven by the in-game day/night cycle so the exposed
world does not disappear into a uniformly black background. Preserve gameplay
visibility rules, and account for indoor, underground, weather, and renderer
cutaway views when choosing fog and sky behavior.

## Render multi-level vehicles in the 3D renderer

Vehicle mounts carry a z coordinate, but the 3D renderer draws each vehicle at
one vehicle-wide base height and keys its render cells by mount x and y only,
so parts on different levels overwrite each other. Key cells by the full mount,
offset each cell's geometry by its mount z, and make the neighbour lookups
(walls, trim, bumpers, roofs) level-aware.

## Find why vehicle meshes differ between runs

With a seeded RNG, `build_vehicle_mesh` for the same spawned vehicle gives the
same vertex counts but different vertex positions on each run of the same
binary. Find the source (atlas-dependent sprite trim bounds, uninitialised
data, or unordered container iteration) and make geometry deterministic so
vehicle geometry can be regression-tested by hash.

## Measure 3D renderer frame cost

Record `CATA_R3D_DEBUG_CHUNKS=1` frame timings for a parked vehicle, a moving
vehicle and a crowded scene. Use them to decide whether per-vehicle mesh
caching (deferred; see
[`r3d_review_2026_10.md`](docs/en/dev/research/r3d_review_2026_10.md)) or other
per-frame work is worth optimising.

## Visual regression checks for the 3D renderer

Turn the comparison captures in `tools/r3d_spike/` into screenshot diffs of a
few fixed scenes (vehicles with roofs, wheels and shaped panels; voxel items;
the avatar silhouette), so geometry refactors such as merging the front and
rear windshield paths can be checked without manual play.

## Confirm CI runs the `[r3d]` tests

Locally the tests only start with `CATA_TEST_COMPUTE_ACCELERATION=cpu`. Check
whether CI hits the same SDL_GPU initialisation failure and silently skips
them; set the variable in CI if so.

## Fix `vehicle_profile` classification quirks

Tests found that the `door` render role never fires for stock parts (no part
has `DOOR` or a `door` location), a closed door counts as interior, and the
corner of a mount with two shaped panels depends on install order. Base each on
the right part flags.

## Make 3D asset loading robust

One failing `.vox` file aborts loading of the rest, stream read errors are not
checked, assets do not reload with the tileset, and loading on the first frame
causes a stall. Load per file with isolated errors, check reads, reload with
the tileset, and load off the first frame.

## Remove 3D renderer dead code

Drop the unreachable `item_geometry::billboard`, the C++ parsing of
`material_regions` that nothing renders, and the duplicated VOX occupancy
validation in `read_vox_model` and `extract_vox_surface`.

## Pin astyle 3.4.7 in the format tooling

astyle 3.6.x mis-indents functions with trailing return types. Make the
`format` target and CI use astyle 3.4.7 (as
`tools/r3d_spike/astyle_347.py` does) instead of relying on agents reading
`AGENTS.md`.
