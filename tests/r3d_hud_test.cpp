#if defined(TILES)

#    include "catch/catch.hpp"
#    include "options_helpers.h"
#    include "r3d_hud.h"

#    include <SDL3/SDL.h>
#    include <memory>
#    include <vector>

TEST_CASE("r3d_hud_preserves_renderer_state", "[r3d][hud]") {
    const auto hud = override_option("R3D_HUD", "false");
    const auto surface = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        SDL_CreateSurface(160, 120, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    REQUIRE(surface);
    const auto renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>(
        SDL_CreateSoftwareRenderer(surface.get()), SDL_DestroyRenderer);
    REQUIRE(renderer);
    const auto original_clip = SDL_Rect{.x = 7, .y = 9, .w = 120, .h = 100};
    REQUIRE(SDL_SetRenderClipRect(renderer.get(), &original_clip));
    REQUIRE(SDL_SetRenderDrawColor(renderer.get(), 32, 64, 96, 128));
    REQUIRE(SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_ADD));
    const auto frame = r3d::hud_frame{.w = 160, .h = 120};
    const auto air = r3d::atmosphere{};
    auto texts = std::vector<r3d::hud_text>{};

    r3d::draw_hud({.renderer = renderer.get(), .frame = frame, .air = air, .texts = texts});

    CHECK(SDL_RenderClipEnabled(renderer.get()));
    auto clip = SDL_Rect{};
    REQUIRE(SDL_GetRenderClipRect(renderer.get(), &clip));
    CHECK(clip.x == original_clip.x);
    CHECK(clip.y == original_clip.y);
    CHECK(clip.w == original_clip.w);
    CHECK(clip.h == original_clip.h);
    auto blend = SDL_BLENDMODE_NONE;
    REQUIRE(SDL_GetRenderDrawBlendMode(renderer.get(), &blend));
    CHECK(blend == SDL_BLENDMODE_ADD);
    auto red = Uint8{};
    auto green = Uint8{};
    auto blue = Uint8{};
    auto alpha = Uint8{};
    REQUIRE(SDL_GetRenderDrawColor(renderer.get(), &red, &green, &blue, &alpha));
    CHECK(red == 32);
    CHECK(green == 64);
    CHECK(blue == 96);
    CHECK(alpha == 128);
    CHECK(texts.empty());
}

#endif
