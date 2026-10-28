#include "catch/catch.hpp"

#if defined(TILES)
#    include "avatar.h"
#    include "avatar_action.h"
#    include "ballistics.h"
#    include "cata_utility.h"
#    include "damage.h"
#    include "dispersion.h"
#    include "game.h"
#    include "item.h"
#    include "map/legacy_pathfinding.h"
#    include "map/map.h"
#    include "map/mapdata.h"
#    include "map/utils/map_functions.h"
#    include "map_helpers.h"
#    include "monster.h"
#    include "npc.h"
#    include "options_helpers.h"
#    include "pathfinding.h"
#    include "player_helpers.h"
#    include "projectile.h"
#    include "state_helpers.h"
#    include "tile_helpers.h"
#    include "vehicle/vehicle.h"
#    include "wall_edges.h"

TEST_CASE(
    "wall_edges_retain_native_collision_when_rendering_falls_back_to_flat", "[wall_edges][tiles]") {
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    const auto contexts = tile_context_fixture(true);
    REQUIRE(contexts.valid());
    CHECK_FALSE(wall_edges::enabled());
    CHECK(wall_edges::terrain_move_cost(t_wall.obj()) == 0);
}

TEST_CASE("wall_edges_use_real_wall_boundaries_and_keep_cell_centres_free", "[wall_edges]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    auto& here = get_map();
    for (auto y = 48; y <= 60; ++y) {
        for (auto x = 48; x <= 60; ++x) {
            const auto pos = tripoint_bub_ms(x, y, 0);
            const auto border = x == 49 || x == 59 || y == 49 || y == 59;
            const auto inside = x >= 49 && x <= 59 && y >= 49 && y <= 59;
            here.ter_set(pos, inside ? border ? t_wall : t_floor : t_grass);
        }
    }
    const auto wall = tripoint_bub_ms(54, 49, 0);
    const auto inside = tripoint_bub_ms(54, 50, 0);
    const auto outside = tripoint_bub_ms(54, 48, 0);
    CHECK(wall_edges::terrain_move_cost(here.ter(wall).obj()) == 2);
    CHECK_FALSE(wall_edges::trace({.here = here, .from = inside, .to = wall}));
    CHECK(wall_edges::trace({.here = here, .from = wall, .to = outside}) == wall);
    CHECK(wall_edges::trace({.here = here, .from = outside, .to = wall}) == wall);
    CHECK_FALSE(wall_edges::trace({.here = here, .from = wall, .to = tripoint_bub_ms(55, 49, 0)}));
    CHECK(wall_edges::trace(
              {.here = here, .from = tripoint_bub_ms(49, 49, 0), .to = tripoint_bub_ms(48, 48, 0)})
          == tripoint_bub_ms(49, 49, 0));
    const auto hit = wall_edges::trace(
        {.buffer = here.get_mapbuffer(),
         .from = map_local_to_abs(here, wall),
         .to = map_local_to_abs(here, outside)});
    REQUIRE(hit);
    CHECK(hit->fraction == Approx(0.25));

    SECTION("shared interior partitions use their west edge") {
        for (auto y = 50; y <= 58; ++y) { here.ter_set(tripoint_bub_ms(54, y, 0), t_wall); }
        const auto partition = tripoint_bub_ms(54, 54, 0);
        CHECK_FALSE(
            wall_edges::trace({.here = here, .from = tripoint_bub_ms(55, 54, 0), .to = partition}));
        CHECK(wall_edges::trace({.here = here, .from = tripoint_bub_ms(53, 54, 0), .to = partition})
              == partition);
        CHECK_FALSE(
            wall_edges::trace({.here = here, .from = partition, .to = tripoint_bub_ms(54, 55, 0)}));
    }
    SECTION("opening state changes replace the actual barrier") {
        here.ter_set(wall, ter_str_id("t_door_c").id());
        CHECK(wall_edges::trace({.here = here, .from = wall, .to = outside}) == wall);
        here.ter_set(wall, ter_str_id("t_door_o").id());
        CHECK_FALSE(wall_edges::trace({.here = here, .from = wall, .to = outside}));
        here.ter_set(wall, ter_str_id("t_window_domestic").id());
        CHECK(wall_edges::trace({.here = here, .from = wall, .to = outside}) == wall);
        CHECK_FALSE(wall_edges::trace(
            {.here = here, .from = wall, .to = outside, .kind = wall_edges::trace_kind::vision}));
        here.ter_set(wall, ter_str_id("t_window_frame").id());
        CHECK_FALSE(wall_edges::trace({.here = here, .from = wall, .to = outside}));
    }
    SECTION("a body can leave existing wall contact without passing through the strip") {
        CHECK_FALSE(wall_edges::trace({.here = here, .from = wall, .to = inside, .padding = 0.5}));
        CHECK(wall_edges::trace({.here = here, .from = wall, .to = outside, .padding = 0.5}));
    }
    SECTION("a gate control is a solid device rather than a wall edge") {
        const auto& control = ter_str_id("t_gates_mech_control").obj();
        CHECK_FALSE(wall_edges::is_edge_terrain(control));
        CHECK(wall_edges::terrain_move_cost(control) == 0);
    }
    SECTION("full-tile mode retains the old terrain cost") {
        const auto full = override_option("R3D_THIN_WALLS", "false");
        CHECK(wall_edges::terrain_move_cost(here.ter(wall).obj()) == 0);
        CHECK_FALSE(wall_edges::trace({.here = here, .from = wall, .to = outside}));
    }
}

