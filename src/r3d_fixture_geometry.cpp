#include "r3d_fixture_geometry.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <numbers>

#if defined(TILES)
#include "r3d_mesh.h"
#include "sdl_wrappers.h"
#endif

namespace r3d
{
auto radio_tower_pieces( const float height ) -> std::vector<fixture_piece>
{
    auto pieces = std::vector<fixture_piece> {};
    for( const auto x : { 0.12f, 0.88f } ) {
        for( const auto y : { 0.12f, 0.88f } ) {
            pieces.push_back( { .box = { .center = { x, y, height * 0.5f }, .size = { 0.055f, 0.055f, height } }, .detail = fixture_detail::leg } );
        }
    }
    const auto sections = std::max( 1, static_cast<int>( std::ceil( height * 2.0f ) ) );
    const auto rise = height / sections;
    for( auto section = 0; section < sections; ++section ) {
        const auto z = rise * ( section + 0.5f );
        const auto diagonal = std::hypot( 0.76f, rise - 0.04f );
        for( const auto y : { 0.12f, 0.88f } ) {
            pieces.push_back( { .box = { .center = { 0.5f, y, z }, .size = { diagonal, 0.025f, 0.025f }, .pitch = std::atan2( rise - 0.04f, 0.76f ) * ( section % 2 ? -1.0f : 1.0f ) }, .finish = section % 2 ? fixture_finish::steel : fixture_finish::red, .detail = fixture_detail::brace } );
        }
        for( const auto x : { 0.12f, 0.88f } ) {
            pieces.push_back( { .box = { .center = { x, 0.5f, z }, .size = { diagonal, 0.025f, 0.025f }, .yaw = std::numbers::pi_v<float> * 0.5f, .pitch = std::atan2( rise - 0.04f, 0.76f ) * ( section % 2 ? -1.0f : 1.0f ) }, .finish = section % 2 ? fixture_finish::steel : fixture_finish::red, .detail = fixture_detail::brace } );
        }
    }
    // Panel aerials sit near the crown, leaving the support lattice open below.
    for( const auto x : { 0.10f, 0.90f } ) {
        pieces.push_back( { .box = { .center = { x, 0.5f, height - 0.26f },
                                     .size = { 0.10f, 0.20f, 0.46f }
                                   }, .finish = fixture_finish::enamel } );
    }
    for( const auto y : { 0.10f, 0.90f } ) {
        pieces.push_back( { .box = { .center = { 0.5f, y, height - 0.26f },
                                     .size = { 0.20f, 0.10f, 0.46f }
                                   }, .finish = fixture_finish::enamel } );
    }
    return pieces;
}

#if defined(TILES)
auto make_fixture_palette( SDL_Renderer *renderer, gpu *device ) -> sprite_ref
{
    constexpr auto colors = std::array<uint32_t, 7> { 0xdadbd6, 0xa3a9ac, 0x25292b, 0x263b45, 0xc3b69a, 0xa84939, 0xffffff };
    const auto surface = SDL_Surface_Ptr( SDL_CreateSurface( colors.size(), 1,
                                          SDL_PIXELFORMAT_RGBA32 ) );
    if( !surface ) { return {}; }
    for( auto i = size_t{ 0 }; i < colors.size(); ++i ) {
        const auto color = colors[i];
        const auto rect = SDL_Rect{ .x = static_cast<int>( i ), .y = 0, .w = 1, .h = 1 };
        const auto pixel = SDL_MapRGBA( SDL_GetPixelFormatDetails( surface->format ), nullptr, color >> 16,
                                        color >> 8, color, 255 );
        if( !SDL_FillSurfaceRect( surface.get(), &rect, pixel ) ) { return {}; }
    }
    // Upload before the first 3D draw; SDL_Renderer textures defer pixels until present.
    auto texture = device ? device->create_sheet( surface.get() ) :
                   std::shared_ptr<SDL_Texture>( SDL_CreateTextureFromSurface( renderer, surface.get() ),
                           SDL_DestroyTexture );
    if( !texture ) { return {}; }
    return { .sheet = texture.get(), .owner = texture, .u0 = 0.0f, .v0 = 0.0f, .u1 = 1.0f, .v1 = 1.0f, .valid = true };
}

auto emit_fixture( const emit_fixture_options &options ) -> void
{
    for( const auto &piece : options.pieces ) {
        auto box = piece.box;
        auto material = options.palette;
        const auto u = ( static_cast<float>( piece.finish ) + 0.5f ) / 7.0f;
        material.solid_uv = std::array<float, 2> { u, 0.5f };
        // Turn the model inside its fitted footprint; wall fitting already oriented the bounds.
        for( auto turn = 0; turn < ( options.facing & 3 ); ++turn ) {
            const auto previous = box.center[0];
            box.center[0] = 1.0f - box.center[1];
            box.center[1] = previous;
        }
        box.center[0] = options.bounds.x0 + box.center[0] * ( options.bounds.x1 - options.bounds.x0 );
        box.center[1] = options.bounds.y0 + box.center[1] * ( options.bounds.y1 - options.bounds.y0 );
        box.center[2] *= options.height;
        const auto odd = ( options.facing & 1 ) != 0;
        box.size[0] *= odd ? options.bounds.y1 - options.bounds.y0 : options.bounds.x1 - options.bounds.x0;
        box.size[1] *= odd ? options.bounds.x1 - options.bounds.x0 : options.bounds.y1 - options.bounds.y0;
        box.yaw += ( options.facing & 3 ) * std::numbers::pi_v<float> * 0.5f;
        box.size[2] *= options.height;
        const auto boxes = std::vector<prop_cuboid> { box };
        emit_prop_cuboids( { .sink = options.sink, .material = material, .boxes = boxes, .x = options.x, .y = options.y, .base = options.base } );
    }
}
#endif
} // namespace r3d
