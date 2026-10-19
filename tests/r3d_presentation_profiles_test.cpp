#include "catch/catch.hpp"
#include "r3d_presentation_profiles.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE(
    "r3d_presentation_profiles_load_dimensions_and_anchors_alongside_gallery_metadata",
    "[r3d][profiles]") {
    auto input = std::istringstream(R"({
        "version": 1,
        "profiles": [{
            "category": "item",
            "id": "carbondioxide_tank",
            "size": [0.48, 0.34, 0.86],
            "support_anchor": [0.5, 0.5, 0.0],
            "depth_fraction": 0.38,
            "material_regions": [
                { "name": "tank_shell", "role": "painted metal", "palette_indices": [1, 2] }
            ]
        }]
    })");
    const auto profiles = r3d::load_presentation_profiles(input);
    const auto& profile = profiles.at("item:carbondioxide_tank");
    CHECK(profile.size[0] == Approx(0.48f));
    CHECK(profile.size[1] == Approx(0.34f));
    CHECK(profile.size[2] == Approx(0.86f));
    CHECK((profile.support_anchor == std::array<float, 3>{0.5f, 0.5f, 0.0f}));
    CHECK(profile.depth_fraction == Approx(0.38f));
}

TEST_CASE("r3d_presentation_profiles_reject_invalid_anchors", "[r3d][profiles]") {
    const auto invalid_anchor = std::string(R"({ "version": 1, "profiles": [{
        "category": "item", "id": "bad", "support_anchor": [1.1, 0.5, 0]
    }] })");
    auto anchor_input = std::istringstream(invalid_anchor);
    CHECK_THROWS(r3d::load_presentation_profiles(anchor_input));
}

TEST_CASE("r3d_profile_face_sources_validate_identity_crop_and_ambiguity", "[r3d][profiles]") {
    const auto bad_source = GENERATE(
        R"({"face":"side","tileset":"","sprite":"t_wall"})",
        R"({"face":"front","tileset":"test","sprite":"t_wall"})",
        R"({"face":"top","tileset":"test","sprite":"t_wall","crop":[0,0,2,1]})",
        R"({"face":"top","tileset":"test","sprite":"t_wall","crop":[0.5,0,0.5,1]})",
        R"({"face":"side","tileset":"test","sprite":"t_wall","rotation":4})",
        R"({"face":"side","tileset":"test","sprite":""})");
    auto input = std::istringstream(
        std::string(
            R"({"version":1,"profiles":[{"category":"terrain","id":"t_wall","face_sources":[)")
        + bad_source + "]}]}");
    CHECK_THROWS(r3d::load_presentation_profiles(input));
    auto duplicate = std::istringstream(R"({"version":1,"profiles":[{
        "category":"terrain","id":"t_wall","face_sources":[
        {"face":"top","tileset":"test","sprite":"t_wall"},
        {"face":"top","tileset":"test","sprite":"t_grass"}]}]})");
    CHECK_THROWS(r3d::load_presentation_profiles(duplicate));
}

#if defined(TILES)
#    include "cata_tiles.h"
#    include "map/map.h"
#    include "map/mapdata.h"
#    include "map/submap.h"
#    include "r3d_mesh.h"
#    include "r3d_profile_materials.h"
#    include "sdltiles.h"
#    include "tile_helpers.h"

TEST_CASE(
    "r3d_wall_profile_uses_bound_donor_sheet_and_crop_then_restores_fallback",
    "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    cache.set_wall_sides(std::make_shared<r3d::wall_sides>());
    const auto original = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    REQUIRE(original.elevation.valid);
    const auto donor =
        cache.named({.category = C_TERRAIN, .id = "t_grass", .tiles = *tilecontext}).main();
    REQUIRE(donor.valid);
    auto source = r3d::profile_face_source{
        .face = r3d::profile_face::top,
        .tileset = "UltimateCataclysm",
        .sprite = "t_grass",
        .crop = {0.25f, 0.25f, 0.75f, 0.75f}};
    const auto face = GENERATE(r3d::profile_face::top, r3d::profile_face::side);
    source.face = face;
    const auto scenario = GENERATE(0, 1, 2);
    CAPTURE(scenario);
    if (scenario == 1) { source.tileset = "another_tileset"; }
    if (scenario == 2) { source.sprite = "missing_profile_donor"; }
    auto profile = r3d::presentation_profile{.face_sources = {source}};
    cache.set_presentation_profiles({{"terrain:t_wall", profile}});
    const auto art = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    const auto& material = face == r3d::profile_face::top ? art.cap : art.side;
    const auto& fallback = face == r3d::profile_face::top ? original.cap : original.side;
    if (scenario != 0) {
        CHECK(material.sheet == fallback.sheet);
        CHECK(material.valid == fallback.valid);
        CHECK(material.u0 == fallback.u0);
        return;
    }
    const auto cropped = r3d::crop_profile_sprite(
        {.sprite = donor, .source = source, .tileset = "UltimateCataclysm"});
    REQUIRE(cropped);
    REQUIRE(material.valid);
    CHECK(material.sheet == donor.sheet);
    CHECK(material.owner == donor.owner);
    CHECK(material.u0 == Approx(cropped->u0));
    CHECK(material.v1 == Approx(cropped->v1));
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    sm.set_ter(point_sm_ms(5, 5), t_wall);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    auto found_cap = false;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != donor.sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& vertex = mesh.vertices[mesh.indices[i]];
            if (vertex.z == Approx(art.sh.height) && vertex.u == Approx(cropped->u0)
                && vertex.v == Approx(cropped->v0)) {
                found_cap = true;
            }
        }
    }
    CHECK(found_cap);
    cache.set_presentation_profiles({});
    const auto restored = cache.terrain({.id = t_wall, .tiles = *tilecontext});
    CHECK(restored.cap.sheet == original.cap.sheet);
    CHECK(restored.cap.valid == original.cap.valid);
    CHECK(restored.cap.u0 == original.cap.u0);
}

