# Authored item voxel models

The 3D renderer can load optional MagicaVoxel `.vox` files from this directory
when **3D voxel-style item models** is enabled. Name each file with the exact
item type ID, such as `water_clean.vox`. The model is centered over its map tile
and proportionally scaled so its longest dimension is one tile. X and Y lie on
the ground plane; Z points up. Palette index zero is transparent, and occupied
voxels should use nonzero palette indices.

An authored model takes precedence over the renderer's sprite-based voxel
profile for that item. If the file is absent or invalid, the existing sprite
rendering remains available. Keep source artwork, conversion settings, and
license information with contributed models; do not add models whose source
licenses do not permit redistribution.

`carbondioxide_tank.vox` is a hand-authored renderer asset with a steel cylinder,
base ring, shoulder and valve assembly. Its dimensions and ground support are
defined in `../presentation_profiles.json`; it uses palette slots 1 and 3 for
the shell, 2 for dark hardware, and 4 for the red valve grips.

Use `../presentation_profiles.json` to refine scene proportions and the point
that touches the ground. Each profile names a `category` and exact asset `id`;
`size` is `[forward, right, up]` in map-tile units, and `support_anchor` is the
same three axes in normalized model coordinates. An omitted or zero size axis
keeps the model's inferred sprite or VOX dimension. `depth_fraction` controls
sprite silhouette extrusion. `material_regions` names palette-index groups for
the art gallery and future material-aware rendering. The comparison capture
copies this file into its art assets so profile values remain reproducible.
