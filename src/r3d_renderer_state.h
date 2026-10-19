#pragma once
#ifndef CATA_SRC_R3D_RENDERER_STATE_H
#define CATA_SRC_R3D_RENDERER_STATE_H

#if defined(TILES)
#include <SDL3/SDL.h>

namespace r3d
{

/// Restrict an overlay to its map rectangle and restore native SDL drawing state afterward.
class renderer_state_guard
{
        SDL_Renderer *renderer_;
        bool clipped_;
        SDL_Rect clip_{};
        SDL_FColor color_{};
        SDL_BlendMode blend_ = SDL_BLENDMODE_NONE;

    public:
        renderer_state_guard( SDL_Renderer *renderer, const SDL_Rect &bounds ) :
            renderer_( renderer ), clipped_( SDL_RenderClipEnabled( renderer ) ) {
            SDL_GetRenderClipRect( renderer_, &clip_ );
            SDL_GetRenderDrawColorFloat( renderer_, &color_.r, &color_.g, &color_.b, &color_.a );
            SDL_GetRenderDrawBlendMode( renderer_, &blend_ );
            auto clip = bounds;
            if( clipped_ ) { SDL_GetRectIntersection( &clip_, &bounds, &clip ); }
            SDL_SetRenderClipRect( renderer_, &clip );
            SDL_SetRenderDrawBlendMode( renderer_, SDL_BLENDMODE_BLEND );
        }
        ~renderer_state_guard() {
            SDL_SetRenderClipRect( renderer_, clipped_ ? &clip_ : nullptr );
            SDL_SetRenderDrawColorFloat( renderer_, color_.r, color_.g, color_.b, color_.a );
            SDL_SetRenderDrawBlendMode( renderer_, blend_ );
        }
        renderer_state_guard( const renderer_state_guard & ) = delete;
        auto operator=( const renderer_state_guard & ) -> renderer_state_guard & = delete; // *NOPAD*
};

} // namespace r3d
#endif
#endif
