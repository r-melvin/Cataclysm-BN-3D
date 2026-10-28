#pragma once
#ifndef CATA_SRC_R3D_VEHICLE_LIGHTING_H
#define CATA_SRC_R3D_VEHICLE_LIGHTING_H

#if defined(TILES)
#include <functional>

#include "map/map.h"
#include "r3d_mesh.h"

namespace r3d
{

/// A live vehicle surface uses local illumination, independently of the remembered
/// terrain beneath it. This callback never decides whether a vehicle is visible.
inline auto vehicle_surface_lighting( const map &here, const bool fullbright = false ) ->
std::function < auto( const tripoint_bub_ms & ) -> uint32_t >
{
    return [&here, fullbright]( const tripoint_bub_ms & p ) -> uint32_t {
        if( !here.inbounds( p ) ) { return pack_light( 0, 0, 0 ); }
        if( fullbright ) { return pack_light( 255, 255, 255 ); }
        const auto level = here.light_at( p );
        const auto brightness = level == lit_level::BRIGHT || level == lit_level::LIT ? 255 :
        level == lit_level::LOW ? 224 : 204;
        return pack_light( brightness, brightness, brightness );
    };
}

} // namespace r3d
#endif
#endif
