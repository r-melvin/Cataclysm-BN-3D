#include "catch/catch.hpp"
#include "r3d_fixture_geometry.h"
#include "r3d_vehicle_details.h"

#include <algorithm>
#include <cmath>
#include <ranges>

TEST_CASE("r3d_sink_has_a_recess_and_a_raised_tap", "[r3d][fixtures]") {
    const auto pieces = r3d::appliance_pieces(r3d::appliance_kind::sink);
    REQUIRE_FALSE(pieces.empty());
    CHECK(std::ranges::any_of(pieces, [](const auto& piece) {
        return piece.detail == r3d::fixture_detail::tap
            && piece.box.center[2] + piece.box.size[2] * 0.5f > 1.2f;
    }));
    CHECK_FALSE(std::ranges::any_of(pieces, [](const auto& piece) {
        const auto& b = piece.box;
        return std::abs(b.center[0] - 0.5f) < b.size[0] * 0.5f
            && std::abs(b.center[1] - 0.5f) < b.size[1] * 0.5f
            && std::abs(b.center[2] - 0.95f) < b.size[2] * 0.5f;
    }));
}

TEST_CASE("r3d_oven_door_and_handle_are_on_the_front_and_rings_are_on_the_hob", "[r3d][fixtures]") {
    const auto pieces = r3d::appliance_pieces(r3d::appliance_kind::oven);
    REQUIRE_FALSE(pieces.empty());
    for (const auto& piece : pieces) {
        if (piece.detail == r3d::fixture_detail::oven_door) {
            CHECK(piece.box.center[1] > 0.95f);
            CHECK(piece.box.center[2] + piece.box.size[2] * 0.5f < 0.8f);
        }
        if (piece.detail == r3d::fixture_detail::burner) { CHECK(piece.box.center[2] > 1.0f); }
    }
    CHECK(std::ranges::any_of(pieces, [](const auto& piece) {
        return piece.finish == r3d::fixture_finish::glass;
    }));
}

TEST_CASE("r3d_rotor_uses_native_diameter_and_preserves_broken_stubs", "[r3d][vehicle]") {
    const auto diameter = GENERATE(6.0f, 8.0f, 15.0f);
    const auto intact = r3d::rotor_pieces({.diameter = diameter, .blades = 4});
    const auto broken = r3d::rotor_pieces({.diameter = diameter, .blades = 4, .broken = true});
    REQUIRE(intact.size() == 6);
    REQUIRE(broken.size() == intact.size());
    const auto& blade = intact[2].box;
    CHECK(std::hypot(blade.center[0] - 0.5f, blade.center[1] - 0.5f) + blade.size[0] * 0.5f
          == Approx(diameter * 0.5f));
    CHECK(broken[2].box.size[0] < intact[2].box.size[0]);
    CHECK(broken[0].box.size == intact[0].box.size);
    CHECK(r3d::rotor_pieces({.diameter = 0.0f}).empty());
}

TEST_CASE("r3d_radio_supports_have_open_lattice_faces_over_the_full_height", "[r3d][fixtures]") {
    const auto pieces = r3d::radio_tower_pieces(6.0f);
    CHECK(std::ranges::count(pieces, r3d::fixture_detail::leg, &r3d::fixture_piece::detail) == 4);
    CHECK(std::ranges::any_of(pieces, [](const auto& p) {
        return p.detail == r3d::fixture_detail::brace && p.box.center[2] > 5.0f;
    }));
    CHECK(std::ranges::none_of(pieces, [](const auto& p) {
        return p.box.size[0] > 0.9f && p.box.size[1] > 0.9f;
    }));
    CHECK(r3d::roof_crown(0.0f) > r3d::roof_crown(1.0f));
    CHECK(r3d::roof_crown(-0.5f) == r3d::roof_crown(0.5f));
}

#if defined(TILES)
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "map/map.h"
#    include "map/mapdata.h"
#    include "map/submap.h"
#    include "map_helpers.h"
#    include "r3d_mesh.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"

#    include <memory>
#    include <span>

