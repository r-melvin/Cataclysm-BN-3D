#include "catch/catch.hpp"
#include "path_info.h"
#include "r3d_item_profiles.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>

#if defined(TILES)
#    include "avatar.h"
#    include "cata_tiles.h"
#    include "cata_utility.h"
#    include "item.h"
#    include "map/map.h"
#    include "map_helpers.h"
#    include "monster.h"
#    include "options.h"
#    include "options_helpers.h"
#    include "r3d_entities.h"
#    include "sdltiles.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"

TEST_CASE("r3d_avatar_only_mesh_excludes_loose_items", "[r3d][entities][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("rock", calendar::turn));

    auto cache = r3d::tile_info_cache{};
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = r3d::camera{},
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto full = r3d::build_entity_mesh(view);
    view.avatar_only = true;
    const auto with_item = r3d::build_entity_mesh(view);
    here.i_clear(origin);
    const auto without_item = r3d::build_entity_mesh(view);

    CHECK_FALSE(with_item.empty());
    CHECK(full.indices.size() > with_item.indices.size());
    CHECK(with_item.vertices.size() == without_item.vertices.size());
    CHECK(with_item.indices.size() == without_item.indices.size());
}

TEST_CASE("r3d_voxel_item_menu_option_switches_tank_geometry", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("carbondioxide_tank", calendar::turn));

    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    cache.set_voxel_items(true);
    const auto& art = cache.named(
        {.category = C_ITEM, .id = "carbondioxide_tank", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE_FALSE(art.backdrop.valid);
    REQUIRE(art.main().alpha_mask);
    REQUIRE(art.main().alpha_width > 0);
    REQUIRE(art.main().alpha_height > 0);
    REQUIRE(art.main().voxel_surface);

    const auto voxelized = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto billboard = r3d::build_entity_mesh(view);
    CHECK(voxelized.vertices.size() > billboard.vertices.size() + 16);
    CHECK(voxelized.indices.size() > billboard.indices.size() + 24);
}

TEST_CASE("r3d_voxel_item_option_extrudes_generic_item_sprites", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("rock", calendar::turn));

    auto cache = r3d::tile_info_cache{};
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = r3d::camera{},
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    cache.set_voxel_items(true);
    const auto& art = cache.named({.category = C_ITEM, .id = "rock", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE_FALSE(art.backdrop.valid);
    REQUIRE(art.main().alpha_mask);
    REQUIRE(art.main().voxel_surface);

    const auto voxelized = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto billboard = r3d::build_entity_mesh(view);
    CHECK(voxelized.vertices.size() > billboard.vertices.size() + 16);
    CHECK(voxelized.indices.size() > billboard.indices.size() + 24);

    cache.set_voxel_items(false);
    const auto& flat = cache.named({.category = C_ITEM, .id = "rock", .tiles = *tilecontext});
    REQUIRE(flat.main().valid);
    CHECK_FALSE(flat.main().alpha_mask);
    CHECK_FALSE(flat.main().voxel_surface);
}

TEST_CASE(
    "r3d_authored_voxel_item_uses_voxel_mesh_and_respects_menu_option", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);

    auto& here = get_map();
    auto& you = get_avatar();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    you.setpos(origin);
    here.add_item(origin, item::spawn("carbondioxide_tank", calendar::turn));

    auto model = r3d::vox_model{.size_x = 8, .size_y = 8, .size_z = 8, .palette = {}, .cells = {}};
    for (auto z = 0; z < model.size_z; ++z) {
        for (auto y = 0; y < model.size_y; ++y) {
            for (auto x = 0; x < model.size_x; ++x) {
                model.cells.push_back(
                    {.x = static_cast<uint8_t>(x),
                     .y = static_cast<uint8_t>(y),
                     .z = static_cast<uint8_t>(z),
                     .color_index = 1});
            }
        }
    }
    const auto surface = r3d::extract_vox_surface(model);
    REQUIRE(surface.has_value());
    auto palette = r3d::sprite_ref{};
    palette.valid = true;
    const auto assets = std::unordered_map<std::string, r3d::vox_asset>{
        {"carbondioxide_tank",
         {.model = std::move(model), .surface = *surface, .palette = palette}}};

    auto cache = r3d::tile_info_cache{};
    const auto cam = r3d::camera{};
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers = {},
        .uprights = {},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; },
        .item_vox_models = &assets};

    const auto authored = r3d::build_entity_mesh(view);
    const auto profiles = r3d::presentation_profile_map{
        {"item:carbondioxide_tank",
         {.size = {0.48f, 0.34f, 0.5f}, .support_anchor = {0.25f, 0.75f, 0.2f}}}};
    view.presentation_profiles = &profiles;
    const auto profiled = r3d::build_entity_mesh(view);
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto fallback = r3d::build_entity_mesh(view);
    CHECK(authored.vertices.size() == fallback.vertices.size() + 20);
    CHECK(authored.indices.size() == fallback.indices.size() + 30);
    REQUIRE(profiled.vertices.size() == authored.vertices.size());
    auto authored_height = 0.0f;
    auto profiled_height = 0.0f;
    const auto same_vertex = [](const r3d::vertex& left, const r3d::vertex& right) {
        return left.x == right.x && left.y == right.y && left.z == right.z && left.u == right.u
            && left.v == right.v;
    };
    auto item_start = size_t{0};
    while (item_start < fallback.vertices.size()
           && same_vertex(authored.vertices[item_start], fallback.vertices[item_start])) {
        ++item_start;
    }
    REQUIRE(item_start + 24 <= authored.vertices.size());
    for (auto index = item_start; index < item_start + 24; ++index) {
        authored_height = std::max(authored_height, authored.vertices[index].z);
        profiled_height = std::max(profiled_height, profiled.vertices[index].z);
    }
    auto authored_bottom = std::numeric_limits<float>::max();
    auto profiled_bottom = std::numeric_limits<float>::max();
    for (auto index = item_start; index < item_start + 24; ++index) {
        authored_bottom = std::min(authored_bottom, authored.vertices[index].z);
        profiled_bottom = std::min(profiled_bottom, profiled.vertices[index].z);
    }
    CHECK(authored_height - authored_bottom == Approx(1.0f));
    CHECK(profiled_height - profiled_bottom == Approx(0.5f));
    CHECK(profiled_bottom == Approx(authored_bottom - 0.1f));
}

