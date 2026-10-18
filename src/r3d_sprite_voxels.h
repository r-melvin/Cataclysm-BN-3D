#pragma once
#ifndef CATA_SRC_R3D_SPRITE_VOXELS_H
#define CATA_SRC_R3D_SPRITE_VOXELS_H

#include <cstdint>
#include <span>
#include <vector>

namespace r3d
{

enum class sprite_voxel_face : uint8_t {
    front,
    back,
    left,
    right,
    top,
    bottom
};

struct sprite_voxel_surface_face {
    sprite_voxel_face face;
    float x0;
    float z0;
    float x1;
    float z1;
    int sample_x;
    int sample_y;
};

/// Faces needed to extrude an alpha silhouette by one depth interval.
struct sprite_voxel_surface {
    int width = 0;
    int height = 0;
    std::vector<sprite_voxel_surface_face> faces;
};

struct sprite_alpha_image {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels;
};

struct orient_sprite_alpha_options {
    int width;
    int height;
    std::span<const uint8_t> alpha;
    int quarter_turns;
    bool flip;
};

/// Transform source alpha into the same displayed orientation used by the sprite UVs.
auto orient_sprite_alpha( const orient_sprite_alpha_options &options ) -> sprite_alpha_image;

/// Build sprite planes and exposed contour faces from a row-major opaque-pixel mask.
auto voxelize_sprite_alpha( int width, int height, std::span<const uint8_t> alpha )
-> sprite_voxel_surface;

} // namespace r3d

#endif // CATA_SRC_R3D_SPRITE_VOXELS_H
