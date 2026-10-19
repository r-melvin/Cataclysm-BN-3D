#include "status_report.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "creature.h"
#include "r3d_display.h"
#include "game.h"
#include "line.h"
#include "map/map.h"
#include "overmap/overmapbuffer.h"
#include "output.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "weather/weather.h"

namespace
{

auto moon_phase_name( moon_phase phase ) -> std::string
{
    switch( phase ) {
        case MOON_NEW:
            return _( "new moon" );
        case MOON_WAXING_CRESCENT:
            return _( "waxing crescent moon" );
        case MOON_HALF_MOON_WAXING:
            return _( "waxing half moon" );
        case MOON_WAXING_GIBBOUS:
            return _( "waxing gibbous moon" );
        case MOON_FULL:
            return _( "full moon" );
        case MOON_WANING_GIBBOUS:
            return _( "waning gibbous moon" );
        case MOON_HALF_MOON_WANING:
            return _( "waning half moon" );
        case MOON_WANING_CRESCENT:
        default:
            return _( "waning crescent moon" );
    }
}

// "Hungry" etc.: the sidebar's own text for a need, left out when it is normal (empty or a
// colour the sidebar uses for "fine").
auto add_need( std::vector<std::string> &out, const std::pair<std::string, nc_color> &need ) -> void
{
    const auto text = std::string( remove_color_tags( need.first ) );
    if( text.empty() || need.second == c_white || need.second == c_green ||
        need.second == c_light_green ) {
        return;
    }
    out.push_back( text );
}

auto join( const std::vector<std::string> &parts ) -> std::string
{
    return enumerate_as_string( parts.begin(), parts.end(), []( const std::string & s ) {
        return s;
    } );
}

} // namespace

auto status_report( const avatar &you ) -> std::string
{
    auto out = std::string{};
    const auto sky = bool( you.bub_pos().z() >= 0 && !g->is_sheltered( you.bub_pos() ) );
    // Time and date.
    out += string_format( _( "%s, %s." ), display::date_string(), display::time_string( you ) );
    // Weather overhead, wind, temperature.
    if( you.bub_pos().z() < 0 ) {
        out += " " + std::string( _( "You are underground." ) );
    } else {
        out += " " + string_format( _( "Weather: %s." ), get_weather().weather_id->name.translated() );
        const auto &weather = get_weather();
        // The wind where the player stands, as the sidebar reports it (cover slows it).
        const auto wind = int( get_local_windpower( weather.windspeed,
                               ACTIVE_OVERMAP_BUFFER.ter( you.abs_omt_pos() ),
                               you.abs_pos(), weather.winddirection, g->is_sheltered( you.bub_pos() ) ) );
        if( wind < 3 ) {
            out += " " + std::string( _( "The air is calm." ) );
        } else {
            out += " " + string_format( _( "Wind %d mph from the %s." ), wind,
                                        get_dirstring( weather.winddirection ) );
        }
        if( sky && is_night( calendar::turn ) ) {
            out += " " + string_format( _( "The sky shows a %s." ),
                                        moon_phase_name( get_moon_phase( calendar::turn ) ) );
        }
    }
    const auto temp = std::string( display::get_temp( you ) );
    if( temp != "-" ) {
        out += " " + string_format( _( "Temperature %s." ), temp );
    }
    // Where.
    out += " " + string_format( _( "Place: %s." ),
                                display::current_position_text( you.abs_omt_pos() ) );
    // Needs and conditions that are not normal.
    auto needs = std::vector<std::string> {};
    add_need( needs, you.get_hunger_description() );
    add_need( needs, you.get_thirst_description() );
    add_need( needs, you.get_fatigue_description() );
    add_need( needs, you.get_pain_description() );

    out += " " + ( needs.empty() ? std::string( _( "No pressing needs." ) ) :
                   string_format( _( "You are %s." ), join( needs ) ) );
    // Hostiles in sight, nearest first.
    struct hostile_report {
        int distance;
        std::string description;
    };
    auto hostiles = std::vector<hostile_report> {};
    for( Creature &critter : g->all_creatures() ) {
        if( critter.is_avatar() || critter.attitude_to( you ) != Attitude::A_HOSTILE ||
            !you.sees( critter ) ) {
            continue;
        }
        const auto dist = int( rl_dist( you.bub_pos(), critter.bub_pos() ) );
        auto where = std::string( direction_name( direction_from( you.bub_pos().raw(),
                                  critter.bub_pos().raw() ) ) );
        hostiles.push_back( { .distance = dist,
                              .description = string_format( _( "%1$s %2$d tiles %3$s" ), critter.get_name(), dist, where ) } );
    }
    std::ranges::sort( hostiles, {}, &hostile_report::distance );
    if( hostiles.empty() ) {
        out += " " + std::string( _( "No hostiles in sight." ) );
    } else {
        auto names = std::vector<std::string> {};
        for( auto i = size_t( 0 ); i < hostiles.size() && i < 6; ++i ) {
            names.push_back( hostiles[i].description );
        }
        if( hostiles.size() > 6 ) {
            names.push_back( string_format( _( "%d more" ), static_cast<int>( hostiles.size() - 6 ) ) );
        }
        out += " " + string_format( _( "Hostiles in sight: %s." ), join( names ) );
    }
    // Sounds heard this turn.
    auto heard = std::vector<std::string> {};
    for( const tripoint_bub_ms &p : sounds::get_footstep_markers() ) {
        const auto what = std::string( sounds::sound_at( p ) );
        const auto where = std::string( direction_name( direction_from( you.bub_pos().raw(), p.raw() ) ) );
        heard.push_back( what.empty() ? string_format( _( "something %s" ), where ) :
                         string_format( _( "%1$s %2$s" ), what, where ) );
        if( heard.size() >= 4 ) {
            break;
        }
    }
    if( !heard.empty() ) {
        out += " " + string_format( _( "You hear %s." ), join( heard ) );
    }
    return out;
}
