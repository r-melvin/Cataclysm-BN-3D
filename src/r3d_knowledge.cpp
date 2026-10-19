#include "r3d_knowledge.h"

#include "avatar.h"
#include "game.h"
#include "map/map.h"
#include "map/mapdata.h"
#include "map_memory.h"
#include "trap.h"
#include "vehicle/veh_type.h"

namespace r3d
{
namespace
{
auto memory_description( const memorized_terrain_tile &memory ) -> std::string
{
    if( memory.tile.empty() ) { return {}; }
    const auto furniture = furn_str_id( memory.tile );
    if( furniture.is_valid() && furniture != furn_str_id::NULL_ID() ) { return furniture->name(); }
    const auto terrain = ter_str_id( memory.tile );
    if( terrain.is_valid() && terrain != ter_str_id::NULL_ID() ) { return terrain->name(); }
    const auto remembered_trap = trap_str_id( memory.tile );
    if( remembered_trap.is_valid() && remembered_trap != trap_str_id::NULL_ID() ) {
        return remembered_trap->name();
    }
    // BN's native tiles view records vehicle overlays with this exact prefix.
    if( memory.tile.starts_with( "vp_" ) ) {
        const auto part = vpart_id( memory.tile.substr( 3 ) );
        if( part.is_valid() ) { return part->name(); }
    }
    return {};
}
} // namespace

auto crosshair_description( avatar &you, map &here, const tripoint_bub_ms &p ) -> std::string
{
    if( !here.inbounds( p ) ) { return {}; }
    if( here.pl_sees( p, -1 ) ) {
        const auto *critter = g->critter_at( p, true );
        if( critter && you.sees( *critter ) ) { return critter->disp_name(); }
        return here.furn( p ) != furn_str_id::NULL_ID() ? here.furnname( p ) : here.tername( p );
    }
    if( !you.should_show_map_memory() ) { return {}; }
    const auto absolute = bub_to_abs( p );
    const auto overlay = memory_description( you.get_memorized_tile( absolute ) );
    return overlay.empty() ? memory_description( you.get_terrain_tile( absolute ) ) : overlay;
}
} // namespace r3d
