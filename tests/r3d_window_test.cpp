#include "catch/catch.hpp"

#if defined(TILES)

#    include "map/map.h"
#    include "map/mapdata.h"
#    include "map/submap.h"
#    include "r3d_mesh.h"
#    include "sdltiles.h"
#    include "tile_helpers.h"

#    include <algorithm>
#    include <cmath>
#    include <limits>
#    include <ranges>

TEST_CASE( "r3d_window_recess_has_local_contact_shading", "[r3d][tiles]" )
{
    const auto contexts = tile_context_fixture( true );
    REQUIRE( contexts.valid() );
    const auto pos = tripoint_abs_sm( get_map().get_abs_sub(), 0 );
    auto sm = submap( pos, {} );
    sm.set_all_ter( t_floor );
    sm.set_ter( point_sm_ms( 5, 5 ), ter_str_id( "t_window" ).id() );
    sm.set_ter( point_sm_ms( 5, 4 ), t_wall );
    sm.set_ter( point_sm_ms( 5, 6 ), t_wall );
    const auto lookup = [&sm, &pos]( const tripoint_abs_sm &requested ) -> const submap * {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto mesh = r3d::build_chunk_mesh(
                          { .pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext } );
    CHECK( std::ranges::any_of( mesh.light_refs, []( const auto &ref ) { return ref.shade == 178; } ) );
    CHECK( std::ranges::any_of( mesh.light_refs, []( const auto &ref ) { return ref.shade == 230; } ) );
}

#endif
