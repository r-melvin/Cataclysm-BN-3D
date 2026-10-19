#include "catch/catch.hpp"

#if defined(TILES)

#    include "cata_tiles.h"
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
    tilecontext->load_tileset( "UltimateCataclysm", {}, false, true, false );
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

TEST_CASE("r3d_damaged_shelter_furniture_has_closed_opaque_geometry", "[r3d][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    for (const auto& id : {"f_bench_flipped", "f_wreckage"}) {
        CAPTURE(id);
        const auto furniture = furn_str_id(id).id();
        sm.set_furn(point_sm_ms(5, 5), furniture);
        const auto& info = cache.furniture({.id = furniture, .tiles = *tilecontext});
        REQUIRE(info.prop != r3d::solid_prop::none);
        REQUIRE(info.main().solid_uv.has_value());
        const auto mesh = r3d::build_chunk_mesh(
            {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
        CHECK(sm.get_furn(point_sm_ms(5, 5)) == furniture);
        CHECK(mesh.uprights.empty());
        auto elevated = size_t{0};
        for (const auto& vertex : mesh.vertices) {
            if (vertex.z <= r3d::floor_height + 0.001f) { continue; }
            ++elevated;
            CHECK(vertex.x >= 5.0f);
            CHECK(vertex.x <= 6.0f);
            CHECK(vertex.y >= -6.0f);
            CHECK(vertex.y <= -5.0f);
            CHECK(vertex.z < r3d::floor_height + 0.7f);
        }
        CHECK(elevated > 0);
    }
}

TEST_CASE("r3d_stair_flights_cross_chunk_boundaries", "[r3d][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto west = submap(pos, {});
    const auto east_pos = pos + tripoint_rel_sm(1, 0, 0);
    auto east = submap(east_pos, {});
    west.set_all_ter(t_floor);
    east.set_all_ter(t_floor);
    const auto lookup = [&](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &west : requested == east_pos ? &east : nullptr;
    };
    for (const auto& id : {"t_stairs_down", "t_stairs_up"}) {
        CAPTURE(id);
        const auto stairs = ter_str_id(id).id();
        west.set_ter(point_sm_ms(SEEX - 1, 5), stairs);
        east.set_ter(point_sm_ms(0, 5), stairs);
        auto cache = r3d::tile_info_cache{};
        const auto first = r3d::build_chunk_mesh(
            {.pos = pos,
             .lookup = lookup,
             .cache = cache,
             .tiles = *tilecontext,
             .config = {.first_person = true}});
        const auto second = r3d::build_chunk_mesh(
            {.pos = east_pos,
             .lookup = lookup,
             .cache = cache,
             .tiles = *tilecontext,
             .config = {.first_person = true}});
        CHECK(first.uprights.empty());
        CHECK(second.uprights.empty());
        const auto down = std::string_view(id) == "t_stairs_down";
        const auto has_step = [&](const auto& mesh, const float height) {
            return std::ranges::any_of(mesh.vertices, [&](const auto& v) {
                return std::abs(v.z - height) < 0.0001f;
            });
        };
        CHECK(has_step(first, r3d::floor_height + (down ? -0.375f : 0.5f)));
        CHECK(has_step(second, r3d::floor_height + (down ? -0.875f : 1.0f)));
        CHECK(west.get_ter(point_sm_ms(SEEX - 1, 5)) == stairs);
        CHECK(east.get_ter(point_sm_ms(0, 5)) == stairs);
    }
}

TEST_CASE("r3d_bench_rows_use_horizontal_seat_art_without_gaps", "[r3d][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    const auto donor = tilecontext->get_sprite_draw_data(
        {.id = "f_bench",
         .category = C_FURNITURE,
         .pos = tripoint_bub_ms(0, 0, 0),
         .subtile = 2,
         .rotation = 1,
         .variant = ""});
    REQUIRE(donor.has_value());
    const auto bench = furn_str_id("f_bench").id();
    auto cache = r3d::tile_info_cache{};
    for (auto rotation = 0; rotation < 4; ++rotation) {
        const auto& art = cache.furniture(
            {.id = bench, .tiles = *tilecontext, .subtile = 2, .rota = rotation});
        CHECK(art.main().sheet == donor->texture);
        CHECK(art.main().u0 >= donor->uv0.first);
        CHECK(art.main().u1 <= donor->uv1.first);
        CHECK(art.main().v0 >= donor->uv0.second);
        CHECK(art.main().v1 <= donor->uv1.second);
    }
    for (const auto along_y : {false, true}) {
        auto sm = submap(pos, {});
        sm.set_all_ter(t_floor);
        sm.set_furn(point_sm_ms(5, 5), bench);
        sm.set_furn(point_sm_ms(along_y ? 5 : 6, along_y ? 6 : 5), bench);
        const auto lookup = [&](const tripoint_abs_sm& requested) -> const submap* {
            return requested == pos ? &sm : nullptr;
        };
        const auto mesh = r3d::build_chunk_mesh(
            {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
        const auto height = cache.furniture({.id = bench, .tiles = *tilecontext}).sh.height;
        auto contacts = 0;
        for (auto i = size_t{0}; i + 3 < mesh.vertices.size(); i += 4) {
            auto top = true;
            auto seam = false;
            for (auto j = size_t{0}; j < 4; ++j) {
                const auto& v = mesh.vertices[i + j];
                top = top && v.z == Approx(height);
                seam = seam || (along_y ? -v.y : v.x) == Approx(6.0f);
            }
            if (top && seam) { ++contacts; }
        }
        CHECK(contacts == 2);
    }
}

TEST_CASE("r3d_locker_depth_stays_constant_at_thin_wall_corners", "[r3d][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    const auto& locker = cache.furniture(
        {.id = furn_str_id("f_locker").id(), .tiles = *tilecontext});
    for (auto facing = 0; facing < 4; ++facing) {
        for (const auto walls :
             {uint8_t{0}, uint8_t(r3d::face_north), uint8_t(r3d::face_north | r3d::face_west)}) {
            const auto fitted = r3d::place_furniture_box(
                {.box = {},
                 .flip = false,
                 .facing = facing,
                 .joined = 0,
                 .wall_contacts = walls,
                 .depth_scale = locker.depth_scale});
            const auto bounds = r3d::furniture_profile_bounds(
                {.fitted = fitted,
                 .profile = locker.proportions,
                 .facing = facing,
                 .wall_contacts = walls});
            CHECK(((facing & 1) ? bounds.x1 - bounds.x0 : bounds.y1 - bounds.y0) == Approx(0.5f));
        }
    }
}


TEST_CASE("r3d_missing_furniture_art_is_an_upright_marker", "[r3d][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = furn_str_id("test_r3d_missing_art_furniture").id();
    auto cache = r3d::tile_info_cache{};
    const auto& art = cache.furniture({.id = id, .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    CHECK(art.main().placeholder);
    CHECK(art.sh.kind == r3d::shape_kind::upright);
    const auto donor = tilecontext->get_sprite_draw_data(
        {.id = "test_r3d_missing_art_furniture",
         .category = C_FURNITURE,
         .pos = tripoint_bub_ms(0, 0, 0),
         .variant = ""});
    REQUIRE(donor);
    CHECK(art.main().sheet == donor->texture);
    CHECK(art.main().u0 >= donor->uv0.first);
    CHECK(art.main().u1 <= donor->uv1.first);
    CHECK(art.main().v0 >= donor->uv0.second);
    CHECK(art.main().v1 <= donor->uv1.second);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    sm.set_furn(point_sm_ms(5, 5), id);
    const auto lookup = [&](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    REQUIRE(mesh.uprights.size() == 1);
    REQUIRE(mesh.uprights.front().info != nullptr);
    CHECK(mesh.uprights.front().info->main().sheet == art.main().sheet);
    CHECK(mesh.uprights.front().info->main().placeholder);
    CHECK(sm.get_furn(point_sm_ms(5, 5)) == id);
}

#endif
