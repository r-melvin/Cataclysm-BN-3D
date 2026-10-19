#if defined(TILES)
#include "cata_tiles.h"

#include <algorithm>
#include <array>
#include <ranges>

#include "avatar.h"
#include "character.h"
#include "game.h"
#include "itype.h"
#include "mtype.h"
#include "catacharset.h"
#include "cursesport.h"
#include "map/map.h"
#include "map/mapdata.h"
#include "map/field_type.h"
#include "r3d_scene_rules.h"
#include "sdltiles.h"

namespace
{
const auto multitile_keys = std::array<std::string, 8> { "center", "corner", "edge", "t_connection", "end_piece", "unconnected", "open", "broken" };
/// Read only the sampled sprite; never take ownership of a dynamic atlas surface.
auto scan_sprite( texture_draw_data &data ) -> void
{
    auto renderer = get_sdl_renderer().get();
    auto old_r = Uint8{};
    auto old_g = Uint8{};
    auto old_b = Uint8{};
    auto old_a = Uint8{};
    SDL_GetRenderDrawColor( renderer, &old_r, &old_g, &old_b, &old_a );
    auto previous = SDL_GetRenderTarget( renderer );
    auto target = SDL_Texture_Ptr( SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_TARGET, data.dimensions.w, data.dimensions.h ) );
    if( !target || !SDL_SetRenderTarget( renderer, target.get() ) ) { return; }
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 0 );
    SDL_RenderClear( renderer );
    const auto source = SDL_FRect{ .x = float( data.dimensions.x ), .y = float( data.dimensions.y ),
                                   .w = float( data.dimensions.w ), .h = float( data.dimensions.h ) };
    SDL_RenderTexture( renderer, data.texture, &source, nullptr );
    auto surface = SDL_Surface_Ptr( SDL_RenderReadPixels( renderer, nullptr ) );
    SDL_SetRenderTarget( renderer, previous );
    SDL_SetRenderDrawColor( renderer, old_r, old_g, old_b, old_a );
    if( !surface ) { return; }
    auto min_x = surface->w;
    auto min_y = surface->h;
    auto max_x = -1;
    auto max_y = -1;
    auto alpha_mask = std::make_shared<std::vector<uint8_t>>(
                          static_cast<size_t>( surface->w * surface->h ), uint8_t{} );
    for( auto y = 0; y < surface->h; ++y ) {
        for( auto x = 0; x < surface->w; ++x ) {
            auto r = Uint8{};
            auto g = Uint8{};
            auto b = Uint8{};
            auto a = Uint8{};
            if( SDL_ReadSurfacePixel( surface.get(), x, y, &r, &g, &b, &a ) && a > 32 ) {
                ( *alpha_mask )[static_cast<size_t>( y * surface->w + x )] = 1;
                min_x = std::min( min_x, x );
                min_y = std::min( min_y, y );
                max_x = std::max( max_x, x );
                max_y = std::max( max_y, y );
            }
        }
    }
    if( max_x < min_x ) { return; }
    data.alpha_mask = std::move( alpha_mask );
    data.opaque = { .x = min_x, .y = min_y, .w = max_x - min_x + 1, .h = max_y - min_y + 1 };
    auto colors = std::array<std::array<uint64_t, 4>, 2> {};
    for( auto y = min_y; y <= max_y; ++y ) {
        for( auto x = min_x; x <= max_x; ++x ) {
            auto r = Uint8{};
            auto g = Uint8{};
            auto b = Uint8{};
            auto a = Uint8{};
            if( SDL_ReadSurfacePixel( surface.get(), x, y, &r, &g, &b, &a ) && a > 32 ) {
                auto &sum = colors[y < min_y + data.opaque.h / 3 ? 0 : 1];
                sum[0] += r;
                sum[1] += g;
                sum[2] += b;
                ++sum[3];
            }
        }
    }
    const auto pack = []( const auto & sum ) -> uint32_t {
        return sum[3] == 0 ? 0 : 0xff000000u | uint32_t( sum[0] / sum[3] ) << 16 |
                uint32_t( sum[1] / sum[3] ) << 8 | uint32_t( sum[2] / sum[3] );
    };
    data.top_color = pack( colors[0] );
    data.body_color = pack( colors[1] );
    // Sample a fully opaque body texel for closed solids, never a baked ground shadow.
    auto best = 3 * 256 * 256;
    for( auto y = min_y; y <= max_y; ++y ) {
        for( auto x = min_x; x <= max_x; ++x ) {
            auto r = Uint8{};
            auto g = Uint8{};
            auto b = Uint8{};
            auto a = Uint8{};
            if( !SDL_ReadSurfacePixel( surface.get(), x, y, &r, &g, &b, &a ) || a < 250 ) { continue; }
            const auto dr = int( r ) - int( ( data.body_color >> 16 ) & 255 );
            const auto dg = int( g ) - int( ( data.body_color >> 8 ) & 255 );
            const auto db = int( b ) - int( data.body_color & 255 );
            const auto distance = dr * dr + dg * dg + db * db;
            if( distance < best ) {
                best = distance;
                data.solid_uv = std::array<float, 2> {
                    data.uv0.first + ( x + 0.5f ) / surface->w *( data.uv1.first - data.uv0.first ),
                        data.uv0.second + ( y + 0.5f ) / surface->h *( data.uv1.second - data.uv0.second )
                };
            }
        }
    }
}
}

