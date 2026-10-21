#pragma once
#ifndef CATA_SRC_R3D_CAMERA_H
#define CATA_SRC_R3D_CAMERA_H

#include <array>
#include <cstdint>
#include <optional>

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

/// Supported presentation cameras.
enum class camera_mode : uint8_t {
    orbit,
    first_person
};

/// Orbit camera around a target point. yaw 0 puts the camera south of the target looking north;
/// increasing yaw swings it clockwise seen from above. pitch is the elevation above the horizon.
/// In first-person mode the same struct is an eye standing on the target (the point on the floor
/// under the viewer, in world space), facing one of the eight map directions given by heading.
/// yaw, pitch, distance and fovy are then unused; the fields below take their place.
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

    camera_mode mode = camera_mode::orbit;
    // First person only.
    static constexpr auto fp_near_plane = static_cast<float>( 0.05f );
    static constexpr auto fp_far_plane = static_cast<float>( 200.0f );
    static constexpr auto max_look_pitch = static_cast<float>( 1.2f ); // ~70 degrees
    static constexpr auto look_pitch_step = static_cast<float>( 0.2618f ); // 15 degrees
    int heading = 0;           // 1/8 turns clockwise from north, 0..7 (map convention, y south)
    float look_pitch = 0.0f;   // radians, positive looks up
    float eye_height = 0.8f;   // above the target, in stretched space (see z_scale)
    float glance_yaw = 0.0f;   // mouse glance, display only; snaps back to 0
    float glance_pitch = 0.0f;
    float tween_yaw = 0.0f;    // the turn still to be shown while a turn is eased, display only
    float tilt_pitch = 0.0f;   // looking at another level (look mode), display only
    float fovx = 1.5708f;      // horizontal field of view, 90 degrees
    std::optional<float> exact_fp_yaw;

    auto first_person() const -> bool {
        return mode == camera_mode::first_person;
    }
    /// First person: yaw of the view in world space (0 looks north, positive swings east).
    auto fp_yaw() const -> float;
    /// First person: unit view direction in stretched space.
    auto fp_forward() const -> vec3;
    /// Vertical field of view at the aspect ratio (width / height). fovx is the field of view of
    /// the narrower side: across a wide picture (so a wide terrain window does not become a
    /// fisheye vertically), and up and down a tall, narrow one (a 90 degree width there would be
    /// more than 100 degrees high).
    auto fp_fovy( float aspect ) const -> float;
    /// Horizontal field of view actually shown at the aspect ratio (see fp_fovy).
    auto fp_fovx( float aspect ) const -> float;

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

/// Nearest map heading, clockwise from north, for a tile offset.
auto eighths_towards( int dx, int dy ) -> int;

/// Extracts the six clip planes from a view-projection matrix (depth range [0, 1]).
auto frustum_from_matrix( const mat4 &view_proj ) -> frustum;

} // namespace r3d

#endif // CATA_SRC_R3D_CAMERA_H
