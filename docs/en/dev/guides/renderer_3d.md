# Experimental 3D renderer fork

This fork ports the renderer from
[r-melvin/Cataclysm-DDA-3D](https://github.com/r-melvin/Cataclysm-DDA-3D),
renderer commit `00d5825a2585bc58cc46c4cfe5d72f7c7f46d37a`, onto Bright Nights.
The original Cataclysm contributors retain their existing attribution and license.
BN remains responsible for gameplay, world generation, visibility, turns, and saves.
CDDA worlds and mods are not migration inputs for this port; start with a BN world.
Current limitations, feature-branch integration and research follow-ups are tracked
only in the [project backlog](../../../../BACKLOG.md).

## Build and play

Use BN's Clang/CMake build instructions. Renderer shaders use BN's existing HLSL
and shadercross build pipeline; backend artifacts are generated in `data/shaders`
for the requested `SHADER_TARGETS`. No separate GLSL compiler is required.
When editing HLSL, enable `BUILD_SHADERCROSS` or set `SHADERCROSS_EXE` to an
available compiler; the precompiled fallback does not rebuild shader sources.
For Vulkan builds, install SPIRV-Tools and build the `validate-shaders` target
after compiling. It checks every shipped SPIR-V module against Vulkan 1.0 rules,
including uniform-buffer layout. Native Metal/Direct3D execution remains a
separate platform check.

```sh
cmake --preset linux-slim
cmake --build --preset linux-slim --target cataclysm-bn-tiles cata_test-tiles
./out/build/linux-slim/src/cataclysm-bn-tiles
```

In **Options > Graphics > Choose tileset**, select **UltimateCataclysm** (Ultica)
for the recommended 3D artwork. The renderer uses the selected game tileset;
enabling 3D does not change an existing selection such as Hoder. Smaller/older
tilesets have incomplete asset coverage. Missing wall art uses a generic material
instead of stretching the tileset’s unknown marker across the wall. Brick, timber,
stone and metal walls use elevation textures tinted from the tileset; top-down
studs and wall caps are not repeated over their sides. Painted walls retain
their native elevation unless the plaster texture option is enabled.

**3D thin walls** draws ¼-tile walls at tile edges, with the room floor extending
beneath the wall tiles, including corners. Exterior faces sit against the far
edge from the room. Interior partitions and ambiguous walls consistently use
the north/west edge. Doors, windows, surrounds and fitted cabinetry follow the
same line. Floor roof metadata identifies building interiors even when the
terrain lacks an `INDOORS` flag.

With 3D thin walls enabled, characters keep whole-tile positions but can enter
the free floor in a wall tile. Crossing the wall strip is blocked. Player and AI
movement, routes, melee/reach and projectiles use that boundary; closed doors
open when their edge is reached. Vehicle parts sweep their native tile footprint
against the strips. Terrain IDs, materials and damage states remain native.
Disabling thin walls or 3D restores whole-cell collision. Mouse picking and the
lighting grid still use native tile columns.

In Options > Graphics, select the `gpu` renderer, restart if changing renderers,
and enable **Voxelised 3D renderer (experimental)**. Disable it in the same menu
to return to the standard isometric view. Flat-renderer fallback retains native
whole-cell collision.
New configurations prefer `gpu` when SDL provides it. Unsupported graphics
backends retain BN's ordinary 2D view.

- **Ctrl+B** toggles 3D.
- With **3D WASD controls** enabled, W/A/S/D walk relative to the camera and arrows
  rotate/tilt it. These keys replace their ordinary gameplay bindings only in the
  main 3D gameplay view. Menus retain their normal controls.
- The wheel zooms; **Ctrl+R** resets the camera.
- Middle drag orbits; Shift+middle drag pans.
- All camera actions can be rebound or selected through the action menu.

Graphics options control thin walls, wall finishes, cutaways, storey height, and
view depth. The port includes wall corner trimming, wall surrounds for windows,
wall-mounted boards, shallower lockers fitted against walls, upright loose items,
solid natural objects, and visible vehicles. Movement, combat, vehicle physics,
and item behavior remain BN's own.

The optional **3D avatar occlusion silhouette** graphics setting shows a faint
cyan silhouette of the avatar where world geometry hides them. It uses the same
visible player sprite layers and does not reveal other creatures or items. It can
be used alongside the cutaway setting.

Appliances use closed boxes with separate top, front and side materials. Their
full height is independent of the transparent margins in top-down artwork, and
face profiles apply to sinks, ovens and dishwashers as well as storage furniture.

Vehicle quarterpanels (`HALF_BOARD`) stay at body height even beneath a roof;
full panels and windshields keep cabin height. Roof caps take the exterior panel
material from their own mount, while rooftop equipment keeps its own sprite.
Body faces sample that material once, without a second vehicle-wide paint tint. Native `shapes` direction
variants ending in `_nw`, `_ne`, `_sw`, or `_se` trim the corresponding body corner.
These are generated part IDs, not new map or vehicle rules. Once a vehicle has a
renderable mount, its rigid model stays complete rather than disappearing in
sections; unlit mounts use a dim fallback. A vehicle with no renderable mounts
remains hidden. The backlog records the remaining knowledge/occlusion audit.
Installed wheel parts now use faceted elliptical tire meshes with raised hubs,
and map the installed wheel sprite onto the tire face. BN remains authoritative
for wheel damage, removal, and vehicle behavior.

The optional **3D voxel-style item models** graphics setting extrudes loose-item
sprites that have a transparent foreground and no separate backdrop. The source
art stays on the front and back, while shaded faces follow opaque-pixel
boundaries, preserving transparent corners and notches. Sprite rotation and
mirroring are applied to the silhouette. Generic items use a shallow volume;
cylinders use deeper profiles, and authored `.vox` items take precedence when
available. Items without a supported transparent sprite keep the upright
billboard fallback. The setting is off by default. This adapts the general
single-sprite extrusion idea from
[2d-to-3d-voxelizer](https://github.com/GazPrash/2d-to-3d-voxelizer) to the
renderer; no external code or art is included.

Authored asset proportions and contact points are defined in
`data/raw/r3d/presentation_profiles.json`. A profile names a category and asset
ID, gives optional forward/right/up dimensions in map tiles, and marks the
normalized point that should touch its support surface. Zero dimensions retain
the inferred sprite or VOX size. Sprite depth can be specified directly with a
right-axis size or as a width-relative `depth_fraction`; an authored
`depth_fraction` takes precedence over the depth implied by `size` (right /
forward), which in turn overrides the built-in default. Material-region names
and VOX palette slots are included for authoring and comparison; they are
currently shown as annotations in the gallery rather than changing palette
colors. Comparison packages preserve the profile file and render a scaled top
footprint and side elevation beside the source sprite and captured views.


Box and slab furniture also consumes `size` and `support_anchor`. For furniture,
axes are local width (east), depth (south), and height before its clockwise
facing rotation; mirroring also mirrors the width anchor. For example:

```json
{
  "category": "furniture",
  "id": "f_trashcan",
  "size": [0.4, 0.6, 0.8],
  "support_anchor": [0.5, 0.5, 0.0]
}
```

A nonzero width/depth is exact, replacing native row extension and locker depth
reduction on that axis. Its anchor sits at the tile centre; explicit anchors can
also reposition inferred spans. Adjacent thin walls take precedence over the
horizontal anchor: translate the authored span flush to the wall without
stretching it. With opposite wall contacts, the back wall wins on the depth
axis, otherwise the north/west wall wins. Omitted axes retain native wall fitting
and row joins. Height is measured from the support point on the floor; the
normalized vertical anchor can place part of the model below that surface.
The cached top height follows the same geometry; the existing one-level marker
height clamp still applies. Legs
stay within small authored footprints. These overrides do not change furniture
collision, storage, reach, movement or other native gameplay properties.
Upright, natural-block and panel furniture retain their inferred geometry.
`depth_fraction` remains an item extrusion control, not a furniture-box size.
Bundled examples include the talking doll’s toy proportions and the trashcan’s
Ultica body/lid crops. The latter deliberately excludes transparent margins and
the baked ground shadow so each solid face remains connected.

Wall profiles also support `face_sources` for explicit side and top materials.
These are opt-in entries in the same version-1 file, for example:

```json
{
  "category": "terrain",
  "id": "t_wall",
  "face_sources": [
    {
      "face": "side",
      "tileset": "UltimateCataclysm",
      "sprite": "t_wall_edge",
      "rotation": 0,
      "crop": [0.0, 0.25, 1.0, 1.0]
    }
  ]
}
```

The example illustrates the format, not a recommended crop for every tileset.
`tileset` is the exact tileset option ID; `sprite` must exist in that tileset.
The source must name a concrete foreground sprite, such as `t_wall_edge`,
rather than a multitile container without foreground art. Optional `rotation`
is in clockwise quarter turns (0 to 3). `crop` is
[left, top, right, bottom] in normalized, unrotated source-sprite coordinates;
it defaults to the whole sprite. A donor retains its own texture sheet and
ownership. One source per face per tileset is allowed. Invalid profiles are
reported through asset-loading warnings; absent donors and other tilesets keep
the normal inferred material. Installing/reloading profiles rebuilds affected
cached geometry. The renderer does not change terrain collision, visibility,
door/window states or native part colours to apply these textures.

These face sources apply to solid walls and window/door surrounds. A profile on
an opening overrides only its selected surround faces; missing faces still use
the neighboring wall. The window pane, door leaf, hinge and open/broken state
retain their native artwork and geometry. Quarter-turns and crops also apply to
partial sill/lintel sections. Native broken/empty window frames remain upright
even though BN deliberately omits their `WINDOW` gameplay flag.
Box and slab furniture (including lockers, chairs, tables and bins) uses the
same `face_sources` contract under category `furniture`. `top` replaces the lid,
`side` replaces vertical body/rim faces other than the front, and optional
`front` replaces only the front face. Without `front`, native handles and doors
remain visible. The top follows the furniture's facing in addition to its donor
rotation. Crops do not alter the inferred footprint, wall fitting, joins, slab
thickness, feet or underside. Those support surfaces retain their native art.
Upright, panel and natural solid furniture keep their existing rendering.

| Profile fields | Current runtime consumers |
| --- | --- |
| Size, support anchor, depth fraction | Loose items and authored item models |
| Side/top face sources | Solid walls, window/door surrounds, box/slab furniture |
| Front face source | Box/slab furniture |
| Material-region names and palette slots | Gallery annotations |

Geometry-family overrides, arbitrary furniture dimensions/anchors, vehicle
layers and runtime palette material regions remain
in [the presentation backlog](../../../../BACKLOG.md#r3d-03-complete-the-presentation-profile-shape-and-material-contract).

## Renderer diagnostics

Set `CATA_R3D_DEBUG_CHUNKS=1` before launching to log a chunk-work sample after
the initial visible chunks are built and once per second afterward. `frame_ms`
is renderer CPU time; `mesh_ms` measures CPU meshing;
`upload_ms` measures chunk buffer creation and geometry upload; and `light_ms`
measures light sampling and light-buffer upload. The sample also reports the
visible, rebuilt, deferred, and failed chunk counts, geometry size, uploaded
bytes, and estimated bytes held by resident chunk buffers. The initial visible
scene is built synchronously, so its first sample shows that cost even though
there is no deferred queue. These measurements separate mesh work, light work,
and queued rebuilds before changing the existing scheduling policy. Diagnostic
timing reads are disabled when `CATA_R3D_DEBUG_CHUNKS` is absent; asset-refresh
and capture deadlines retain their independent clocks.

## Comparison reports

BN ships the `HitButton_iso` reference and `UltimateCataclysm` tilesets; no external
CDDA tileset composition is needed.

```sh
python3 tools/r3d_spike/compare_session.py 'My BN world' \
  --binary out/build/linux-slim/src/cataclysm-bn-tiles --output /tmp/bn-report
```

Play and quit normally. A private world copy is used; session progress remains in
`/tmp/bn-report/user`, and the original world is untouched. Five checkpoints are
recorded at least 30 seconds apart at normal gameplay input boundaries. Each has
one fixed 2D isometric reference, eight 3D angles, the current world snapshot,
active options, map/state dumps, build/mod information, and selected logs.
Captures briefly pause play without charging realtime moves or consuming queued
input. SQLite snapshots include committed WAL pages and do not require copying
live journal files.

Attach every generated `comparison-NNN.zip` to a fork issue. Files stay below
GitHub's 25 MB limit; oversized individual saves are chunked without dropping data.
The included `restore.py` reconstructs them after extraction. Review the archive
before sharing: character names, notes, explored maps, and paths can be included.
Saves reproduce a scene with matching BN assets/mods, not deterministic future RNG.
See `tools/r3d_spike/COMPARE.md` for controls and crash recovery.

Native BN implementations of UI/HUD/weather, first-person camera and guided
onboarding exist on separate feature branches and are not included on `main`.
Their integration and validation status is tracked in the backlog. The UI port
uses BN's own interface rather than CDDA's ImGui frontend.
