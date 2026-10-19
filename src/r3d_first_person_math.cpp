#include "r3d_first_person_math.h"

#include <algorithm>
#include <cmath>

namespace r3d
{

namespace
{
constexpr auto pi = static_cast<float>( 3.14159265f );
constexpr auto eighth_turn = static_cast<float>( pi * 0.25f );
// A hair over 135 degrees: the rear diagonals are side alerts, not "behind" (see edge_alert_side).
constexpr auto behind_from = static_cast<float>( pi * 0.75f + 1e-3f );
} // namespace

auto signed_map_bearing( float dx, float dy ) -> float
{
    if( dx == 0.0f && dy == 0.0f ) {
        return 0.0f;
    }
    // North is -y on the map, so the bearing is the angle of (dx, -dy) from the +y axis.
    return std::atan2( dx, -dy );
}

auto wrap_bearing( float radians ) -> float
{
    auto a = static_cast<float>( std::fmod( radians, 2.0f * pi ) );
    if( a > pi ) {
        a -= 2.0f * pi;
    } else if( a <= -pi ) {
        a += 2.0f * pi;
    }
    return a;
}

auto relative_bearing( float dx, float dy, float view_yaw ) -> float { return wrap_bearing( signed_map_bearing( dx, dy ) - view_yaw ); }

auto heading_bearing( int heading ) -> float { return wrap_bearing( static_cast<float>( ( heading % 8 + 8 ) % 8 ) * eighth_turn ); }

auto compass_offset( float rel, float strip_width, float span ) -> float
{
    const auto half = static_cast<float>( span * 0.5f );
    return std::clamp( rel, -half, half ) / half * strip_width * 0.5f;
}

auto on_compass( float rel, float span ) -> bool { return std::abs( rel ) <= span * 0.5f + 1e-4f; }

auto edge_alert_side( float rel, float fovx ) -> edge_side
{
    const auto a = static_cast<float>( std::abs( rel ) );
    // Exactly on the picture's edge (a diagonal neighbour at a 90 degree view) half the sprite is
    // in the picture: that counts as seen, whatever the rounding of the bearing.
    if( a <= fovx * 0.5f + 1e-3f ) {
        return edge_side::none;
    }
    if( a >= behind_from ) {
        return edge_side::behind;
    }
    return rel < 0.0f ? edge_side::left : edge_side::right;
}

auto minimap_turn( int heading ) -> float { return -heading_bearing( heading ); }

auto slide_from_eye( const slide_from_eye_options &options ) -> float
{
    auto x = options.x;
    auto y = options.y;
    auto ex = options.ex;
    auto ey = options.ey;
    auto fx = options.fx;
    auto fy = options.fy;
    auto min_depth = options.min_depth;

    const auto depth = static_cast<float>( ( x - ex ) * fx + ( y - ey ) * fy );
    if( depth <= 0.0f || depth >= min_depth ) {
        return 0.0f;
    }
    return min_depth - depth;
}

auto turn_on_screen( const turn_on_screen_options &options ) -> void
{
    auto &x = options.x;
    auto &y = options.y;
    auto cx = options.cx;
    auto cy = options.cy;
    auto radians = options.radians;

    // With y pointing down, the usual rotation matrix turns clockwise as seen on screen.
    const auto c = static_cast<float>( std::cos( radians ) );
    const auto s = static_cast<float>( std::sin( radians ) );
    const auto dx = static_cast<float>( x - cx );
    const auto dy = static_cast<float>( y - cy );
    x = cx + dx * c - dy * s;
    y = cy + dx * s + dy * c;
}

} // namespace r3d
