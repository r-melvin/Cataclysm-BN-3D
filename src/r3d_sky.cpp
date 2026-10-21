#if defined(TILES)

#include "r3d_sky.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "avatar.h"
#include "game.h"
#include "calendar.h"
#include "messages.h"
#include "translations.h"
#include "r3d_gpu.h"
#include "weather/weather.h"
#include "weather/weather_type.h"

namespace r3d
{

namespace
{

constexpr auto cell = static_cast<int>( 32 );

struct painter {
    SDL_Surface *surface;

    struct put_options {
        int x;
        int y;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a = 255;
    };

    auto put( const put_options &options ) -> void {
        auto x = options.x;
        auto y = options.y;
        auto r = options.r;
        auto g = options.g;
        auto b = options.b;
        auto a = options.a;

        if( x < 0 || y < 0 || x >= surface->w || y >= surface->h ) {
            return;
        }
        const auto px = static_cast<uint8_t *>( surface->pixels ) + y * surface->pitch + x *
                        4;
        px[0] = r;
        px[1] = g;
        px[2] = b;
        px[3] = a;
    }
};

// Unit disc coordinates of a pixel of a cell: x right, y up, 1 at the rim (with a margin).
struct disc_coords_options {
    int px;
    int py;
    float &x;
    float &y;
};

auto disc_coords( const disc_coords_options &options ) -> void
{
    auto px = options.px;
    auto py = options.py;
    auto &x = options.x;
    auto &y = options.y;

    const auto half = static_cast<float>( cell * 0.5f );
    x = ( static_cast<float>( px ) + 0.5f - half ) / ( half - 2.0f );
    y = ( half - static_cast<float>( py ) - 0.5f ) / ( half - 2.0f );
}

auto paint_sun( painter &p, int ox ) -> void
{
    for( auto py = static_cast<int>( 0 ); py < cell; ++py ) {
        for( auto px = static_cast<int>( 0 ); px < cell; ++px ) {
            auto x = static_cast<float>( 0.0f );
            auto y = static_cast<float>( 0.0f );
            disc_coords( { .px = px, .py = py, .x = x, .y = y } );
            const auto r2 = static_cast<float>( x * x + y * y );
            if( r2 <= 1.0f ) {
                // Brighter in the middle, warm at the rim.
                const auto g = uint8_t( static_cast<uint8_t>( 255.0f - 60.0f * r2 ) );
                const auto b = uint8_t( static_cast<uint8_t>( 200.0f - 120.0f * r2 ) );
                p.put( { .x = ox + px, .y = py, .r = 255, .g = g, .b = b } );
            }
        }
    }
}

// Phase 0 is the new moon, 4 the full moon; 1-3 wax (lit on the right), 5-7 wane.
auto paint_moon( painter &p, int ox, int phase ) -> void
{
    constexpr auto quarter = static_cast<float>( 0.78539816f );
    const auto angle = static_cast<float>( static_cast<float>( phase ) * quarter );
    const auto waxing = static_cast<bool>( phase <= 4 );
    for( auto py = static_cast<int>( 0 ); py < cell; ++py ) {
        for( auto px = static_cast<int>( 0 ); px < cell; ++px ) {
            auto x = static_cast<float>( 0.0f );
            auto y = static_cast<float>( 0.0f );
            disc_coords( { .px = px, .py = py, .x = x, .y = y } );
            const auto r2 = static_cast<float>( x * x + y * y );
            if( r2 > 1.0f ) {
                continue;
            }
            // The terminator is an ellipse: x = cos( angle ) * sqrt( 1 - y^2 ).
            const auto edge = static_cast<float>( std::cos( angle ) * std::sqrt( std::max( 0.0f,
                                                  1.0f - y * y ) ) );
            const auto lit = static_cast<bool>( waxing ? x >= edge : -x >= std::cos( 6.2831853f - angle ) *
                                                std::sqrt( std::max( 0.0f, 1.0f - y * y ) ) );
            if( lit ) {
                const auto v = uint8_t( static_cast<uint8_t>( 235.0f - 30.0f * r2 ) );
                p.put( { .x = ox + px, .y = py, .r = v, .g = v, .b = static_cast<uint8_t>( v * 0.92f ) } );
            } else {
                // The dark part stays faintly visible, so a new moon is still a disc.
                p.put( { .x = ox + px, .y = py, .r = 38, .g = 42, .b = 58 } );
            }
        }
    }
}

auto paint_rain( painter &p, int ox ) -> void
{
    // A thin slanted streak.
    for( auto py = static_cast<int>( 2 ); py < cell - 2; ++py ) {
        const auto x = static_cast<int>( cell / 2 + ( cell / 2 - py ) / 6 );
        p.put( { .x = ox + x, .y = py, .r = 170, .g = 190, .b = 230 } );
        p.put( { .x = ox + x + 1, .y = py, .r = 150, .g = 170, .b = 220 } );
    }
}

auto paint_snow( painter &p, int ox ) -> void
{
    // A six-armed flake.
    const auto half = static_cast<float>( cell * 0.5f );
    for( auto arm = static_cast<int>( 0 ); arm < 6; ++arm ) {
        const auto a = static_cast<float>( static_cast<float>( arm ) * 1.0471976f );
        for( auto t = static_cast<float>( 0.0f ); t < half - 3.0f; t += 0.5f ) {
            const auto x = static_cast<int>( static_cast<int>( half + std::cos( a ) * t ) );
            const auto y = static_cast<int>( static_cast<int>( half + std::sin( a ) * t ) );
            p.put( { .x = ox + x, .y = y, .r = 245, .g = 248, .b = 255 } );
            p.put( { .x = ox + x + 1, .y = y, .r = 245, .g = 248, .b = 255 } );
        }
    }
    for( auto dy = static_cast<int>( -2 ); dy <= 2; ++dy ) {
        for( auto dx = static_cast<int>( -2 ); dx <= 2; ++dx ) {
            p.put( { .x = ox + cell / 2 + dx, .y = cell / 2 + dy, .r = 255, .g = 255, .b = 255 } );
        }
    }
}

} // namespace

auto make_sky_sprites( gpu &device ) -> sky_sprites
{
    auto out = sky_sprites{};
    const auto surface = SDL_CreateSurface( cell * sky_sprites::count, cell,
                                            SDL_PIXELFORMAT_RGBA32 );
    if( !surface ) {
        return out;
    }
    SDL_FillSurfaceRect( surface, nullptr, 0 );
    auto p = painter{ .surface = surface };
    paint_sun( p, sky_sprites::sun * cell );
    for( auto phase = static_cast<int>( 0 ); phase < 8; ++phase ) {
        paint_moon( p, ( sky_sprites::moon + phase ) * cell, phase );
    }
    paint_rain( p, sky_sprites::rain * cell );
    paint_snow( p, sky_sprites::snow * cell );
    out.sheet = device.create_sheet( surface );
    SDL_DestroySurface( surface );
    if( !out.sheet ) {
        return out;
    }
    const auto w = static_cast<float>( static_cast<float>( cell * sky_sprites::count ) );
    for( auto i = static_cast<int>( 0 ); i < sky_sprites::count; ++i ) {
        auto &r = out.cells[static_cast<size_t>( i )];
        r.sheet = out.sheet.get();
        r.owner = out.sheet;
        // Half a texel inside the cell so filtering never reads the neighbour.
        r.u0 = ( static_cast<float>( i * cell ) + 0.5f ) / w;
        r.u1 = ( static_cast<float>( ( i + 1 ) * cell ) - 0.5f ) / w;
        r.v0 = 0.5f / cell;
        r.v1 = 1.0f - 0.5f / cell;
        r.valid = true;
    }
    return out;
}

auto sky_time() -> time_point
{
    // Debug aid for screenshots: CATA_R3D_TIME_SHIFT=<hours> draws the sky that many hours
    // later than the game's time. Nothing else is affected.
    static const auto shift = static_cast<int>( []() {
        const auto env = std::getenv( "CATA_R3D_TIME_SHIFT" );
        return env ? std::atoi( env ) : 0;
    }
    () );
    return calendar::turn + time_duration::from_hours( shift );
}

auto current_atmosphere( int view_z, bool with_weather ) -> atmosphere
{
    auto a = atmosphere{};
    const auto full = static_cast<float>( std::max( float( default_daylight_level() ), 1.0f ) );
    a.daylight = std::clamp( sunlight( sky_time() ) / full, 0.0f, 1.0f );
    const auto &weather = get_weather();
    // Debug aid for screenshots: CATA_R3D_WEATHER=<weather type id> draws that weather instead
    // of the current one. Nothing else is affected.
    static const auto forced_env = std::getenv( "CATA_R3D_WEATHER" );
    const auto weather_id = weather_type_id( forced_env ? weather_type_id( forced_env ) :
                            weather.weather_id );
    const auto weather_here = static_cast<bool>( with_weather && view_z >= 0 && weather_id.is_valid() &&
                              weather_id != weather_type_id( "weather_null" ) );
    if( weather_here ) {
        const auto &w = *weather_id;
        // Cloud from how much the weather takes off the ambient light (fog: -40, storms -45).
        a.cloud = std::clamp( -static_cast<float>( w.light_modifier ) / 45.0f, 0.0f, 1.0f );
        a.fog_density = fog_density( w.sight_penalty );
        a.particle_id = w.animation.tile;
        a.snow = !w.rains && w.precip != precip_class::none;
        switch( w.precip ) {
            case precip_class::very_light:
                a.particles = 140;
                break;
            case precip_class::light:
                a.particles = 320;
                break;
            case precip_class::heavy:
                a.particles = 700;
                break;
            default:
                a.particles = 0;
                break;
        }
        if( w.sound_category == weather_sound_category::rainy ||
            w.sound_category == weather_sound_category::thunder ||
            w.sound_category == weather_sound_category::snowstorm ) {
            a.particles = 1000;
        }
        if( a.particles > 0 && a.particle_id.empty() ) {
            a.particle_id = a.snow ? "weather_snowflake" : "weather_rain_drop";
        }
        // Wind in miles per hour; one tile is about a metre.
        constexpr auto mph_to_tiles_per_second = static_cast<float>( 0.447f );
        a.wind = wind_velocity( weather.winddirection,
                                static_cast<float>( weather.windspeed ) * mph_to_tiles_per_second );
    }
    if( view_z < 0 ) {
        // Underground there is no sky; keep the near-black the view always had.
        a.sky = { 0.05f, 0.05f, 0.07f };
    } else {
        a.sky = sky_color( a.daylight, a.cloud, weather_here && weather.lightning_active );
    }
    // Fog is lighter than the sky by day (mist glows) and just as dark at night.
    const auto grey = static_cast<float>( ( a.sky.r + a.sky.g + a.sky.b ) / 3.0f );
    const auto lift = static_cast<float>( 0.15f * a.daylight );
    a.fog_color = { std::min( 1.0f, ( a.sky.r + grey ) * 0.5f + lift ),
                    std::min( 1.0f, ( a.sky.g + grey ) * 0.5f + lift ),
                    std::min( 1.0f, ( a.sky.b + grey ) * 0.5f + lift )
                  };
    return a;
}

auto sky_bodies( int view_z, const atmosphere &air ) -> std::vector<sky_body>
{
    auto out = std::vector<sky_body> {};
    if( view_z < 0 || air.cloud > 0.4f ) {
        return out;
    }
    // Match BN's daylight arc: east at sunrise, overhead at solar noon, west at sunset.
    const auto when = sky_time();
    const auto rise = sunrise( when );
    const auto set = sunset( when );
    const auto day_seconds = std::max( 1.0, to_seconds<double>( set - rise ) );
    const auto progress = to_seconds<double>( when - rise ) / day_seconds;
    const auto theta = float( progress ) * 3.14159265f;
    const auto altitude = std::asin( std::sin( theta ) );
    const auto azimuth = std::cos( theta ) >= 0 ? -1.57079633f : 1.57079633f;
    if( altitude > -0.05f ) {
        out.push_back( { .dir = sky_direction( azimuth, altitude ), .cell = sky_sprites::sun, .sun = true } );
    }
    // The sundial puts the moon opposite the sun; it is up when the sun is down.
    if( -altitude > -0.05f ) {
        const auto phase = static_cast<int>( static_cast<int>( get_moon_phase( sky_time() ) ) );
        out.push_back( { .dir = sky_direction( azimuth + 3.14159265f, -altitude ),
                         .cell = sky_sprites::moon + std::clamp( phase, 0, 7 ), .sun = false } );
    }
    return out;
}

auto report_weather_changes() -> void
{
    static auto last_weather = weather_type_id( "weather_null" );
    static auto last_band = -1;
    static auto last_turn = calendar::before_time_starts;
    const auto &weather = get_weather();
    const auto &you = get_avatar();
    // Check once per turn. Anything but the next turns (a loaded save, 3D switched off for a while)
    // only records the current state: the character did not watch it change.
    if( calendar::turn == last_turn ) { return; }
    const auto watched = calendar::turn > last_turn && calendar::turn - last_turn <= 1_hours;
    last_turn = calendar::turn;
    if( !watched ) {
        last_weather = weather_type_id( "weather_null" );
        last_band = -1;
    }
    // Only what the character can tell: the weather overhead, from outdoors or a window.
    if( you.bub_pos().z() < 0 || !!g->is_sheltered( you.bub_pos() ) ) {
        return;
    }
    if( weather.weather_id != last_weather && weather.weather_id.is_valid() &&
        weather.weather_id != weather_type_id( "weather_null" ) ) {
        if( last_weather != weather_type_id( "weather_null" ) ) {
            add_msg( m_info, _( "The weather turns: %s." ), weather.weather_id->name );
        }
        last_weather = weather.weather_id;
    }
    const auto band = wind_band( weather.windspeed, last_band );
    if( band != last_band ) {
        if( last_band >= 0 ) {
            if( band == 0 ) {
                add_msg( m_info, _( "The wind dies down." ) );
            } else if( band > last_band ) {
                add_msg( m_info, _( "The wind picks up from the %s." ),
                         get_dirstring( weather.winddirection ) );
            } else {
                add_msg( m_info, _( "The wind eases, blowing from the %s." ),
                         get_dirstring( weather.winddirection ) );
            }
        }
        last_band = band;
    }
}

} // namespace r3d

#endif // defined(TILES)
