#include "catch/catch.hpp"
#include "r3d_wall_furniture.h"
#include "r3d_wall_layout.h"

TEST_CASE("r3d_shared_walls_use_a_consistent_outer_edge", "[r3d][walls]") {
    const auto layout = r3d::exterior_wall_layout(
        {.indoors = {true, true, true, true, false, true, true, true, true}});
    CHECK(layout.center_x == Approx(0.125f));
    CHECK(layout.center_y == Approx(0.125f));
    CHECK(layout.x_edges[1] == Approx(0.0f));
    CHECK(layout.y_edges[2] == Approx(0.25f));
}

TEST_CASE("r3d_wall_furniture_preserves_authored_spans_at_exterior_edges", "[r3d][walls]") {
    const auto original = r3d::box_layout{.x0 = 0.0f, .x1 = 1.0f, .y0 = -0.375f, .y1 = 0.125f};
    const auto exact = r3d::furniture_proportions{.size = {1.0f, 0.5f, 0.9f}};
    const auto moved = r3d::fit_furniture_to_wall_edges(
        {.bounds = original,
         .profile = exact,
         .contacts = r3d::face_north,
         .gaps = {0.75f, 0.0f, 0.0f, 0.0f}});
    CHECK(moved.y0 == Approx(-0.75f));
    CHECK(moved.y1 - moved.y0 == Approx(0.5f));
    const auto inferred = r3d::furniture_proportions{};
    const auto extended = r3d::fit_furniture_to_wall_edges(
        {.bounds = original,
         .profile = inferred,
         .contacts = r3d::face_north,
         .gaps = {0.75f, 0.0f, 0.0f, 0.0f}});
    CHECK(extended.y0 == Approx(-0.75f));
    CHECK(extended.y1 == Approx(original.y1));
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

#    include <algorithm>
#    include <limits>
#    include <ranges>
#    include <span>

namespace {
struct wall_tile_measurement {
    float low = std::numeric_limits<float>::max();
    float high = std::numeric_limits<float>::lowest();
    bool indoor_floor = false;
};

struct measure_wall_options {
    const r3d::chunk_mesh& mesh;
    point tile;
    bool along_y;
    const r3d::sprite_ref& floor;
};

auto measure_wall_tile(const measure_wall_options& options) -> wall_tile_measurement {
    auto result = wall_tile_measurement{};
    const auto& mesh = options.mesh;
    for (auto q = size_t{0}; q < mesh.vertices.size(); q += 4) {
        const auto& ref = mesh.light_refs[q];
        if (ref.lx != options.tile.x || ref.ly != options.tile.y) { continue; }
        const auto quad = std::span(mesh.vertices).subspan(q, 4);
        for (const auto& vertex : quad) {
            if (vertex.z > 0.95f) {
                const auto normal = options.along_y ? vertex.x : -vertex.y;
                result.low = std::min(result.low, normal);
                result.high = std::max(result.high, normal);
            }
        }
        if (!std::ranges::all_of(quad, [](const auto& v) {
                return v.z == Approx(r3d::floor_height);
            })) {
            continue;
        }
        if (!std::ranges::all_of(quad, [&options](const auto& v) {
                return v.u >= options.floor.u0 && v.u <= options.floor.u1 && v.v >= options.floor.v0
                    && v.v <= options.floor.v1;
            })) {
            continue;
        }
        for (const auto& batch : mesh.batches) {
            const auto indices =
                std::span(mesh.indices).subspan(batch.first_index, batch.index_count);
            if (batch.sheet == options.floor.sheet
                && std::ranges::contains(indices, static_cast<uint32_t>(q))) {
                result.indoor_floor = true;
                // The room floor covers the entire wall tile, not merely the old half gap.
                CHECK(std::ranges::min(quad, {}, &r3d::vertex::x).x == Approx(options.tile.x));
                CHECK(std::ranges::max(quad, {}, &r3d::vertex::x).x == Approx(options.tile.x + 1));
                CHECK(-std::ranges::max(quad, {}, &r3d::vertex::y).y == Approx(options.tile.y));
                CHECK(-std::ranges::min(quad, {}, &r3d::vertex::y).y == Approx(options.tile.y + 1));
            }
        }
    }
    return result;
}
} // namespace

TEST_CASE("r3d_exterior_walls_and_openings_follow_the_outer_tile_edge", "[r3d][walls][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = std::string(GENERATE(
        "t_wall", "t_window_domestic", "t_window_frame", "t_door_c", "t_door_o", "t_door_frame"));
    const auto side = GENERATE(0, 1, 2, 3); // north, south, west, east
    const auto thin = GENERATE(false, true);
    CAPTURE(id, side, thin);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_grass);
    for (auto y = 2; y <= 9; ++y) {
        for (auto x = 2; x <= 9; ++x) {
            sm.set_ter(point_sm_ms(x, y), x == 2 || x == 9 || y == 2 || y == 9 ? t_wall : t_floor);
        }
    }
    const auto tile =
        side == 0   ? point(5, 2)
        : side == 1 ? point(5, 9)
        : side == 2 ? point(2, 5)
                    : point(9, 5);
    sm.set_ter(point_sm_ms(tile), ter_str_id(id).id());
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    cache.set_thin_walls(thin);
    const auto floor = cache.terrain({.id = t_floor, .tiles = *tilecontext}).main();
    REQUIRE(floor.valid);
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    const auto measured = measure_wall_tile(
        {.mesh = mesh, .tile = tile, .along_y = side >= 2, .floor = floor});
    const auto normal = static_cast<float>(side >= 2 ? tile.x : tile.y);
    const auto positive_edge = side == 1 || side == 3;
    CHECK(measured.low == Approx(normal + (thin && positive_edge ? 0.75f : 0.0f)));
    CHECK(measured.high == Approx(normal + (thin && !positive_edge ? 0.25f : 1.0f)));
    CHECK(measured.indoor_floor);
}

