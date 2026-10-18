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