TEST_CASE(
    "wall_edges_native_player_steps_use_the_free_floor_and_stop_at_the_edge",
    "[wall_edges][movement]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    auto& here = get_map();
    auto& you = get_avatar();
    const auto inside = tripoint_bub_ms(60, 60, 0);
    const auto wall = tripoint_bub_ms(60, 59, 0);
    for (auto y = 58; y <= 62; ++y) {
        for (auto x = 58; x <= 62; ++x) {
            here.ter_set(tripoint_bub_ms(x, y, 0), y == 59 ? t_wall : y >= 60 ? t_floor : t_grass);
        }
    }
    const auto inside_abs = map_local_to_abs(here, inside);
    const auto wall_abs = map_local_to_abs(here, wall);
    const auto outside_abs = map_local_to_abs(here, tripoint_bub_ms(60, 58, 0));
    you.setpos(inside);
    you.set_moves(1000);
    CHECK(here.move_cost(wall) == 2);
    REQUIRE(avatar_action::move(you, here, tripoint_rel_ms::north()));
    REQUIRE(map_local_to_abs(here, you.bub_pos()) == wall_abs);
    const auto moves_before = you.moves;
    CHECK_FALSE(avatar_action::move(you, here, tripoint_rel_ms::north()));
    CHECK(map_local_to_abs(here, you.bub_pos()) == wall_abs);
    CHECK(you.moves == moves_before);
    REQUIRE(avatar_action::move(you, here, tripoint_rel_ms::south()));
    CHECK(map_local_to_abs(here, you.bub_pos()) == inside_abs);

    here.ter_set(abs_to_map_local(here, wall_abs), ter_str_id("t_door_c").id());
    REQUIRE(avatar_action::move(you, here, tripoint_rel_ms::north()));
    REQUIRE(map_local_to_abs(here, you.bub_pos()) == wall_abs);
    REQUIRE(avatar_action::move(you, here, tripoint_rel_ms::north()));
    CHECK(here.ter(abs_to_map_local(here, wall_abs)) == ter_str_id("t_door_o").id());
    CHECK(map_local_to_abs(here, you.bub_pos()) == wall_abs);
    REQUIRE(avatar_action::move(you, here, tripoint_rel_ms::north()));
    CHECK(map_local_to_abs(here, you.bub_pos()) == outside_abs);
}

