# Sprite-to-voxel asset pipeline for vehicles and characters

Research date: 2026-10-19. This is a prototype plan based on the tools' published
capabilities and the current renderer interfaces. Neither tool has been
integrated into the game's build or runtime.

## Finding

[2d-to-3d-voxelizer](https://github.com/GazPrash/2d-to-3d-voxelizer) and
[Kenney Shape](https://kenney.nl/tools/kenney-shape) could form a useful art
pipeline, but serve different jobs. The voxelizer documents single-sprite and
multi-view modes, including a four-view sheet ordered `[LEFT, FRONT, RIGHT,
BACK]`; it exports OBJ. Its README says the shape reconstruction uses simple
math and can struggle with pixel art that contains varying depths. Kenney Shape
lets an artist set per-pixel depth and export MagicaVoxel `.vox`, which the BN
renderer can now read for vehicle wheels. Kenney Shape's documented workflow
does not describe importing a PNG; it is best treated as a manual modeling and
cleanup step.

The two tools therefore cover two important gaps: multi-view sprite input for
characters and manual per-pixel volume authoring in the renderer's existing
`.vox` format for items and vehicle parts. The renderer loads authored item
models named by item type ID from `data/raw/r3d/items/`, fitting each model
proportionally into a one-tile cube while keeping sprite-based voxel profiles as
the fallback. Kenney Shape's sample character project in the
local tool package is a compressed JSON document, but its undocumented
project schema should not become a runtime dependency; the documented VOX
export is the clean integration boundary. The renderer can now load a `.vox`
asset matching a vehicle part ID and use it to replace that part's procedural
geometry while retaining the game's part assembly and state. They do not yet
make one automated pipeline:
the voxelizer's published output is OBJ, its README does not document a batch or
command-line interface, and Kenney Shape's published limits are 64×64 pixels,
16 colors, with extruded art looking the same from both sides.

## Prototype plan

1. **Loose item:** choose a recognizable item with clean source art, author a
   compact model in Kenney Shape, and place the exported `.vox` under
   `data/raw/r3d/items/<item-type-id>.vox`. Confirm the face orientation and
   compare it at all camera angles with the existing sprite-based fallback.
2. **Vehicle wheel:** use the existing native wheel sprite as a reference, make a
   compact model in Kenney Shape, export `.vox`, and place it under
   `data/raw/r3d/vehicle_parts/<native-part-id>.vox`. Verify the palette,
   orientation, scale, wheel state fallback, and eight yaw views against the
   procedural wheel. This path is already supported by the renderer.
3. **Vehicle body part:** try one panel or fender. Place its model beside the
   wheel assets with the native part ID as the filename. The renderer scales it
   to that part's mount bounds and uses the existing procedural geometry when
   the asset is absent. Keep the BN part ID, mount,
   paint, state, and assembly dynamic in the game; the authored asset describes
   only reusable shape and material. Compare manual Kenney modeling with a
   voxelizer-generated OBJ from the available sprite views. Do not adopt the OBJ
   route until its geometry, texture/palette, pivot, and scale can be imported
   reproducibly.
4. **Character:** prepare a centered four-view sheet for one representative
   character in `[LEFT, FRONT, RIGHT, BACK]` order and test the voxelizer's quad
   mode. Inspect its OBJ and determine whether adapting its exporter to MagicaVoxel
   `.vox` is practical, or whether a small OBJ importer is the better boundary.
   Validate that view selection follows character travel relative to the camera.
5. **Layering and variation:** preserve the current base-body, clothing, hair,
   armor, and wielded-item layers. Test whether each layer can have compatible
   four-view geometry and combine at runtime. If a layer lacks directional art,
   retain the existing billboard for that layer. Avoid baking equipment
   combinations into a single model.
6. **Package only proven assets:** record source sprite IDs, tool version,
   conversion settings, palette, pivot, scale, and licenses beside each model.
   Keep the editor tools out of the game's runtime and preserve current
   sprite/procedural fallbacks for absent or invalid files.

The first success gate is visual, not just parser acceptance: the voxelized wheel
must be clearer than the faceted fallback, and the four-view character must
show a recognizable back, face, and left/right profiles without breaking
clothing or wielded layers. If a tool's output needs more manual repair than the
result saves, retain the renderer fallback and stop that route.

## References

- [2d-to-3d-voxelizer README](https://github.com/GazPrash/2d-to-3d-voxelizer):
  single, dual, quad, and six-view modes; quad-sheet order; OBJ output; stated
  reconstruction limitations; MIT license.
- [Kenney Shape](https://kenney.nl/tools/kenney-shape): per-pixel depth, model
  export formats, equal-looking front/back extrusion, and art-size/color limits.
- [Kenney Shape documentation](https://kenney.nl/knowledge-base/tools/kenney-shape-documentation):
  manual depth editing, optimization, VOX export, and square-voxel conversion.
- [Vehicle part model authoring guide](../../../../data/raw/r3d/vehicle_parts/README.md):
  current runtime filename and coordinate conventions.
