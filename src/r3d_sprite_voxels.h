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

enum class orthogonal_face_kind : uint8_t {
    top,
    bottom,
    south,
    north,
    west,
    east
};

struct orthogonal_voxel_options {
    int width;
    int height;
    std::span<const uint8_t> mask;
};

/// 3D voxel grid representing the boolean intersection of four swept orthogonal extrusions.
struct orthogonal_voxel_shape {
    int size_x = 0;
    int size_y = 0;
    int size_z = 0;
    std::vector<uint8_t> voxels;

    auto is_occupied( int x, int y, int z ) const -> bool {
        if( x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z ) {
            return false;
        }
        return voxels[static_cast<size_t>( ( z * size_y + y ) * size_x + x )] != 0;
    }
};

struct orthogonal_surface_quad {
    orthogonal_face_kind face;
    float x0 = 0.0f;
    float y0 = 0.0f;
    float z0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    float z1 = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
};

struct orthogonal_voxel_mesh {
    int size_x = 0;
    int size_y = 0;
    int size_z = 0;
    std::vector<orthogonal_surface_quad> quads;
};

/// Generate four orthogonal voxel extrusions by applying the same 2D mask to each horizontal
/// side of a bounding cube, sweeping occupied pixels inward along the face normal, and
/// computing the boolean intersection of all four swept volumes.
auto generate_orthogonal_voxel_shape( const orthogonal_voxel_options &options )
-> orthogonal_voxel_shape;

/// Extract greedily merged exposed quad faces from an orthogonal voxel shape.
auto extract_orthogonal_voxel_mesh( const orthogonal_voxel_shape &shape )
-> orthogonal_voxel_mesh;

} // namespace r3d

#endif // CATA_SRC_R3D_SPRITE_VOXELS_H