TEST_CASE("wall_edges_native_ai_and_routes_respect_wall_edges", "[wall_edges][ai]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    move_player_out_of_the_way();
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    auto& here = get_map();
    const auto inside = tripoint_bub_ms(60, 60, 0);
    const auto wall = tripoint_bub_ms(60, 59, 0);
    const auto outside = tripoint_bub_ms(60, 58, 0);
    for (auto y = 58; y <= 62; ++y) {
        for (auto x = 58; x <= 62; ++x) {
            here.ter_set(tripoint_bub_ms(x, y, 0), y == 59 ? t_wall : y >= 60 ? t_floor : t_grass);
        }
    }
    SECTION("monster native movement") {
        auto& mon = spawn_test_monster("debug_mon", inside);
        mon.moves = 1000;
        REQUIRE(mon.can_move_to(wall));
        REQUIRE(mon.move_to(wall));
        REQUIRE(mon.bub_pos() == wall);
        CHECK_FALSE(mon.can_move_to(outside));
        CHECK_FALSE(mon.move_to(outside));
        CHECK(mon.bub_pos() == wall);
    }
    SECTION("NPC native movement") {
        auto& human = spawn_npc(inside, "test_talker");
        human.set_moves(1000);
        REQUIRE(human.can_move_to(wall, true));
        human.move_to(wall, true);
        REQUIRE(human.bub_pos() == wall);
        CHECK_FALSE(human.can_move_to(outside, true));
        human.move_to(outside, true);
        CHECK(human.bub_pos() == wall);
    }
    SECTION("NPC opens a door at the boundary rather than upon entering its floor") {
        here.ter_set(wall, ter_str_id("t_door_c").id());
        auto& human = spawn_npc(inside, "test_talker");
        human.set_moves(1000);
        human.move_to(wall, true);
        REQUIRE(human.bub_pos() == wall);
        REQUIRE(here.ter(wall) == ter_str_id("t_door_c").id());
        human.move_to(outside, true);
        CHECK(here.ter(wall) == ter_str_id("t_door_o").id());
        CHECK(human.bub_pos() == wall);
        human.move_to(outside, true);
        CHECK(human.bub_pos() == outside);
    }
    SECTION("modern routes retain the cost of breaking a crossed wall") {
        Pathfinding::clear_d_maps();
        const auto reset_paths = on_out_of_scope([] { Pathfinding::clear_d_maps(); });
        auto settings = PathfindingSettings{};
        settings.bash_strength_val = 100;
        settings.bash_cost = 1000.0f;
        const auto route = Pathfinding::route(inside, outside, settings, RouteSettings{});
        REQUIRE_FALSE(route.empty());
        CHECK(route.back() == outside);
        auto previous = inside;
        for (const auto& step : route) {
            CHECK_FALSE(wall_edges::trace({.here = here, .from = previous, .to = step}));
            previous = step;
        }
    }
    SECTION("modern route goes around a boundary instead of through it") {
        Pathfinding::clear_d_maps();
        const auto reset_paths = on_out_of_scope([] { Pathfinding::clear_d_maps(); });
        const auto route =
            Pathfinding::route(inside, outside, PathfindingSettings{}, RouteSettings{});
        REQUIRE_FALSE(route.empty());
        CHECK(route.back() == outside);
        auto previous = inside;
        for (const auto& step : route) {
            CHECK_FALSE(wall_edges::trace({.here = here, .from = previous, .to = step}));
            previous = step;
        }
    }
    SECTION("native route goes around a boundary instead of through it") {
        auto settings = pathfinding_settings{};
        settings.max_dist = 20;
        settings.max_length = 100;
        const auto route = here.route(inside, outside, settings, {});
        REQUIRE_FALSE(route.empty());
        CHECK(route.back() == outside);
        auto previous = inside;
        for (const auto& step : route) {
            CHECK_FALSE(wall_edges::trace({.here = here, .from = previous, .to = step}));
            previous = step;
        }
    }
}
TEST_CASE("wall_edges_vehicle_parts_collide_with_the_wall_strip", "[wall_edges][vehicle]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    move_player_out_of_the_way();
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    auto& here = get_map();
    const auto inside = tripoint_bub_ms(60, 60, 0);
    const auto wall = tripoint_bub_ms(60, 59, 0);
    for (auto x = 58; x <= 62; ++x) {
        here.ter_set(tripoint_bub_ms(x, 60, 0), t_floor);
        here.ter_set(tripoint_bub_ms(x, 59, 0), ter_str_id("t_concrete_wall").id());
        here.ter_set(tripoint_bub_ms(x, 58, 0), t_grass);
    }
    auto* veh = here.add_vehicle(
        vproto_id("bicycle_test"), inside + tripoint_rel_ms::south(), 270_degrees, 0, 0);
    REQUIRE(veh != nullptr);
    REQUIRE(veh->bub_part_location(0) == inside);
    veh->velocity = 222;
    const auto probe = veh->part_collision({.part = 0, .pos = wall, .just_detect = true});
    CHECK(probe.type != veh_coll_nothing);
    const auto open_probe = veh->part_collision(
        {.part = 0, .pos = inside + tripoint_rel_ms::south(), .just_detect = true});
    CHECK(open_probe.type == veh_coll_nothing);
    const auto impact = veh->part_collision({.part = 0, .pos = wall});
    CHECK(impact.type != veh_coll_nothing);
    CHECK(std::abs(veh->velocity) < 222);
}

