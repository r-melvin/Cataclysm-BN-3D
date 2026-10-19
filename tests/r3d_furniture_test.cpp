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

TEST_CASE( "r3d_chair_is_meshed_as_a_solid_prop", "[r3d][tiles]" )
{
    const auto contexts = tile_context_fixture( true );
    REQUIRE( contexts.valid() );
    const auto pos = tripoint_abs_sm( get_map().get_abs_sub(), 0 );
    auto sm = submap( pos, {} );
    sm.set_all_ter( t_floor );
    sm.set_furn( point_sm_ms( 5, 5 ), furn_str_id( "f_chair" ).id() );
    const auto lookup = [&sm, &pos]( const tripoint_abs_sm &requested ) -> const submap * {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto mesh = r3d::build_chunk_mesh(
                          { .pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext } );
    CHECK( mesh.uprights.empty() );
    CHECK( std::ranges::any_of( mesh.vertices, []( const auto &vertex ) {
        return vertex.x >= 5.0f && vertex.x <= 6.0f && vertex.y <= -5.0f && vertex.y >= -6.0f &&
               vertex.z > r3d::floor_height;
    } ) );
}

#endif
