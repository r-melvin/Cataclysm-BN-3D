#pragma once
#ifndef CATA_SRC_R3D_PROP_GEOMETRY_H
#define CATA_SRC_R3D_PROP_GEOMETRY_H

#include <array>
#include <vector>

namespace r3d
{
class quad_sink;
struct sprite_ref;

enum class solid_prop { none, overturned_bench, metal_wreckage };

/// Local tile-space cuboid. Rotation is around its centre, in radians.
struct prop_cuboid {
    std::array<float, 3> center;
    std::array<float, 3> size;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

/// Closed solids, independent of the source sprite's baked perspective and transparency.
auto prop_cuboids( solid_prop kind ) -> std::vector<prop_cuboid>;

struct emit_prop_options {
    quad_sink &sink;
    const sprite_ref &material;
    const std::vector<prop_cuboid> &boxes;
    int x;
    int y;
    float base;
    int facing = 0;
};
auto emit_prop_cuboids( const emit_prop_options &options ) -> void;

struct stair_flight_options {
    bool down;
    bool along_y;
    int segment;
    int segments;
};
/// Each tile contributes four treads to a continuous, one-storey flight.
auto stair_cuboids( const stair_flight_options &options ) -> std::vector<prop_cuboid>;
} // namespace r3d
#endif