TEST_CASE(
    "r3d_detailed_appliances_bind_opaque_model_materials_at_every_facing",
    "[r3d][fixtures][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    const auto tileset = std::string(GENERATE("UltimateCataclysm", "hoder"));
    tilecontext->load_tileset(tileset, {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    const auto kind = GENERATE(r3d::appliance_kind::sink, r3d::appliance_kind::oven);
    const auto facing = GENERATE(0, 1, 2, 3);
    const auto& palette = cache.fixture_palette();
    REQUIRE(palette.valid);
    auto sink = r3d::quad_sink{};
    r3d::emit_fixture(
        {.sink = sink,
         .palette = palette,
         .pieces = r3d::appliance_pieces(kind),
         .bounds = {.x0 = 0.0f, .x1 = 1.0f, .y0 = 0.0f, .y1 = 1.0f},
         .x = 5,
         .y = 5,
         .base = r3d::floor_height,
         .height = 0.4f,
         .facing = facing});
    const auto mesh = sink.finish();
    REQUIRE_FALSE(mesh.vertices.empty());
    for (const auto& batch : mesh.batches) { CHECK(batch.sheet == palette.sheet); }
    const auto detail_color =
        kind == r3d::appliance_kind::oven ? r3d::fixture_finish::glass : r3d::fixture_finish::steel;
    const auto u = (static_cast<float>(detail_color) + 0.5f) / 7.0f;
    CHECK(std::ranges::any_of(mesh.vertices, [u](const auto& v) {
        return v.u == Approx(u) && v.z > r3d::floor_height;
    }));
}

TEST_CASE("r3d_native_sink_and_oven_use_the_detailed_models", "[r3d][fixtures][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    const auto tileset = std::string(GENERATE("UltimateCataclysm", "hoder"));
    tilecontext->load_tileset(tileset, {}, false, true, false);
    const auto id = std::string(GENERATE("f_sink", "f_oven", "f_dishwasher", "f_washer"));
    const auto origin = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(origin, {});
    sm.set_all_ter(t_floor);
    sm.set_furn(point_sm_ms(5, 5), furn_str_id(id).id());
    const auto lookup = [&](const tripoint_abs_sm& p) -> const submap* {
        return p == origin ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto& info = cache.furniture({.id = furn_str_id(id).id(), .tiles = *tilecontext});
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = origin, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    if (id == "f_dishwasher" || id == "f_washer") {
        CHECK(info.appliance == r3d::appliance_kind::none);
    } else if (info.main().placeholder) {
        CHECK(info.sh.kind == r3d::shape_kind::upright);
        CHECK_FALSE(mesh.uprights.empty());
    } else {
        CHECK(info.appliance
              == (id == "f_sink" ? r3d::appliance_kind::sink : r3d::appliance_kind::oven));
        CHECK(std::ranges::any_of(mesh.batches, [&](const auto& batch) {
            return batch.sheet == cache.fixture_palette().sheet;
        }));
    }
}

TEST_CASE(
    "r3d_native_radio_columns_join_across_storeys_and_invalidate_when_cut",
    "[r3d][fixtures][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto levels = std::vector<std::unique_ptr<submap>>{};
    for (auto z = 0; z < 6; ++z) {
        auto sm = std::make_unique<submap>(origin + tripoint_rel_sm(0, 0, z), dimension_id{});
        sm->set_all_ter(ter_str_id("t_open_air").id());
        sm->set_ter(point_sm_ms(5, 5), ter_str_id("t_radio_tower").id());
        levels.push_back(std::move(sm));
    }
    const auto lookup = [&](const tripoint_abs_sm& p) -> const submap* {
        const auto dz = p.z() - origin.z();
        return p.x() == origin.x() && p.y() == origin.y() && dz >= 0 && dz < 6
                 ? levels[dz].get()
                 : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto base = r3d::build_chunk_mesh(
        {.pos = origin, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    REQUIRE_FALSE(base.vertices.empty());
    CHECK(
        std::ranges::max(base.vertices, {}, &r3d::vertex::z).z == Approx(6.0f + r3d::floor_height));
    const auto upper = r3d::build_chunk_mesh(
        {.pos = origin + tripoint_rel_sm(0, 0, 1),
         .lookup = lookup,
         .cache = cache,
         .tiles = *tilecontext});
    CHECK(upper.vertices.empty());
    const auto visible_upper = r3d::build_chunk_mesh(
        {.pos = origin + tripoint_rel_sm(0, 0, 1),
         .lookup = lookup,
         .cache = cache,
         .tiles = *tilecontext,
         .config = {.lowest_visible_z = origin.z() + 1}});
    REQUIRE_FALSE(visible_upper.vertices.empty());
    CHECK(std::ranges::max(visible_upper.vertices, {}, &r3d::vertex::z).z
          == Approx(5.0f + r3d::floor_height));
    const auto visible_top = r3d::build_chunk_mesh(
        {.pos = origin + tripoint_rel_sm(0, 0, 5),
         .lookup = lookup,
         .cache = cache,
         .tiles = *tilecontext,
         .config = {.lowest_visible_z = origin.z() + 5}});
    REQUIRE_FALSE(visible_top.vertices.empty());
    CHECK(std::ranges::max(visible_top.vertices, {}, &r3d::vertex::z).z
          == Approx(1.0f + r3d::floor_height));
    const auto before = r3d::chunk_stamp(origin, lookup);
    levels[3]->set_ter(point_sm_ms(5, 5), ter_str_id("t_open_air").id());
    CHECK(r3d::chunk_stamp(origin, lookup) != before);
}
#endif
