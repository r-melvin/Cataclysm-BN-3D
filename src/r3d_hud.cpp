#if defined(TILES)

#include "r3d_hud.h"
#include "r3d_renderer_state.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <tuple>

#include <SDL3/SDL.h>

#include "avatar.h"
#include "color.h"
#include "cursesdef.h"
#include "calendar.h"
#include "cata_utility.h"
#include "creature.h"
#include "cursesport.h"
#include "r3d_display.h"
#include "event.h"
#include "event_bus.h"
#include "game.h"
#include "line.h"
#include "map/map.h"
#include "overmap/overmap_ui.h"
#include "overmap/overmapbuffer.h"
#include "options.h"
#include "output.h"
#include "r3d_hud_math.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "weather/weather.h"

namespace r3d
{

auto hud_frame::clip( const vec3 &tile, float &w_out ) const -> vec3
{
    const auto ndc = vec3( transform_point( view_proj, tile_to_world( tile ), &w_out ) );
    return { ndc.x * w_out, ndc.y * w_out, ndc.z * w_out };
}

auto hud_frame::screen( const vec3 &tile ) const -> std::optional<vec3>
{
    auto w_clip = static_cast<float>( 0.0f );
    const auto ndc = vec3( transform_point( view_proj, tile_to_world( tile ), &w_clip ) );
    if( w_clip <= 1e-4f ) {
        return std::nullopt;
    }
    return vec3{ .x = x + ( ndc.x * 0.5f + 0.5f ) * w, .y = y + ( 0.5f - ndc.y * 0.5f ) * h, .z = ndc.z };
}

auto hud_frame::tile_point( const tripoint_bub_ms &p, float height ) const -> vec3
{
    return { static_cast<float>( p.x() - center.x() ) + 0.5f,
             static_cast<float>( p.y() - center.y() ) + 0.5f,
             static_cast<float>( p.z() - center.z() ) + height };
}

auto hud_frame::screen( const tripoint_bub_ms &p, float height ) const -> std::optional<vec3>
{ return screen( tile_point( p, height ) ); }

namespace
{

// Same colours as the flat view's scrolling combat text (msgtype_to_tilecolor in cata_tiles.cpp).
auto message_color( game_message_type type, bool old ) -> int
{
    const auto bright = static_cast<int>( old ? 0 : 8 );
    switch( type ) {
        case m_good:
            return bright + catacurses::green;
        case m_bad:
            return bright + catacurses::red;
        case m_mixed:
        case m_headshot:
            return bright + catacurses::magenta;
        case m_warning:
        case m_critical:
            return bright + catacurses::yellow;
        case m_info:
        case m_grazing:
            return bright + catacurses::blue;
        case m_neutral:
        default:
            return bright + catacurses::white;
    }
}

// Scrolling combat text over the 3D picture. The flat view moves it across tiles; here it
// rises from the head of whoever it is about, and texts to the side (damage numbers to the right,
// health to the left) keep their side.
auto add_combat_text( const hud_frame &frame, std::vector<hud_text> &texts ) -> void
{
    if( !frame.sct ) {
        return;
    }
    const auto z = static_cast<int>( get_avatar().bub_pos().z() );
    constexpr auto head_height = static_cast<float>( 1.1f );
    constexpr auto rise_per_step = static_cast<float>( 7.0f ); // logical pixels
    for( const scrollingcombattext::cSCT &sct : SCT.vSCT ) {
        const auto at = tripoint_bub_ms( sct.getInitPosX(), sct.getInitPosY(), z );
        const auto p = std::optional<vec3>( frame.screen( at, head_height ) );
        if( !p ) {
            continue;
        }
        const auto steps = static_cast<int>( sct.getStep() + sct.getStepOffset() );
        const auto old = static_cast<bool>( sct.getStep() >= scrollingcombattext::iMaxSteps / 2 );
        const auto side = point( ( sct.getPosX() > sct.getInitPosX() ) - ( sct.getPosX() <
                                 sct.getInitPosX() ), 0 );
        const auto sx = static_cast<float>( p->x + static_cast<float>( side.x ) * 18.0f );
        const auto sy = static_cast<float>( p->y - static_cast<float>( steps ) * rise_per_step );
        if( sx < frame.x || sx > frame.x + frame.w || sy < frame.y || sy > frame.y + frame.h ) {
            continue;
        }
        const auto first = std::string( sct.getText( "first" ) );
        const auto second = std::string( sct.getText( "second" ) );
        const auto align = static_cast<int>( side.x > 0 ? -1 : side.x < 0 ? 1 : 0 );
        if( second.empty() ) {
            texts.push_back( { .x = sx, .y = sy, .text = first, .color = message_color( sct.getMsgType( "first" ), old ), .align = align, .boxed = false } );
        } else {
            // Two coloured parts on one line: the second starts where the first ends (the caller
            // measures text in cells, so split around x).
            texts.push_back( { .x = sx, .y = sy, .text = first, .color = message_color( sct.getMsgType( "first" ), old ), .align = 1, .boxed = false } );
            texts.push_back( { .x = sx, .y = sy, .text = second, .color = message_color( sct.getMsgType( "second" ), old ), .align = -1, .boxed = false } );
        }
    }
}

struct rgba8 {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

auto fill_triangles( SDL_Renderer *renderer, const std::vector<SDL_FPoint> &points,
                     rgba8 c ) -> void
{
    if( points.size() < 3 ) {
        return;
    }
    auto v = std::vector<SDL_Vertex>( points.size() );
    const auto color = SDL_FColor{ .r = c.r / 255.0f, .g = c.g / 255.0f, .b = c.b / 255.0f, .a = c.a / 255.0f };
    for( auto i = size_t{ 0 }; i < points.size(); ++i ) {
        v[i].position = points[i];
        v[i].color = color;
        v[i].tex_coord = SDL_FPoint{ .x = 0.0f, .y = 0.0f };
    }
    SDL_RenderGeometry( renderer, nullptr, v.data(), static_cast<int>( v.size() ), nullptr, 0 );
}

struct draw_arrow_head_options {
    SDL_Renderer *renderer;
    float x;
    float y;
    float angle;
    float s;
    rgba8 col;
};

auto draw_arrow_head( const draw_arrow_head_options &options ) -> void
{
    auto renderer = options.renderer;
    auto x = options.x;
    auto y = options.y;
    auto angle = options.angle;
    auto s = options.s;
    auto col = options.col;

    const auto ca = static_cast<float>( std::cos( angle ) );
    const auto sa = static_cast<float>( std::sin( angle ) );
    const auto at = [ x, y, ca, sa ]( float fx, float fy ) {
        return SDL_FPoint{ .x = x + fx * ca - fy * sa, .y = y + fx * sa + fy * ca };
    };
    fill_triangles( renderer, { at( s, 0.0f ), at( -s * 0.6f, s * 0.7f ), at( -s * 0.6f, -s * 0.7f ) },
                    col );
}

// An arrowhead at (x, y) pointing along angle (screen radians, y down), with a dark outline so
// it reads on any picture.
struct draw_arrow_options {
    SDL_Renderer *renderer;
    float x;
    float y;
    float angle;
    float size;
    rgba8 c;
};

auto draw_arrow( const draw_arrow_options &options ) -> void
{
    auto renderer = options.renderer;
    auto x = options.x;
    auto y = options.y;
    auto angle = options.angle;
    auto size = options.size;
    auto c = options.c;

    draw_arrow_head( { .renderer = renderer, .x = x, .y = y, .angle = angle, .s = size + 2.0f, .col = rgba8{ .r = 0, .g = 0, .b = 0, .a = 200 } } );
    draw_arrow_head( { .renderer = renderer, .x = x, .y = y, .angle = angle, .s = size, .col = c } );
}

// Pointers at the edge of the view towards hostiles the player sees and sounds the player heard
// that are outside the picture. One arrow per direction (bins of 1/32 turn), the nearest wins.
auto draw_edge_arrows( SDL_Renderer *renderer, const hud_frame &frame ) -> void
{
    const auto &you = get_avatar();
    struct mark {
        edge_mark m;
        bool hostile = false;
        int dist = 0;
        int dz = 0;
    };
    auto marks = std::vector<mark> {};
    // Margins: the arrows sit a little inside the picture.
    const auto margin_x = static_cast<float>( 24.0f / std::max( frame.w, 1.0f ) * 2.0f );
    const auto margin_y = static_cast<float>( 24.0f / std::max( frame.h, 1.0f ) * 2.0f );
    const auto consider = [ &frame, margin_x, margin_y, &marks, &you ]( const tripoint_bub_ms & p,
    bool hostile ) {
        auto w_clip = static_cast<float>( 0.0f );
        const auto c = vec3( frame.clip( frame.tile_point( p, 0.5f ), w_clip ) );
        const auto m = edge_mark( place_edge_mark( { .clip_x = c.x, .clip_y = c.y, .clip_w = w_clip, .margin_x = margin_x, .margin_y = margin_y } ) );
        if( !m.on_screen ) {
            marks.push_back( { .m = m, .hostile = hostile, .dist = rl_dist( you.bub_pos(), p ), .dz = p.z() - you.bub_pos().z() } );
        }
    };
    for( Creature &critter : g->all_creatures() ) {
        if( critter.is_avatar() || critter.attitude_to( you ) != Attitude::A_HOSTILE ||
            !you.sees( critter ) ) {
            continue;
        }
        consider( critter.bub_pos(), true );
    }
    for( const tripoint_bub_ms &p : sounds::get_footstep_markers() ) {
        consider( p, false );
    }
    if( marks.empty() ) {
        return;
    }
    // Hostiles first, then the nearest, so each direction shows the most urgent thing.
    std::ranges::sort( marks, []( const mark & a, const mark & b ) {
        return a.hostile != b.hostile ? a.hostile : a.dist < b.dist;
    } );
    constexpr auto bins = static_cast<int>( 32 );
    auto used = std::array<bool, bins> {};
    for( const mark &k : marks ) {
        const auto turn = static_cast<float>( k.m.angle / 6.2831853f + 0.5f );
        const auto bin = static_cast<int>( std::clamp( static_cast<int>( turn * bins ), 0, bins - 1 ) );
        if( used[static_cast<size_t>( bin )] ) {
            continue;
        }
        used[static_cast<size_t>( bin )] = true;
        const auto sx = static_cast<float>( frame.x + ( k.m.x * 0.5f + 0.5f ) * frame.w );
        const auto sy = static_cast<float>( frame.y + ( 0.5f - k.m.y * 0.5f ) * frame.h );
        // Closer things get bigger arrows; another level dims the arrow.
        const auto size = static_cast<float>( std::clamp( 14.0f - static_cast<float>( k.dist ) * 0.15f,
                                              7.0f, 14.0f ) );
        auto c = rgba8( k.hostile ? rgba8{ .r = 235, .g = 50, .b = 40, .a = 255 } :
                        rgba8{ .r = 250, .g = 210, .b = 60, .a = 255 } );
        if( k.dz != 0 ) {
            c.a = 170;
        }
        // NDC y is up, the screen's is down.
        draw_arrow( { .renderer = renderer, .x = sx, .y = sy, .angle = -k.m.angle, .size = size, .c = c } );
    }
}

// Curses colour index (0-15) of an nc_color, as the curses port draws it.
auto tile_color( const nc_color &c ) -> int
{
    auto fg = static_cast<int>( cata_cursesport::colorpairs[static_cast<size_t>
                                ( c.to_color_pair_index() )].FG );
    if( c.is_bold() ) {
        fg += 8;
    }
    return fg;
}

// Overmap notes near the player, refreshed when the turn or the overmap tile changes.
struct near_note {
    tripoint_abs_omt pos;
    char symbol = 'N';
    int color = 7;
    int dist = 0;
};

auto nearby_notes( const tripoint_abs_omt &here_omt ) -> const std::vector<near_note> & // *NOPAD*
{
    static auto notes = std::vector<near_note> {};
    static auto last_turn = time_point( calendar::before_time_starts );
    static auto last_pos = tripoint_abs_omt( tripoint_abs_omt( tripoint_min ) );
    if( last_turn == calendar::turn && last_pos == here_omt ) {
        return notes;
    }
    last_turn = calendar::turn;
    last_pos = here_omt;
    notes.clear();
    constexpr auto max_dist = static_cast<int>( 30 );
    constexpr auto max_notes = static_cast<int>( 4 );
    for( const overmapbuffer::t_point_with_note &n : ACTIVE_OVERMAP_BUFFER.get_all_notes(
             here_omt.z() ) ) {
        const auto p = tripoint_abs_omt( n.first, here_omt.z() );
        const auto dist = static_cast<int>( rl_dist( p, here_omt ) );
        if( dist < 1 || dist > max_dist ) {
            continue;
        }
        const auto info = overmap_ui::get_note_display_info( n.second );
        notes.push_back( { .pos = p, .symbol = std::get<0>( info ), .color = tile_color( std::get<1>( info ) ), .dist = dist } );
    }
    std::ranges::sort( notes, []( const near_note & a, const near_note & b ) {
        return a.dist < b.dist;
    } );
    if( notes.size() > max_notes ) {
        notes.resize( max_notes );
    }
    return notes;
}

// Map bearing from the player to the middle of an overmap tile.
auto bearing_to( const tripoint_abs_omt &target ) -> float
{
    const auto from = tripoint_abs_ms( get_avatar().abs_pos() );
    const auto to = tripoint_abs_ms( project_to<coords::ms>( target ) + tripoint_rel_ms( SEEX, SEEY,
                                     0 ) );
    return map_bearing( static_cast<float>( to.x() - from.x() ),
                        static_cast<float>( to.y() - from.y() ) );
}

// A compass tape along the top of the view, centred on where the camera looks: the cardinal
// points, the sun and the moon, where the wind comes from, and places the player chose or was
// given: the mission target, the auto-travel destination and the nearest overmap notes. Threats
// are not on it: the edge arrows point at them.
struct draw_compass_options {
    SDL_Renderer *renderer;
    const hud_frame &frame;
    const atmosphere &air;
    std::vector<hud_text> &texts;
};

auto draw_compass( const draw_compass_options &options ) -> void
{
    auto renderer = options.renderer;
    const auto &frame = options.frame;
    const auto &air = options.air;
    auto &texts = options.texts;

    constexpr auto pi = static_cast<float>( 3.14159265f );
    const auto half_w = static_cast<float>( std::min( frame.w * 0.3f, 240.0f ) );
    const auto cx = static_cast<float>( frame.x + frame.w * 0.5f );
    const auto top = static_cast<float>( frame.y + 4.0f );
    const auto height = static_cast<float>( 18.0f );
    const auto forward = static_cast<float>( camera_forward_bearing( frame.cam.yaw ) );
    const auto x_of = [ cx, forward, half_w ]( float bearing ) {
        return cx + bearing_delta( forward, bearing ) / pi * half_w;
    };
    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 130 );
    const auto band = SDL_FRect{ .x = cx - half_w, .y = top, .w = half_w * 2.0f, .h = height };
    SDL_RenderFillRect( renderer, &band );
    // Ticks every 45 degrees; letters at the cardinal points.
    SDL_SetRenderDrawColor( renderer, 200, 200, 200, 160 );
    static const auto letters = std::array<std::string, 4> {{ "N", "E", "S", "W" }};
    for( auto i = static_cast<int>( 0 ); i < 8; ++i ) {
        const auto b = static_cast<float>( static_cast<float>( i ) * pi / 4.0f );
        const auto x = static_cast<float>( x_of( b ) );
        if( i % 2 == 0 ) {
            texts.push_back( { .x = x, .y = top + 1.0f, .text = letters[static_cast<size_t>( i / 2 )], .color = i == 0 ? 9 : 15, .align = 0, .boxed = false } );
        } else {
            SDL_RenderLine( renderer, x, top + height - 6.0f, x, top + height );
        }
    }
    // Where the camera looks.
    SDL_SetRenderDrawColor( renderer, 255, 255, 255, 220 );
    SDL_RenderLine( renderer, cx, top + height, cx, top + height + 4.0f );

