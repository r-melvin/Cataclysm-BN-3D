#pragma once
#ifndef CATA_SRC_R3D_PROFILE_MATERIALS_H
#define CATA_SRC_R3D_PROFILE_MATERIALS_H

#include <algorithm>
#include <optional>
#include <string_view>

#include "r3d_mesh.h"
#include "r3d_presentation_profiles.h"

namespace r3d
{

struct profile_crop_options {
    const sprite_ref &sprite;
    const profile_face_source &source;
    std::string_view tileset;
};

/// Preserve the donor sheet/ownership; never reinterpret crops on another tileset.
inline auto crop_profile_sprite( const profile_crop_options &options ) -> std::optional<sprite_ref>
{
    const auto &crop = options.source.crop;
    const auto valid_coordinates = std::ranges::all_of( crop, []( const auto value ) { return value >= 0.0f && value <= 1.0f; } );
    if( !options.sprite.valid || options.source.tileset != options.tileset || !valid_coordinates ||
        crop[0] >= crop[2] || crop[1] >= crop[3] ) { return std::nullopt; }
    auto result = options.sprite;
    const auto width = result.u1 - result.u0;
    const auto height = result.v1 - result.v0;
    result.u1 = result.u0 + width * crop[2];
    result.v1 = result.v0 + height * crop[3];
    result.u0 += width * crop[0];
    result.v0 += height * crop[1];
    return result;
}

/// Orient a cropped face, including partial sill/lintel sections, within its own sheet region.
inline auto orient_material_uvs( const sprite_ref &sprite, const uvs_t &source ) -> uvs_t
{
    const auto turns = sprite.quarter_turns & 3;
    if( turns == 0 && !sprite.flip ) { return source; }
    const auto width = sprite.u1 - sprite.u0;
    const auto height = sprite.v1 - sprite.v0;
    if( width <= 0.0f || height <= 0.0f ) { return source; }
    auto result = source;
    for( auto &uv : result ) {
        const auto x = ( uv[0] - sprite.u0 ) / width;
        const auto y = ( uv[1] - sprite.v0 ) / height;
        auto sx = x;
        auto sy = y;
        switch( turns ) {
            case 1:
                sx = y;
                sy = 1.0f - x;
                break;
            case 2:
                sx = 1.0f - x;
                sy = 1.0f - y;
                break;
            case 3:
                sx = 1.0f - y;
                sy = x;
                break;
            default:
                break;
        }
        if( sprite.flip ) { sx = 1.0f - sx; }
        uv = { sprite.u0 + width * sx, sprite.v0 + height * sy };
    }
    return result;
}

} // namespace r3d
#endif // CATA_SRC_R3D_PROFILE_MATERIALS_H