TEST_CASE(
    "r3d_exterior_corner_uses_diagonal_indoor_floor_across_chunk_seams", "[r3d][walls][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto corner = submap(pos, {});
    auto east = submap(pos + tripoint_rel_sm(1, 0, 0), {});
    auto south = submap(pos + tripoint_rel_sm(0, 1, 0), {});
    auto diagonal = submap(pos + tripoint_rel_sm(1, 1, 0), {});
    for (auto* sm : {&corner, &east, &south, &diagonal}) { sm->set_all_ter(t_grass); }
    corner.set_ter(point_sm_ms(11, 11), t_wall);
    east.set_ter(point_sm_ms(0, 11), t_wall);
    south.set_ter(point_sm_ms(11, 0), t_wall);
    diagonal.set_ter(point_sm_ms(0, 0), t_floor);
    const auto lookup = [&](const tripoint_abs_sm& requested) -> const submap* {
        if (requested == pos) { return &corner; }
        if (requested == pos + tripoint_rel_sm(1, 0, 0)) { return &east; }
        if (requested == pos + tripoint_rel_sm(0, 1, 0)) { return &south; }
        if (requested == pos + tripoint_rel_sm(1, 1, 0)) { return &diagonal; }
        return nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    const auto floor = cache.terrain({.id = t_floor, .tiles = *tilecontext}).main();
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    CHECK(measure_wall_tile({.mesh = mesh, .tile = point(11, 11), .along_y = false, .floor = floor})
              .indoor_floor);
    auto caps = 0;
    for (auto i = size_t{0}; i < mesh.vertices.size(); ++i) {
        if (mesh.light_refs[i].lx != 11 || mesh.light_refs[i].ly != 11
            || mesh.vertices[i].z < 0.95f) {
            continue;
        }
        ++caps;
        const auto& v = mesh.vertices[i];
        CHECK((v.x <= 11.251f || -v.y <= 11.251f));
    }
    CHECK(caps > 0);
    const auto stamp = r3d::chunk_stamp(pos, lookup);
    diagonal.set_ter(point_sm_ms(0, 0), t_grass);
    CHECK(r3d::chunk_stamp(pos, lookup) != stamp);
}
TEST_CASE("r3d_furniture_remains_visible_on_the_floor_of_a_wall_tile", "[r3d][walls][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    for (auto x = 0; x < SEEX; ++x) { sm.set_ter(point_sm_ms(x, 2), t_wall); }
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    cache.set_thin_walls(true);
    const auto bare = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    sm.set_furn(point_sm_ms(5, 2), furn_str_id("f_oven").id());
    const auto furnished = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    CHECK(furnished.vertices.size() > bare.vertices.size());
    const auto top =
        cache.furniture({.id = furn_str_id("f_oven").id(), .tiles = *tilecontext}).sh.height;
    REQUIRE(top > r3d::floor_height);
    CHECK(std::ranges::any_of(furnished.vertices, [top](const auto& vertex) {
        return vertex.z == Approx(top);
    }));
}

#endif