    // The sun and the moon, drawn with their own sprites just under the tape.
    if( frame.sky && frame.sky->loaded() ) {
        for( const sky_body &b : sky_bodies( frame.view_z, air ) ) {
            const auto x = static_cast<float>( x_of( map_bearing( b.dir.x, -b.dir.y ) ) );
            const auto &s = frame.sky->cells[static_cast<size_t>( b.cell )];
            auto tw = static_cast<float>( 0.0f );
            auto th = static_cast<float>( 0.0f );
            SDL_GetTextureSize( frame.sky->sheet.get(), &tw, &th );
            const auto src = SDL_FRect{ .x = s.u0 * tw, .y = s.v0 * th, .w = ( s.u1 - s.u0 ) * tw, .h = ( s.v1 - s.v0 ) * th };
            const auto dst = SDL_FRect{ .x = x - 6.0f, .y = top + height + 1.0f, .w = 12.0f, .h = 12.0f };
            SDL_RenderTexture( renderer, frame.sky->sheet.get(), &src, &dst );
        }
    }
    // The wind: a chevron where it comes from, pointing down the way it blows past.
    const auto &weather = get_weather();
    const auto &you = get_avatar();
    // The wind where the player stands, as the sidebar reports it (cover slows it).
    const auto wind = static_cast<int>( get_local_windpower( weather.windspeed,
                                        ACTIVE_OVERMAP_BUFFER.ter( you.abs_omt_pos() ),
                                        you.abs_pos(), weather.winddirection, g->is_sheltered( you.bub_pos() ) ) );
    if( frame.view_z >= 0 && wind >= 3 ) {
        const auto b = static_cast<float>( static_cast<float>( weather.winddirection ) * pi / 180.0f );
        draw_arrow( { .renderer = renderer, .x = x_of( b ), .y = top + height + 6.0f, .angle = pi * 0.5f, .size = wind >= 25 ? 7.0f : 5.0f, .c = rgba8{ .r = 140, .g = 210, .b = 255, .a = 230 } } );
    }
    // Places: notes first so the mission and the destination are drawn over them.
    const auto here_omt = tripoint_abs_omt( you.abs_omt_pos() );
    for( const near_note &n : nearby_notes( here_omt ) ) {
        texts.push_back( { .x = x_of( bearing_to( n.pos ) ), .y = top + height + 2.0f, .text = std::string( 1, n.symbol ), .color = n.color, .align = 0, .boxed = true } );
    }
    if( !you.omt_path.empty() && you.omt_path.front() != here_omt ) {
        texts.push_back( { .x = x_of( bearing_to( you.omt_path.front() ) ), .y = top + height + 2.0f, .text = "X", .color = 10, .align = 0, .boxed = true } );
    }
    const auto mission = tripoint_abs_omt( you.get_active_mission_target() );
    if( mission != tripoint_abs_omt( tripoint_min ) && mission.xy() != here_omt.xy() ) {
        const auto x = static_cast<float>( x_of( bearing_to( mission ) ) );
        const auto y = static_cast<float>( top + height * 0.5f );
        fill_triangles( renderer, { SDL_FPoint{ .x = x, .y = y - 7.0f }, SDL_FPoint{ .x = x + 6.0f, .y = y }, SDL_FPoint{ .x = x - 6.0f, .y = y },
                                    SDL_FPoint{ .x = x - 6.0f, .y = y }, SDL_FPoint{ .x = x + 6.0f, .y = y }, SDL_FPoint{ .x = x, .y = y + 7.0f }
                                  }, rgba8{ .r = 255, .g = 200, .b = 40, .a = 255 } );
    }
}

// Badges for needs and conditions that are not normal, along the bottom left of the view. Which
// widgets become badges, and their text and colour, come from the "r3d_status_row" widget in
// data/json/ui/sidebar-r3d.json: the thresholds stay in the widgets' own clauses. A widget whose
// text is empty or shown in a "fine" colour (white, green) is normal and has no badge.
auto add_status_badges( const hud_frame &frame, std::vector<hud_text> &texts ) -> void
{
    const auto &you = get_avatar();
    auto x = frame.x + 8.0f;
    const auto y = frame.y + frame.h - frame.cell_h - 8.0f;
    // Use BN's own sidebar descriptions and thresholds.
    const auto needs = std::array{ you.get_hunger_description(), you.get_thirst_description(),
                                   you.get_fatigue_description(), you.get_pain_description() };
    for( const auto &[text, color] : needs ) {
        if( text.empty() || color == c_white || color == c_green || color == c_light_green ) { continue; }
        texts.push_back( { .x = x, .y = y, .text = text, .color = tile_color( color ), .align = -1, .boxed = true } );
        x += float( utf8_width( text ) ) * frame.cell_w + 14.0f;
    }

}

// Listens on the game's event bus for what the overlays react to: the player being hurt and the
// player entering another overmap tile.
class hud_events : public event_subscriber
{
    public:
        auto notify( const cata::event &e ) -> void override {
            if( e.type() == event_type::character_takes_damage ) {
                const auto &you = get_avatar();
                if( e.get<character_id>( "character" ) != you.getID() || e.get<int>( "damage" ) <= 0 ) {
                    return;
                }
                hit_ticks = SDL_GetTicks();
                hit_from = guess_attacker( you );
            } else if( e.type() == event_type::avatar_moves ) {
                const auto name = std::string( display::current_position_text( get_avatar().abs_omt_pos() ) );
                if( name != place ) {
                    place = name;
                    place_ticks = SDL_GetTicks();
                }
            }
        }