TEST_CASE("r3d_opening_profiles_preserve_native_geometry_and_leaf_art", "[r3d][profiles][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto id = std::string(
        GENERATE("t_window", "t_window_open", "t_window_frame", "t_window_empty", "t_door_c",
                 "t_door_o", "t_door_b", "t_door_frame"));
    const auto thin = GENERATE(false, true);
    const auto along_y = GENERATE(false, true);
    CAPTURE(id, thin, along_y);
    const auto pos = tripoint_abs_sm(get_map().get_abs_sub(), 0);
    auto sm = submap(pos, {});
    sm.set_all_ter(t_floor);
    const auto terrain_id = ter_str_id(id).id();
    sm.set_ter(point_sm_ms(5, 5), terrain_id);
    sm.set_ter(point_sm_ms(along_y ? 5 : 4, along_y ? 4 : 5), t_wall);
    sm.set_ter(point_sm_ms(along_y ? 5 : 6, along_y ? 6 : 5), t_wall);
    const auto lookup = [&sm, &pos](const tripoint_abs_sm& requested) -> const submap* {
        return requested == pos ? &sm : nullptr;
    };
    auto cache = r3d::tile_info_cache{};
    cache.set_thin_walls(thin);
    const auto original = cache.terrain({.id = terrain_id, .tiles = *tilecontext});
    REQUIRE(original.sh.kind == r3d::shape_kind::pane);
    if (id == "t_window_frame" || id == "t_window_empty") {
        CHECK(original.window);
        CHECK_FALSE(terrain_id.obj().has_flag("WINDOW"));
    }
    const auto baseline = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    auto cap_source = r3d::profile_face_source{
        .face = r3d::profile_face::top,
        .tileset = "UltimateCataclysm",
        .sprite = "t_grass",
        .rotation = 1,
        .crop = {0.23f, 0.19f, 0.71f, 0.83f}};
    auto side_source = cap_source;
    side_source.face = r3d::profile_face::side;
    cache.set_presentation_profiles(
        {{"terrain:" + id, {.face_sources = {cap_source, side_source}}}});
    const auto art = cache.terrain({.id = terrain_id, .tiles = *tilecontext});
    REQUIRE(art.cap.valid);
    REQUIRE(art.side.valid);
    const auto donor =
        cache.named({.category = C_NONE, .id = "t_grass", .tiles = *tilecontext, .rota = 1}).main();
    CHECK(art.cap.sheet == donor.sheet);
    CHECK(art.side.sheet == donor.sheet);
    CHECK(art.sprite.sheet == original.sprite.sheet);
    CHECK(art.sprite.u0 == original.sprite.u0);
    CHECK(art.open_door == original.open_door);
    CHECK(art.closed_door_id == original.closed_door_id);
    CHECK(art.see_through == original.see_through);
    const auto mesh = r3d::build_chunk_mesh(
        {.pos = pos, .lookup = lookup, .cache = cache, .tiles = *tilecontext});
    CHECK(mesh.indices.size() == baseline.indices.size());
    const auto positions = [](const auto& geometry) {
        auto result = std::vector<std::array<float, 3>>{};
        for (const auto& vertex : geometry.vertices) {
            result.push_back({vertex.x, vertex.y, vertex.z});
        }
        std::ranges::sort(result);
        return result;
    };
    CHECK(positions(mesh) == positions(baseline));
    const auto raw_uvs = r3d::uvs_t{
        {{art.cap.u0, art.cap.v0},
         {art.cap.u1, art.cap.v0},
         {art.cap.u1, art.cap.v1},
         {art.cap.u0, art.cap.v1}}};
    const auto expected = r3d::orient_material_uvs(art.cap, raw_uvs)[0];
    const auto inset = thin ? 0.5f - r3d::wall_half_thickness : 0.0f;
    const auto cap_x = 5.0f + (along_y ? inset : 0.0f);
    const auto cap_y = -(5.0f + (along_y ? 0.0f : inset));
    auto found_cap = false;
    for (const auto& batch : mesh.batches) {
        if (batch.sheet != donor.sheet) { continue; }
        for (auto i = batch.first_index; i < batch.first_index + batch.index_count; ++i) {
            const auto& vertex = mesh.vertices[mesh.indices[i]];
            if (vertex.x == Approx(cap_x) && vertex.y == Approx(cap_y) && vertex.z == Approx(1.0f)
                && vertex.u == Approx(expected[0]) && vertex.v == Approx(expected[1])) {
                found_cap = true;
            }
        }
    }
    CHECK(found_cap);
}

TEST_CASE(
    "r3d_material_uv_orientation_keeps_partial_regions_in_the_source_crop", "[r3d][profiles]") {
    const auto sprite =
        r3d::sprite_ref{.u0 = 0.2f, .v0 = 0.3f, .u1 = 0.6f, .v1 = 0.9f, .quarter_turns = 1};
    const auto part = r3d::uvs_t{{{0.3f, 0.6f}, {0.4f, 0.6f}, {0.4f, 0.9f}, {0.3f, 0.9f}}};
    const auto oriented = r3d::orient_material_uvs(sprite, part);
    // The first point is (1/4, 1/2) of the crop: clockwise rotation maps it to (1/2, 3/4).
    CHECK(oriented[0][0] == Approx(0.4f));
    CHECK(oriented[0][1] == Approx(0.75f));
    auto unrotated = sprite;
    unrotated.quarter_turns = 0;
    CHECK(r3d::orient_material_uvs(unrotated, part) == part);
}
#endif
