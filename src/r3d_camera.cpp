#include "r3d_camera.h"

#include <algorithm>
#include <cmath>

namespace r3d
{

namespace
{
constexpr auto rotate_step = static_cast<float>
                             ( 0.78539816f ); // 45 degrees, matching the eight movement directions
constexpr auto pitch_step = static_cast<float>( 0.0873f );  // 5 degrees
constexpr auto zoom_factor = static_cast<float>( 1.1f );
constexpr auto near_plane = static_cast<float>( 0.5f );
constexpr auto far_plane = static_cast<float>( 400.0f );
constexpr auto eighth_turn = static_cast<float>( 0.78539816f );
auto normalized_plane( const std::array<float, 4> &v ) -> frustum::plane
{
    auto pl = frustum::plane{};
    const auto n = vec3{ .x = v[0], .y = v[1], .z = v[2] };
    const auto len = static_cast<float>( length( n ) );
    const auto inv = static_cast<float>( len > 0.0f ? 1.0f / len : 1.0f );
    pl.n = n * inv;
    pl.d = v[3] * inv;
    return pl;
}

} // namespace

auto frustum::intersects( const aabb &box ) const -> bool
{
    for( const plane &pl : planes ) {
        // Positive vertex: the box corner furthest along the plane normal.
        const auto p = vec3{
            .x = pl.n.x >= 0.0f ? box.max.x : box.min.x,
            .y = pl.n.y >= 0.0f ? box.max.y : box.min.y,
            .z = pl.n.z >= 0.0f ? box.max.z : box.min.z };
        if( dot( pl.n, p ) + pl.d < 0.0f ) {
            return false;
        }
    }
    return true;
}

auto camera::stretched_target() const -> vec3
{ return { .x = target.x, .y = target.y, .z = target.z * z_scale }; }

auto camera::fp_yaw() const -> float
{ return static_cast<float>( heading ) * eighth_turn + glance_yaw + tween_yaw; }

auto camera::fp_forward() const -> vec3
{
    const auto yaw_w = static_cast<float>( fp_yaw() );
    const auto p = static_cast<float>( std::clamp( look_pitch + glance_pitch + tilt_pitch,
                                       -max_look_pitch,
                                       max_look_pitch ) );
    const auto cp = static_cast<float>( std::cos( p ) );
    return { std::sin( yaw_w ) *cp, std::cos( yaw_w ) *cp, std::sin( p ) };
}

auto camera::fp_fovy( float aspect ) const -> float
{
    if( aspect < 1.0f ) {
        return fovx;
    }
    return 2.0f * std::atan( std::tan( fovx * 0.5f ) / aspect );
}

auto camera::fp_fovx( float aspect ) const -> float
{
    if( aspect >= 1.0f ) {
        return fovx;
    }
    return 2.0f * std::atan( std::tan( fovx * 0.5f ) * std::max( aspect, 0.01f ) );
}

auto camera::eye() const -> vec3
{

    if( first_person() ) {
        return stretched_target() + vec3{ 0.0f, 0.0f, eye_height };
    }
    const auto cp = static_cast<float>( std::cos( pitch ) );
    return stretched_target() + vec3{ .x = std::sin( yaw ) * cp, .y = -std::cos( yaw ) * cp, .z = std::sin( pitch ) } *
           distance;
}

auto camera::view() const -> mat4
{
    auto stretch = mat4{};
    stretch.at( 2, 2 ) = z_scale;

    if( first_person() ) {
        const auto at = vec3( eye() );
        return look_at( at, at + fp_forward(), { 0.0f, 0.0f, 1.0f } ) * stretch;
    }
    return look_at( eye(), stretched_target(), { .x = 0.0f, .y = 0.0f, .z = 1.0f } ) * stretch;
}

auto camera::projection( float aspect ) const -> mat4
{

    if( first_person() ) {
        // Never orthographic; the near plane is close because the eye is half a tile from the
        // walls of its own tile.
        return perspective( { .fovy_rad = fp_fovy( aspect ), .aspect = aspect, .near_plane = fp_near_plane, .far_plane = fp_far_plane } );
    }
    if( orthographic ) {
        return r3d::orthographic( { .half_height = distance * std::tan( fovy * 0.5f ), .aspect = aspect, .near_plane = near_plane, .far_plane = far_plane } );
    }
    return perspective( { .fovy_rad = fovy, .aspect = aspect, .near_plane = near_plane, .far_plane = far_plane } );
}

auto camera::view_projection( float aspect ) const -> mat4
{ return projection( aspect ) * view(); }

auto camera::view_frustum( float aspect ) const -> frustum
{ return frustum_from_matrix( view_projection( aspect ) ); }

auto camera::screen_ray( float ndc_x, float ndc_y, float aspect ) const -> ray
{
    auto inverse = mat4{};
    if( !invert( view_projection( aspect ), inverse ) ) {
        return { .origin = target, .dir = { .x = 0.0f, .y = 0.0f, .z = -1.0f } };
    }
    // Depth range is [0, 1]: z = 0 is the near plane, z = 1 the far plane.
    const auto near_pt = vec3( transform_point( inverse, { .x = ndc_x, .y = ndc_y, .z = 0.0f } ) );
    const auto far_pt = vec3( transform_point( inverse, { .x = ndc_x, .y = ndc_y, .z = 1.0f } ) );
    // Both points come back in world space (the stretch is part of the matrix).
    return { .origin = near_pt, .dir = normalize( far_pt - near_pt ) };
}

auto camera::forward_eighths() const -> int
{
    // Forward in tile space (y south) is away from the eye, flattened.

    if( first_person() ) {
        return ( heading % 8 + 8 ) % 8;
    }
    // Forward in tile space (y south) is away from the eye, flattened.
    const auto eye_offset = vec3( eye() - stretched_target() );
    const auto fx = static_cast<float>( -eye_offset.x );
    const auto fy = static_cast<float>( eye_offset.y );
    if( fx == 0.0f && fy == 0.0f ) {
        return 0;
    }
    constexpr auto eighth = static_cast<float>( 0.78539816f );
    const auto turns = static_cast<int>( static_cast<int>( std::lround( std::atan2( fx,
                                         -fy ) / eighth ) ) );
    return ( turns % 8 + 8 ) % 8;
}

auto camera::ground_right() const -> vec3
{
    if( first_person() ) {
        const auto yaw_w = static_cast<float>( fp_yaw() );
        return { std::cos( yaw_w ), -std::sin( yaw_w ), 0.0f };
    }
    return { .x = std::cos( yaw ), .y = std::sin( yaw ), .z = 0.0f };
}

auto camera::rotate_steps( int steps ) -> void
{
    // Restore an aligned heading after a free mouse orbit as well.

    if( first_person() ) {
        heading = ( ( heading + steps ) % 8 + 8 ) % 8;
        return;
    }
    // Restore an aligned heading after a free mouse orbit as well.
    yaw = ( std::round( yaw / rotate_step ) + static_cast<float>( steps ) ) * rotate_step;
    constexpr auto two_pi = static_cast<float>( 6.2831853f );
    yaw = std::fmod( yaw, two_pi );
}

auto camera::pitch_steps( int steps ) -> void
{

    if( first_person() ) {
        look_pitch = std::clamp( look_pitch + look_pitch_step * static_cast<float>( steps ),
                                 -max_look_pitch, max_look_pitch );
        return;
    }
    pitch = std::clamp( pitch + pitch_step * static_cast<float>( steps ), min_pitch, max_pitch );
}

auto camera::zoom_steps( int steps ) -> void
{
    // Positive steps move the camera closer.

    if( first_person() ) {
        return;
    }
    // Positive steps move the camera closer.
    distance = std::clamp( distance * std::pow( 1.0f / zoom_factor, static_cast<float>( steps ) ),
                           min_distance, max_distance );
}

auto eighths_towards( int dx, int dy ) -> int
{
    if( dx == 0 && dy == 0 ) {
        return 0;
    }
    const auto turns = static_cast<int>( static_cast<int>( std::lround( std::atan2( static_cast<float>
                                         ( dx ),
                                         static_cast<float>( -dy ) ) / eighth_turn ) ) );
    return ( turns % 8 + 8 ) % 8;
}

auto frustum_from_matrix( const mat4 &m ) -> frustum
{
    // Gribb/Hartmann extraction, adapted for a [0, 1] depth range: the near plane is row 2 alone
    // rather than row 3 + row 2.
    auto row = [&m]( int r ) -> std::array<float, 4> {
        return std::array<float, 4> { m.at( r, 0 ), m.at( r, 1 ), m.at( r, 2 ), m.at( r, 3 ) };
    };
    const auto r0 = std::array<float, 4>( row( 0 ) );
    const auto r1 = std::array<float, 4>( row( 1 ) );
    const auto r2 = std::array<float, 4>( row( 2 ) );
    const auto r3 = std::array<float, 4>( row( 3 ) );

    auto add = []( const std::array<float, 4> &a,
    const std::array<float, 4> &b ) -> std::array<float, 4> {
        return std::array<float, 4> { a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3] };
    };
    auto sub = []( const std::array<float, 4> &a,
    const std::array<float, 4> &b ) -> std::array<float, 4> {
        return std::array<float, 4> { a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3] };
    };
    auto f = frustum{};
    f.planes[0] = normalized_plane( add( r3, r0 ) ); // left
    f.planes[1] = normalized_plane( sub( r3, r0 ) ); // right
    f.planes[2] = normalized_plane( add( r3, r1 ) ); // bottom
    f.planes[3] = normalized_plane( sub( r3, r1 ) ); // top
    f.planes[4] = normalized_plane( r2 );            // near
    f.planes[5] = normalized_plane( sub( r3, r2 ) ); // far
    return f;
}

} // namespace r3d