TEST_CASE("r3d_particles_face_the_first_person_camera", "[r3d][entities][tiles]") {
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto cache = r3d::tile_info_cache{};
    auto sprite =
        cache.named({.category = C_TERRAIN, .id = "t_grass", .tiles = *tilecontext}).main();
    REQUIRE(sprite.valid);
    REQUIRE(sprite.sheet != nullptr);
    sprite.opaque_x0 = 0;
    sprite.opaque_y0 = 0;
    sprite.opaque_x1 = 1;
    sprite.opaque_y1 = 1;
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::first_person;
    cam.z_scale = 1.8f;
    for (const auto heading : {0, 2, 5}) {
        for (const auto pitch : {0.0f, 0.5f}) {
            cam.heading = heading;
            cam.look_pitch = pitch;
            const auto center = cam.eye() + cam.fp_forward() * 5.0f;
            auto view = r3d::entity_view{.tiles = *tilecontext, .cache = cache, .cam = cam};
            view.particles.push_back(
                {.x = center.x,
                 .y = -center.y,
                 .z = center.z / cam.z_scale,
                 .size = 1.0f,
                 .sprite = &sprite,
                 .color = 0xffffffff});
            const auto mesh = r3d::build_particle_mesh(view);
            REQUIRE(mesh.vertices.size() == 4);
            REQUIRE(mesh.batches.size() == 1);
            CHECK(mesh.batches.front().sheet == sprite.sheet);
            CHECK(mesh.vertices[0].u == Approx(sprite.u0));
            CHECK(mesh.vertices[0].v == Approx(sprite.v0));
            CHECK(mesh.vertices[2].u == Approx(sprite.u1));
            CHECK(mesh.vertices[2].v == Approx(sprite.v1));
            auto projected = std::array<r3d::vec3, 4>{};
            for (auto i = size_t{0}; i < projected.size(); ++i) {
                const auto& vertex = mesh.vertices[i];
                projected[i] = r3d::transform_point(
                    cam.view_projection(1.0f), {.x = vertex.x, .y = vertex.y, .z = vertex.z});
                CHECK(projected[i].z == Approx(projected[0].z).margin(1e-5));
            }
            CHECK(projected[0].x == Approx(projected[3].x).margin(1e-5));
            CHECK(projected[1].x == Approx(projected[2].x).margin(1e-5));
            CHECK(projected[0].y == Approx(projected[1].y).margin(1e-5));
            CHECK(projected[2].y == Approx(projected[3].y).margin(1e-5));
            CHECK(projected[1].x > projected[0].x);
            CHECK(projected[0].y > projected[3].y);
        }
    }
}
TEST_CASE(
    "r3d_bundled_doll_profile_keeps_the_toy_smaller_than_a_character", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    auto input = std::ifstream(PATH_INFO::datadir() + "raw/r3d/presentation_profiles.json");
    REQUIRE(input.good());
    const auto profiles = r3d::load_presentation_profiles(input);
    const auto& profile = profiles.at("item:talking_doll");
    const auto origin = tripoint_bub_ms(60, 60, 0);
    get_avatar().setpos(origin);
    auto& here = get_map();
    here.add_item(origin, item::spawn("talking_doll", calendar::turn));
    auto cache = r3d::tile_info_cache{};
    cache.set_voxel_items(true);
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::first_person;
    auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; },
        .presentation_profiles = &profiles,
        .upright_billboards = true};
    const auto mesh = r3d::build_entity_mesh(view);
    REQUIRE_FALSE(mesh.empty());
    auto min_z = std::numeric_limits<float>::max();
    auto max_z = std::numeric_limits<float>::lowest();
    for (const auto& vertex : mesh.vertices) {
        min_z = std::min(min_z, vertex.z);
        max_z = std::max(max_z, vertex.z);
    }
    const auto& sprite =
        cache.named({.category = C_ITEM, .id = "talking_doll", .tiles = *tilecontext}).main();
    CHECK(max_z - min_z == Approx(profile.size[2] * (sprite.opaque_y1 - sprite.opaque_y0)));
    CHECK(max_z - min_z < 0.25f);
    CHECK(here.i_at(origin).size() == 1);
    here.i_clear(origin);
    CHECK(r3d::build_entity_mesh(view).empty());
}