        uint64_t hit_ticks = 0;
        std::optional<tripoint_bub_ms> hit_from;
        uint64_t place_ticks = 0;
        std::string place;

    private:
        // The event does not say who hit; take the nearest hostile the player can see, preferring
        // one within reach. Nothing when none is seen: then the hit has no direction.
        static auto guess_attacker( const avatar &you ) -> std::optional<tripoint_bub_ms> {
            auto best = std::optional<tripoint_bub_ms> {};
            auto best_dist = static_cast<int>( 61 );
            for( Creature &critter : g->all_creatures() ) {
                if( critter.is_avatar() || critter.attitude_to( you ) != Attitude::A_HOSTILE ||
                    !you.sees( critter ) ) {
                    continue;
                }
                const auto dist = static_cast<int>( rl_dist( you.bub_pos(), critter.bub_pos() ) );
                if( dist < best_dist ) {
                    best_dist = dist;
                    best = critter.bub_pos();
                }
            }
            return best;
        }
};

auto events() -> hud_events & // *NOPAD*
{
    // Leaked like the renderer's state: it must outlive the game's event bus, which unsubscribes
    // it when it goes away.
    static auto ev = new hud_events();
    static auto bus = static_cast<const event_bus *>( nullptr );
    auto &current = get_event_bus();
    if( bus != &current ) {
        current.subscribe( ev );
        bus = &current;
    }
    return *ev;
}

// A red glow at the edge of the view on the side the hit came from, fading over a second; all
// around the edge when the attacker is not seen.
auto draw_hit_direction( SDL_Renderer *renderer, const hud_frame &frame ) -> void
{
    const auto &ev = events();
    constexpr auto fade_ms = static_cast<uint64_t>( 1000 );
    const auto now = static_cast<uint64_t>( SDL_GetTicks() );
    if( ev.hit_ticks == 0 || now - ev.hit_ticks > fade_ms ) {
        return;
    }
    const auto strength = static_cast<float>( 1.0f - static_cast<float>( now - ev.hit_ticks ) /
                          fade_ms );
    const auto outer = SDL_FColor{ .r = 0.9f, .g = 0.05f, .b = 0.05f, .a = 0.75f * strength };
    const auto inner = SDL_FColor{ .r = 0.9f, .g = 0.05f, .b = 0.05f, .a = 0.0f };
    struct gradient_quad {
        SDL_FPoint a;
        SDL_FPoint b;
        SDL_FPoint c;
        SDL_FPoint d;
    };
    const auto quad = [ &outer, &inner, renderer ]( const gradient_quad & q ) {
        const auto vertices = std::array<SDL_Vertex, 6> {{
                { .position = q.a, .color = outer, .tex_coord = {} },
                { .position = q.b, .color = outer, .tex_coord = {} },
                { .position = q.c, .color = inner, .tex_coord = {} },
                { .position = q.a, .color = outer, .tex_coord = {} },
                { .position = q.c, .color = inner, .tex_coord = {} },
                { .position = q.d, .color = inner, .tex_coord = {} }
            }
        };
        SDL_RenderGeometry( renderer, nullptr, vertices.data(), vertices.size(), nullptr, 0 );
    };
    const auto x0 = static_cast<float>( frame.x );
    const auto y0 = static_cast<float>( frame.y );
    const auto x1 = static_cast<float>( frame.x + frame.w );
    const auto y1 = static_cast<float>( frame.y + frame.h );
    const auto &you = get_avatar();
    auto from_screen = std::optional<vec3> {};
    auto me = std::optional<vec3>( frame.screen( you.bub_pos(), 0.5f ) );
    if( ev.hit_from ) {
        from_screen = frame.screen( *ev.hit_from, 0.5f );
    }
    if( !from_screen || !me || ( std::fabs( from_screen->x - me->x ) < 1.0f &&
                                 std::fabs( from_screen->y - me->y ) < 1.0f ) ) {
        const auto d = static_cast<float>( 24.0f );
        quad( { .a = { x0, y0 }, .b = { x1, y0 }, .c = { x1 - d, y0 + d }, .d = { x0 + d, y0 + d } } );
        quad( { .a = { x1, y0 }, .b = { x1, y1 }, .c = { x1 - d, y1 - d }, .d = { x1 - d, y0 + d } } );
        quad( { .a = { x1, y1 }, .b = { x0, y1 }, .c = { x0 + d, y1 - d }, .d = { x1 - d, y1 - d } } );
        quad( { .a = { x0, y1 }, .b = { x0, y0 }, .c = { x0 + d, y0 + d }, .d = { x0 + d, y1 - d } } );
        return;
    }
    // Where the line from the middle of the view along the attacker's direction leaves it.
    const auto dx = static_cast<float>( from_screen->x - me->x );
    const auto dy = static_cast<float>( from_screen->y - me->y );
    const auto m = edge_mark( place_edge_mark( { .clip_x = dx, .clip_y = -dy, .clip_w = 1e-3f, .margin_x = 0.0f, .margin_y = 0.0f } ) );
    const auto ex = static_cast<float>( frame.x + ( m.x * 0.5f + 0.5f ) * frame.w );
    const auto ey = static_cast<float>( frame.y + ( 0.5f - m.y * 0.5f ) * frame.h );
    const auto len = static_cast<float>( std::sqrt( dx * dx + dy * dy ) );
    const auto ux = static_cast<float>( dx / len );
    const auto uy = static_cast<float>( dy / len );
    // Across the direction, and inwards.
    const auto half = static_cast<float>( std::min( frame.w, frame.h ) * 0.22f );
    const auto depth = static_cast<float>( 40.0f );
    const auto a = SDL_FPoint{ .x = ex - uy * half, .y = ey + ux * half };
    const auto b = SDL_FPoint{ .x = ex + uy * half, .y = ey - ux * half };
    quad( { .a = a, .b = b, .c = { b.x - ux * depth, b.y - uy * depth }, .d = { a.x - ux * depth, a.y - uy * depth } } );
}

// "Entering: <place>" under the compass for a few seconds when the place the sidebar names
// changes (not on every overmap tile of a forest).
auto add_place_notice( const hud_frame &frame, std::vector<hud_text> &texts ) -> void
{
    const auto &ev = events();
    constexpr auto show_ms = static_cast<uint64_t>( 3500 );
    if( ev.place_ticks == 0 || SDL_GetTicks() - ev.place_ticks > show_ms || ev.place.empty() ) {
        return;
    }
    texts.push_back( { .x = frame.x + frame.w * 0.5f, .y = frame.y + 44.0f, .text = string_format( _( "Entering: %s" ), ev.place ), .color = 15, .align = 0, .boxed = true } );
}

} // namespace

auto hud_animation_pending() -> bool
{
    if( !get_option<bool>( "R3D_HUD" ) ) { return false; }
    const auto &ev = events();
    const auto now = SDL_GetTicks();
    return ( ev.hit_ticks != 0 && now - ev.hit_ticks <= 1000 ) ||
           ( ev.place_ticks != 0 && now - ev.place_ticks <= 3500 );
}

auto draw_hud( const draw_hud_options &options ) -> void
{
    auto renderer = options.renderer;
    const auto &frame = options.frame;
    const auto &air = options.air;
    auto &texts = options.texts;

    const auto restore_renderer = renderer_state_guard( renderer, {
        .x = static_cast<int>( frame.x ), .y = static_cast<int>( frame.y ),
        .w = static_cast<int>( frame.w ), .h = static_cast<int>( frame.h )
    } );

    if( get_option<bool>( "R3D_HUD" ) ) {
        add_combat_text( frame, texts );
        if( !frame.cam.first_person() ) {
            draw_edge_arrows( renderer, frame );
            draw_compass( { .renderer = renderer, .frame = frame, .air = air, .texts = texts } );
        }
        add_status_badges( frame, texts );
        draw_hit_direction( renderer, frame );
        add_place_notice( frame, texts );
    } else {
        // Keep listening so a notice is not shown late when the option comes back on.
        events();
    }
}

} // namespace r3d

#endif // defined(TILES)
