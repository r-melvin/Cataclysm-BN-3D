#pragma once
#ifndef CATA_SRC_R3D_CAMERA_H
#define CATA_SRC_R3D_CAMERA_H

#include <array>

#include "r3d_math.h"

namespace r3d
{

/// World <-> tile conversions. A tile (tx, ty, tz) occupies [tx, tx+1] x [ty, ty+1] x [tz, tz+1]
/// in tile space; world space flips y so that it is right-handed (see r3d_math.h).
inline auto tile_to_world( const vec3 &t ) -> vec3
{ return { .x = t.x, .y = -t.y, .z = t.z }; }
inline auto world_to_tile( const vec3 &w ) -> vec3
{ return { .x = w.x, .y = -w.y, .z = w.z }; }

struct ray {
    vec3 origin;
    vec3 dir; // normalized
};

/// Axis-aligned box in world space.
struct aabb {
    vec3 min;
    vec3 max;
};

/// Six frustum planes; a point p is inside when dot(n, p) + d >= 0 for every plane.
struct frustum {
    struct plane {
        vec3 n;
        float d = 0.0f;
    };
    std::array<plane, 6> planes;

    /// False only when the box is entirely outside one plane (conservative).
    auto intersects( const aabb &box ) const -> bool;
};

/// Orbit camera around a target point. yaw 0 puts the camera south of the target looking north;
/// increasing yaw swings it clockwise seen from above. pitch is the elevation above the horizon.
struct camera {
    static constexpr auto min_pitch = static_cast<float>( 0.17f );  // ~10 degrees
    static constexpr auto max_pitch = static_cast<float>
                                      ( 1.553f ); // ~89 degrees, avoids the look_at singularity
    static constexpr auto min_distance = static_cast<float>( 4.0f );
    static constexpr auto max_distance = static_cast<float>( 120.0f );

    vec3 target;               // world space, normally the centre of the viewed tile
    // Height of one z-level in tile widths. World space counts z in levels (a wall is 1 high);
    // the view stretches it by this so storeys look taller than a tile is wide. Everything the
    // camera returns in "stretched" space below has z multiplied by it; rays and matrices map
    // straight to and from (unstretched) world space.
    float z_scale = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.96f;       // ~55 degrees
    float distance = 18.0f;
    float fovy = 0.785f;       // 45 degrees
    bool orthographic = false;

    /// Camera position in stretched space (see z_scale).
    auto eye() const -> vec3;
    /// target in stretched space.
    auto stretched_target() const -> vec3;
    auto view() const -> mat4;
    auto projection( float aspect ) const -> mat4;
    auto view_projection( float aspect ) const -> mat4;
    auto view_frustum( float aspect ) const -> frustum;

    /// Ray through a point of the viewport; ndc_x/ndc_y are in [-1, 1] with y up. World space.
    auto screen_ray( float ndc_x, float ndc_y, float aspect ) const -> ray;

    /// Unit vector pointing right on screen, flattened onto the ground plane.
    auto ground_right() const -> vec3;

    /// How many 1/8 turns clockwise (on the map, y south) the camera's forward direction is from
    /// north, rounded: 0 when looking north, 2 east, 6 west. Turning a map direction by this
    /// makes "up" walk away from the camera.
    auto forward_eighths() const -> int;

    /// Orbit by steps of 45 degrees / tilt by steps of 5 degrees / scale distance by 10%.
    auto rotate_steps( int steps ) -> void;
    auto pitch_steps( int steps ) -> void;
    auto zoom_steps( int steps ) -> void;
};

/// Extracts the six clip planes from a view-projection matrix (depth range [0, 1]).
auto frustum_from_matrix( const mat4 &view_proj ) -> frustum;

} // namespace r3d

#endif // CATA_SRC_R3D_CAMERA_H