TEST_CASE("r3d_missing_item_art_stays_readable_with_voxel_items_enabled", "[r3d][items][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto voxel_items = override_option("R3D_VOXEL_ITEMS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    get_avatar().setpos(origin);
    auto& here = get_map();
    here.add_item(origin, item::spawn("test_r3d_missing_art_item", calendar::turn));
    auto cache = r3d::tile_info_cache{};
    cache.set_voxel_items(true);
    const auto& art = cache.named(
        {.category = C_ITEM, .id = "test_r3d_missing_art_item", .tiles = *tilecontext});
    REQUIRE(art.main().valid);
    REQUIRE(art.main().placeholder);
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::first_person;
    const auto view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto enabled = r3d::build_entity_mesh(view);
    REQUIRE_FALSE(enabled.empty());
    get_options().get_option("R3D_VOXEL_ITEMS").setValue("false");
    const auto disabled = r3d::build_entity_mesh(view);
    CHECK(enabled.vertices.size() == disabled.vertices.size());
    CHECK(enabled.indices == disabled.indices);
    CHECK(here.i_at(origin).size() == 1);
}

TEST_CASE(
    "r3d_targeted_creature_renders_red_vats_highlight_without_white_cursor_box",
    "[r3d][entities][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    const auto enemy_pos = origin + tripoint_rel_ms(2, 0, 0);
    get_avatar().setpos(origin);
    auto& zombie = spawn_test_monster("mon_zombie", enemy_pos);
    REQUIRE(zombie.bub_pos() == enemy_pos);
    get_map().update_visibility_cache(0);
    get_avatar().recalc_sight_limits();

    auto cache = r3d::tile_info_cache{};
    const auto& cursor_sprite = cache.named(
        {.category = C_NONE, .id = "cursor", .tiles = *tilecontext});
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::orbit;

    auto untargeted_view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .z_lo = 0,
        .z_hi = 0,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .markers =
            {{.pos = enemy_pos, .top = r3d::floor_height, .sprite = &cursor_sprite, .box = true}},
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto untargeted_mesh = r3d::build_entity_mesh(untargeted_view);

    auto targeted_view = untargeted_view;
    targeted_view.targeted_creature = enemy_pos;
    const auto targeted_mesh = r3d::build_entity_mesh(targeted_view);

    targeted_view.target_outline_only = true;
    const auto outline_mesh = r3d::build_entity_mesh(targeted_view);
    targeted_view.target_outline_only = false;

    targeted_view.target_body_only = true;
    const auto body_mesh = r3d::build_entity_mesh(targeted_view);
    targeted_view.target_body_only = false;

    REQUIRE_FALSE(body_mesh.empty());
    REQUIRE_FALSE(outline_mesh.empty());
    // 4-way expanded red outline has 4x the vertices of the targeted creature's body billboard
    CHECK(outline_mesh.vertices.size() == body_mesh.vertices.size() * 4);

    // Every vertex in the outline mesh is colored bright VATS red
    const auto expected_outline_red = r3d::pack_light(255, 45, 40) & 0x00FFFFFFu;
    for (const auto light : outline_mesh.light) {
        CHECK((light & 0x00FFFFFFu) == expected_outline_red);
    }

    // Every vertex in the targeted body mesh is tinted crimson red
    const auto expected_body_red = r3d::pack_light(255, 95, 90) & 0x00FFFFFFu;
    for (const auto light : body_mesh.light) { CHECK((light & 0x00FFFFFFu) == expected_body_red); }

    // The targeted mesh replaces the 16-vertex 3D white cursor box with the 4-way red outline
    // (net difference: +4 * body_vertices - 16 vertices from suppressing the 4-sided cursor box)
    CHECK(targeted_mesh.vertices.size() + 16
          == untargeted_mesh.vertices.size() + outline_mesh.vertices.size());
}

