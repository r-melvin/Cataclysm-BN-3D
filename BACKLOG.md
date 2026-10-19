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

Use Kenney Shape to author a wheel `.vox` and one vehicle panel, then use
2d-to-3d-voxelizer's four-view mode to prototype a character with left, front,
right, and back views. Resolve the voxelizer's OBJ output into the renderer's
asset path, while preserving game-driven vehicle assembly and character
equipment layers. The detailed trial and success gates are in
[`sprite_to_voxel_pipeline.md`](docs/en/dev/research/sprite_to_voxel_pipeline.md).
No authored models are bundled yet; the current sprite and procedural fallbacks
remain active until generated assets pass visual comparison.

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
