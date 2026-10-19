#pragma once
#include <string>
#include "avatar.h"
#include "calendar.h"
#include "game.h"
#include "overmap/overmapbuffer.h"
#include "overmap/omdata.h"
#include "translations.h"
#include "weather/weather.h"
#include "weather/weather_type.h"

namespace display
{
inline auto current_position_text( const tripoint_abs_omt &pos ) -> std::string
{ return ACTIVE_OVERMAP_BUFFER.ter( pos )->get_name(); }
inline auto date_string() -> std::string
{ return string_format( _( "%s, day %d" ), calendar::name_season( season_of_year( calendar::turn ) ), day_of_season<int>( calendar::turn ) + 1 ); }
inline auto time_string( const avatar &you ) -> std::string
{ return you.has_watch() ? to_string_time_of_day( calendar::turn ) : std::string( _( "Exact time unknown" ) ); }
inline auto get_temp( const avatar &you ) -> std::string
{
    if( !you.has_item_with_flag( flag_id( "THERMOMETER" ) ) &&
        !you.has_enchantment_flag( enchantment_flag_id( "THERMOMETER" ) ) ) { return "-"; }
    return print_temperature( get_weather().get_temperature( you.abs_pos() ) );
}
}
