#include "r3d_hud_math.h"

#include <algorithm>
#include <cmath>

namespace r3d
{

namespace
{
constexpr auto pi = static_cast<float>( 3.14159265f );
constexpr auto two_pi = static_cast<float>( 6.2831853f );

auto lerp( float a, float b, float t ) -> float
{ return a + ( b - a ) * t; }

auto wrap_bearing( float b ) -> float
{
    b = std::fmod( b, two_pi );
    return b < 0.0f ? b + two_pi : b;
}

// Repeatable pseudo-random number in [0, 1) for a particle and a channel.
auto hash01( uint32_t index, uint32_t channel ) -> float
{
    auto h = static_cast<uint32_t>( index * 0x9e3779b1u ^ ( channel + 1u ) * 0x85ebca77u );
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    h *= 0x297a2d39u;
    h ^= h >> 15;
    return static_cast<float>( h >> 8 ) / 16777216.0f;
}

// x wrapped into [lo, lo + span).
auto wrap( float x, float lo, float span ) -> float
{
    const auto t = static_cast<float>( std::fmod( x - lo, span ) );
    return lo + ( t < 0.0f ? t + span : t );
}
} // namespace

auto sky_color( float daylight, float cloud, bool lightning ) -> rgb
{
    daylight = std::clamp( daylight, 0.0f, 1.0f );
    cloud = std::clamp( cloud, 0.0f, 1.0f );
    if( lightning ) {
        return { .r = 0.85f, .g = 0.87f, .b = 0.95f };
    }
    // Night is a deep blue, not black, so the sky still reads as sky around the map.
    const auto night = rgb{ .r = 0.03f, .g = 0.04f, .b = 0.09f };
    const auto day = rgb{ .r = 0.42f, .g = 0.62f, .b = 0.86f };
    // Twilight warms the colour while the sun is low.
    const auto dusk = static_cast<float>( std::clamp( 1.0f - std::fabs( daylight - 0.25f ) / 0.2f, 0.0f,
                                          1.0f ) );
    auto c = rgb{ .r = lerp( night.r, day.r, daylight ), .g = lerp( night.g, day.g, daylight ), .b = lerp( night.b, day.b, daylight ) };
    c.r = lerp( c.r, 0.62f, dusk * 0.45f );
    c.g = lerp( c.g, 0.42f, dusk * 0.3f );
    // Cloud turns the colour grey and darker.
    const auto grey = static_cast<float>( ( c.r + c.g + c.b ) / 3.0f * ( 1.0f - 0.55f * cloud ) );
    return { .r = lerp( c.r, grey, cloud ), .g = lerp( c.g, grey, cloud ), .b = lerp( c.b, grey, cloud ) };
}

auto sky_direction( float azimuth, float altitude ) -> vec3
{
    // Azimuth 0 is south, positive towards the west (see sunlight_angle in calendar.cpp).
    const auto ca = static_cast<float>( std::cos( altitude ) );
    return { .x = -std::sin( azimuth ) * ca, .y = -std::cos( azimuth ) * ca, .z = std::sin( altitude ) };
}

auto wind_velocity( int direction_deg, float speed ) -> vec3
{
    // The wind comes from direction_deg, so it blows the opposite way.
    const auto a = static_cast<float>( static_cast<float>( direction_deg ) * pi / 180.0f );
    return { .x = -std::sin( a ) * speed, .y = -std::cos( a ) * speed, .z = 0.0f };
}

auto particle_position( const particle_position_options &options ) -> vec3
{
    auto index = options.index;
    auto seconds = options.seconds;
    const auto &field = options.field;
    const auto &world_offset = options.world_offset;

    const auto span = static_cast<float>( std::max( field.half_extent * 2.0f, 0.001f ) );
    const auto height = static_cast<float>( std::max( field.height, 0.001f ) );
    // Each particle falls at its own speed (+-25%) so they do not move as one sheet.
    const auto speed = static_cast<float>( 0.75f + 0.5f * hash01( index, 3 ) );
    const auto x = static_cast<float>( hash01( index, 0 ) * span + field.wind.x * speed * seconds );
    const auto y = static_cast<float>( hash01( index, 1 ) * span + field.wind.y * speed * seconds );
    const auto z = static_cast<float>( hash01( index,
                                       2 ) * height - field.fall_speed * speed * seconds );
    return { .x = wrap( x, world_offset.x - field.half_extent, span ) - world_offset.x, .y = wrap( y, world_offset.y - field.half_extent, span ) - world_offset.y, .z = wrap( z, 0.0f, height ) };
}

auto fog_density( float sight_penalty ) -> float
{ return std::max( sight_penalty - 1.0f, 0.0f ) * 0.06f; }

auto fog_amount( float distance, float density ) -> float
{
    const auto d = static_cast<float>( distance * density );
    return 1.0f - std::exp( -d * d );
}

auto map_bearing( float dx, float dy ) -> float
{
    if( dx == 0.0f && dy == 0.0f ) {
        return 0.0f;
    }
    return wrap_bearing( std::atan2( dx, -dy ) );
}

auto camera_forward_bearing( float yaw ) -> float
{
    // camera::eye() sits at world ( sin yaw, -cos yaw ) from the target and looks back at it.
    return wrap_bearing( -yaw );
}

auto bearing_delta( float from, float to ) -> float
{
    auto d = static_cast<float>( std::fmod( to - from, two_pi ) );
    if( d <= -pi ) {
        d += two_pi;
    } else if( d > pi ) {
        d -= two_pi;
    }
    return d;
}

auto place_edge_mark( const place_edge_mark_options &options ) -> edge_mark
{
    auto clip_x = options.clip_x;
    auto clip_y = options.clip_y;
    auto clip_w = options.clip_w;
    auto margin_x = options.margin_x;
    auto margin_y = options.margin_y;

    auto m = edge_mark{};
    auto x = static_cast<float>( 0.0f );
    auto y = static_cast<float>( 0.0f );
    if( clip_w > 1e-6f ) {
        x = clip_x / clip_w;
        y = clip_y / clip_w;
    } else {
        // Behind the camera the projection flips; mirror back so the mark points the way to
        // turn. Something straight behind is shown at the bottom.
        x = -clip_x;
        y = -clip_y;
        if( std::fabs( x ) < 1e-6f && std::fabs( y ) < 1e-6f ) {
            y = -1.0f;
        }
    }
    const auto lim_x = static_cast<float>( std::max( 1.0f - margin_x, 0.01f ) );
    const auto lim_y = static_cast<float>( std::max( 1.0f - margin_y, 0.01f ) );
    if( clip_w > 1e-6f && std::fabs( x ) <= lim_x && std::fabs( y ) <= lim_y ) {
        m.on_screen = true;
        m.x = x;
        m.y = y;
        return m;
    }
    const auto tx = static_cast<float>( std::fabs( x ) > 1e-6f ? lim_x / std::fabs( x ) : 1e9f );
    const auto ty = static_cast<float>( std::fabs( y ) > 1e-6f ? lim_y / std::fabs( y ) : 1e9f );
    const auto t = static_cast<float>( std::min( tx, ty ) );
    m.x = x * t;
    m.y = y * t;
    m.angle = std::atan2( y, x );
    return m;
}

} // namespace r3d