auto cata_tiles::get_sprite_draw_data( const sprite_request &request ) ->
std::optional<texture_draw_data>
{
    auto lookup = find_tile_looks_like( request.id, request.category );
    const auto placeholder = !lookup || lookup->id().starts_with( "unknown" );
    if( !lookup ) {
        auto symbol = uint32_t{};
        auto color = c_white;
        if( request.category == C_TERRAIN && ter_str_id( request.id ).is_valid() ) {
            symbol = ter_str_id( request.id )->symbol();
            color = ter_str_id( request.id )->color();
        } else if( request.category == C_FURNITURE && furn_str_id( request.id ).is_valid() ) {
            symbol = furn_str_id( request.id )->symbol();
            color = furn_str_id( request.id )->color();
        } else if( request.category == C_MONSTER && mtype_id( request.id ).is_valid() ) {
            symbol = UTF8_getch( mtype_id( request.id )->sym );
            color = mtype_id( request.id )->color;
        } else if( request.category == C_ITEM && itype_id( request.id ).is_valid() ) {
            symbol = UTF8_getch( itype_id( request.id )->sym );
            color = itype_id( request.id )->color;
        } else if( request.category == C_FIELD && field_type_str_id( request.id ).is_valid() ) {
            symbol = field_type_str_id( request.id )->get_codepoint();
        }
        if( symbol > 0 && symbol < 256 ) {
            const auto foreground = cata_cursesport::colorpairs[color.to_color_pair_index()].FG +
                                    ( color.is_bold() ? 8 : 0 );
            for( const auto fg : { foreground, -1 } ) {
                const auto ascii = std::string{ 'A', 'S', 'C', 'I', 'I', '_', char( symbol ), char( fg ), char( -1 ) };
                lookup = find_tile_looks_like( ascii, C_NONE );
                if( lookup ) { break; }
            }
        }
        if( !lookup ) { return std::nullopt; }
    }
    const auto *picked = &lookup->tile();
    if( request.subtile >= 0 && request.subtile < int( multitile_keys.size() ) && picked->multitile ) {
        if( auto sub = tileset_ptr->find_tile_type( lookup->id() + "_" +
                       multitile_keys[request.subtile] ) ) {
            picked = sub;
        }
    }
    auto rotation = request.rotation;
    if( !picked->rotates && request.category != C_NONE && request.category != C_MONSTER ) { rotation = 0; }
    // Stable per-map-position variants, without advancing gameplay RNG.
    const auto seed = uint32_t( request.pos.x() * 73856093u ^ request.pos.y() * 19349663u );
    const auto *indices = ( request.background ? picked->sprite.bg : picked->sprite.fg ).pick( seed );
    if( !indices && !request.background && picked->multitile ) {
        for( const auto &key : multitile_keys ) {
            if( auto sub = tileset_ptr->find_tile_type( lookup->id() + "_" + key ) ) {
                indices = sub->sprite.fg.pick( seed );
                if( indices ) { break; }
            }
        }
    }
    if( !indices || indices->empty() ) { return std::nullopt; }
    const auto sprite_num = indices->size() > 1 ? size_t( std::max( rotation,
                            0 ) ) % indices->size() : 0;
    const auto result = tileset_ptr->get_or_default( ( *indices )[sprite_num], -1,
                        tileset_fx_type::none );
    if( !result.tex ) { return std::nullopt; }
    auto owner = result.tex->r3d_texture();
    const auto rect = result.tex->r3d_rect();
    auto width = 0.0f;
    auto height = 0.0f;
    if( !owner || !SDL_GetTextureSize( owner.get(), &width, &height ) || width <= 0 || height <= 0 ) {
        return std::nullopt;
    }
    auto data = texture_draw_data{
        .texture = owner.get(),
        .dimensions = { .x = int( rect.x ), .y = int( rect.y ), .w = int( rect.w ), .h = int( rect.h ) },
        .uv0 = { rect.x / width, rect.y / height },
        .uv1 = { ( rect.x + rect.w ) / width, ( rect.y + rect.h ) / height },
        .owner = owner,
        .size_x = rect.w / std::max( 1, tileset_ptr->get_tile_width() ),
                      .size_y = rect.h / std::max( 1, tileset_ptr->get_tile_height() ),
                      .quarter_turns = indices->size() == 1 && !request.background ? std::max( rotation, 0 ) % 4 : 0,
                      .flip = !request.background && rotation == -1,
        .alpha_mask = {},
        .facing = std::max( rotation, 0 ) % 4,
        .offset_x = picked->offset.x,
        .offset_y = picked->offset.y
    };
    data.placeholder = placeholder;
    scan_sprite( data );
    return data;
}

