# Optional 3D vehicle part models

Place MagicaVoxel `.vox` models in this directory and name each file with the
native Bright Nights vehicle part ID, for example `wheel_wide.vox`. The
renderer loads these files at 3D renderer initialization and uses them when a
matching installed part is present. Missing, malformed, or unsupported models
fall back to the regular tileset and procedural geometry.

The current custom-model path is for wheels. In the model, X runs along the
vehicle's forward/rear axis, Y runs across its width, and Z is vertical. The
renderer scales the model into the wheel's mount-space bounds, so the source
model's proportions are normalized. Keep the wheel centered in the XY footprint
and fill the model's Z height. The MagicaVoxel palette is used directly; palette
index zero is transparent and occupied voxels should use nonzero indices.

Before adding an asset, check that its source artwork and model license allow
redistribution with Bright Nights 3D. Keep the `.vox` source file in the repo so
the asset remains editable.