TEST_CASE(
    "r3d_deaf_effect_overlay_is_excluded_and_beams_and_hit_flash_render",
    "[r3d][entities][tiles]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    tilecontext->load_tileset("UltimateCataclysm", {}, false, true, false);
    const auto origin = tripoint_bub_ms(60, 60, 0);
    const auto enemy_pos = origin + tripoint_rel_ms(2, 0, 0);
    auto& you = get_avatar();
    you.setpos(origin);
    you.add_effect(efftype_id("deaf"), 10_minutes);
    REQUIRE(you.has_effect(efftype_id("deaf")));

    const auto char_layers = tilecontext->get_character_layers(you);
    for (const auto& layer : char_layers.ids) {
        CHECK(layer.first.find("effect_") == std::string::npos);
    }

    auto& zombie = spawn_test_monster("mon_zombie", enemy_pos);
    REQUIRE(zombie.bub_pos() == enemy_pos);
    get_map().update_visibility_cache(0);
    you.recalc_sight_limits();

    auto cache = r3d::tile_info_cache{};
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::orbit;

    auto base_view = r3d::entity_view{
        .tiles = *tilecontext,
        .cache = cache,
        .cam = cam,
        .center = origin,
        .z_lo = 0,
        .z_hi = 0,
        .light = [](const tripoint_bub_ms& /*p*/) { return r3d::pack_light(255, 255, 255); },
        .surface = [](const tripoint_bub_ms& /*p*/) { return r3d::floor_height; }};
    const auto base_mesh = r3d::build_entity_mesh(base_view);

    auto fx_view = base_view;
    const auto flash_col = r3d::pack_light(255, 245, 240);
    fx_view.hit_flash_creature = enemy_pos;
    fx_view.hit_flash_color = flash_col;
    fx_view.beams.push_back(
        {.from = {0.5f, 0.5f, 0.45f},
         .to = {4.5f, 0.5f, 0.45f},
         .width = 0.03f,
         .color = r3d::pack_light(85, 240, 110)});
    const auto fx_mesh = r3d::build_entity_mesh(fx_view);
    // One 3D beam segment emits 4 crossed ribbon quads = 16 vertices and 24 indices
    CHECK(fx_mesh.vertices.size() == base_mesh.vertices.size() + 16);
    CHECK(fx_mesh.indices.size() == base_mesh.indices.size() + 24);

    fx_view.target_body_only = true;
    const auto flash_body_mesh = r3d::build_entity_mesh(fx_view);
    REQUIRE_FALSE(flash_body_mesh.empty());
    for (const auto light : flash_body_mesh.light) {
        CHECK((light & 0x00FFFFFFu) == (flash_col & 0x00FFFFFFu));
    }
}

#endif