auto cata_tiles::get_orientation( const orientation_request &request ) -> void
{
    auto &here = get_map();
    const auto &p = request.pos;
    auto &subtile = request.subtile;
    auto &rotation = request.rotation;
    auto group = 0;
    if( request.field ) {
        const auto field = r3d::visible_field_at( here, get_avatar(), p );
        get_rotation_and_subtile( field ? field->connections : 0, rotation, subtile );
    } else if( request.furniture ) {
        const auto f = here.furn( p );
        if( f->connects( group ) ) { get_furn_connect_values( p, subtile, rotation, group, {} ); }
        else {
            const auto neighbors = std::array<int, 4> { here.furn( p + point_rel_ms::south() ).to_i(),
                    here.furn( p + point_rel_ms::east() ).to_i(), here.furn( p + point_rel_ms::west() ).to_i(),
                    here.furn( p + point_rel_ms::north() ).to_i()
                                                      };
            get_tile_values_with_ter( p, f.to_i(), neighbors.data(), subtile, rotation );
        }
    } else if( here.ter( p )->connects( group ) ) {
        get_connect_values( p, subtile, rotation, group, {} );
    } else {
        const auto invisible = std::array<bool, 5> {};
        const bool known[5] = { invisible[0], invisible[1], invisible[2], invisible[3], invisible[4] };
        get_terrain_orientation( p, rotation, subtile, {}, known );
    }
}

auto cata_tiles::get_character_layers( const Character &ch ) -> character_layers
{
    auto out = character_layers{ .ids = {}, .mirrored = ch.facing == FD_LEFT };
    out.ids.emplace_back( ch.is_npc() ? ( ch.male ? "npc_male" : "npc_female" ) :
                          ( ch.male ? "player_male" : "player_female" ), C_NONE );
    for( const auto &overlay : ch.get_overlay_ids() ) {
        auto id = std::string{};
        if( find_overlay_looks_like( ch.male, overlay.id, id ) ) { out.ids.emplace_back( id, C_NONE ); }
    }
    return out;
}

auto cata_tiles::get_frame_markers() const -> frame_markers
{
    auto out = frame_markers{};
    out.sct = do_draw_sct;
    out.zones = do_draw_zones;
    out.zone_start = zone_start;
    out.zone_end = zone_end;
    out.zone_offset = zone_offset;
    out.zone_points = zone_points;
    if( do_draw_cursor ) { out.cursors = cursors; }
    if( do_draw_highlight ) { out.highlights = highlights; }
    if( do_draw_line && !line_trajectory.empty() ) {
        if( !is_target_line || get_avatar().sees( line_pos ) ) {
            out.line.assign( line_trajectory.begin(), line_trajectory.end() - 1 );
        }
        out.line_end = line_trajectory.back();
        out.line_end_id = line_endpoint_id;
    }
    return out;
}

auto cata_tiles::restore_frame_markers( const frame_markers &markers ) -> void
{
    do_draw_sct = markers.sct;
    do_draw_zones = markers.zones;
    zone_start = markers.zone_start;
    zone_end = markers.zone_end;
    zone_offset = markers.zone_offset;
    zone_points = markers.zone_points;
    cursors = markers.cursors;
    highlights = markers.highlights;
    do_draw_cursor = !cursors.empty();
    do_draw_highlight = !highlights.empty();
    line_trajectory = markers.line;
    if( markers.line_end ) { line_trajectory.push_back( *markers.line_end ); }
    line_endpoint_id = markers.line_end_id;
    do_draw_line = !line_trajectory.empty();
    is_target_line = false; // get_frame_markers already applied target visibility.
}

auto cata_tiles::drop_frame_layers() -> void
{
    void_item_override();
    void_vpart_override();
    void_monster_override();
    void_draw_below_override();
    void_hit();
    void_line();
    void_cursor();
    void_highlight();
    void_weather();
    void_sct();
    void_zones();
    void_radiation_override();
    void_terrain_override();
    void_furniture_override();
    void_graffiti_override();
    void_trap_override();
    void_field_override();
}
#endif