TEST_CASE("wall_edges_protect_against_attacks_across_the_boundary", "[wall_edges][combat]") {
    const auto cleanup = on_out_of_scope([] { clear_all_state(); });
    clear_all_state();
    clear_map();
    const auto renderer = override_option("RENDERER_3D", "true");
    const auto thin = override_option("R3D_THIN_WALLS", "true");
    auto& here = get_map();
    auto& you = get_avatar();
    you.setpos(tripoint_bub_ms(60, 59, 0));
    // setpos follows the production reality-bubble shift before constructing the scene.
    const auto wall = you.bub_pos();
    const auto outside = wall + tripoint_rel_ms::north();
    for (auto dx = -2; dx <= 2; ++dx) {
        const auto p = wall + tripoint_rel_ms(dx, 0, 0);
        here.ter_set(p + tripoint_rel_ms::south(), t_floor);
        here.ter_set(p, ter_str_id("t_concrete_wall").id());
        here.ter_set(p + tripoint_rel_ms::north(), t_grass);
    }
    you.set_moves(1000);
    auto& target = spawn_test_monster("debug_mon", outside);
    REQUIRE(wall_edges::trace({.here = here, .from = wall, .to = outside}));
    SECTION("character melee does not begin through a wall") {
        const auto moves_before = you.moves;
        you.melee_attack(target, false);
        CHECK(you.moves == moves_before);
    }
    SECTION("monster melee does not begin through a wall") { CHECK_FALSE(target.attack_at(wall)); }
    SECTION("a projectile meets the wall before the adjacent target") {
        auto shot = projectile{};
        shot.speed = 1000;
        shot.range = 3;
        shot.impact = damage_instance(DT_BULLET, 1);
        shot.add_effect(ammo_effect_str_id("NO_PENETRATE_OBSTACLES"));
        auto gun = item::spawn(itype_id("glock_19"));
        gun->ammo_set(itype_id("9mm"), 1);
        const auto attack =
            projectile_attack(shot, wall, outside, dispersion_sources{}, &you, gun.get());
        CHECK(attack.hit_critter == nullptr);
        CHECK(attack.end_point != outside);
    }
    SECTION("adjacent creatures cannot see through an opaque edge") {
        CHECK_FALSE(you.sees(target));
        here.ter_set(wall, ter_str_id("t_window_domestic").id());
        CHECK(you.sees(target));
    }
    SECTION("physical reach paths stop at the same wall") {
        CHECK_FALSE(map_funcs::physical_clear_path(
            {.m = here,
             .from = wall,
             .to = outside,
             .range = 1,
             .cost_min = 0,
             .cost_max = 100,
             .require_clear_path = false}));
    }
}
#endif
