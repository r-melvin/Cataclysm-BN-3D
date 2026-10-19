#include "catch/catch.hpp"
#include "r3d_camera.h"
#include "r3d_dda.h"
#include "r3d_first_person_math.h"
#include "r3d_shapes.h"

#include <array>
#include <cmath>

namespace {
auto approx(float a, float b, float eps = 1e-3f) -> bool { return std::fabs(a - b) <= eps; }
auto first_person_camera(int heading) -> r3d::camera {
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::first_person;
    cam.z_scale = 1.8f;
    cam.heading = heading;
    cam.target = r3d::tile_to_world({.x = 0.5f, .y = 0.5f, .z = 0.1f});
    return cam;
}
} // namespace

TEST_CASE("r3d_first_person_eye_stands_on_the_target", "[r3d]") {
    const auto cam = first_person_camera(0);
    const auto eye = cam.eye();
    CHECK(approx(eye.x, 0.5f));
    CHECK(approx(eye.y, -0.5f));
    // The target is the floor, in world levels; the eye is 0.8 above it in stretched space.
    CHECK(approx(eye.z, 0.1f * 1.8f + 0.8f));
}

TEST_CASE("r3d_first_person_forward_eighths_is_the_heading", "[r3d]") {
    for (auto h = 0; h < 8; ++h) { CHECK(first_person_camera(h).forward_eighths() == h); }
    auto cam = first_person_camera(7);
    cam.rotate_steps(1);
    CHECK(cam.heading == 0);
    cam.rotate_steps(-1);
    CHECK(cam.heading == 7);
    cam.rotate_steps(-9);
    CHECK(cam.heading == 6);
    // Turning never touches the orbit angles.
    CHECK(approx(cam.yaw, 0.0f));
}

TEST_CASE("r3d_first_person_centre_ray_points_along_the_heading", "[r3d]") {
    for (auto h = 0; h < 8; ++h) {
        const auto cam = first_person_camera(h);
        const auto ray = cam.screen_ray(0.0f, 0.0f, 16.0f / 9.0f);
        // World space: heading 0 is +y (north), 2 is +x (east).
        const auto angle = static_cast<float>(h) * 0.78539816f;
        CHECK(approx(ray.dir.x, std::sin(angle), 1e-3f));
        CHECK(approx(ray.dir.y, std::cos(angle), 1e-3f));
        CHECK(approx(ray.dir.z, 0.0f, 1e-3f));
        // The ray starts at the eye (the near plane is a few centimetres in front of it). Rays are
        // in unstretched world space, eye() in stretched space.
        const auto eye = cam.eye();
        CHECK(r3d::length(ray.origin - r3d::vec3{eye.x, eye.y, eye.z / cam.z_scale}) < 0.1f);
    }
}

TEST_CASE("r3d_first_person_right_is_clockwise_of_forward", "[r3d]") {
    for (auto h = 0; h < 8; ++h) {
        const auto cam = first_person_camera(h);
        const auto right = cam.ground_right();
        const auto fwd = cam.fp_forward();
        // forward is counter-clockwise of right seen from above (z up), so the cross product points
        // up.
        CHECK(approx(right.x * fwd.y - right.y * fwd.x, 1.0f, 1e-3f));
        // A point to the right projects to the right half of the screen.
        const auto point = cam.eye() + cam.fp_forward() * 5.0f + right * 2.0f;
        const auto ndc = r3d::transform_point(cam.view_projection(1.5f), point);
        CHECK(ndc.x > 0.0f);
    }
}

TEST_CASE("r3d_first_person_frustum_sees_ahead_not_behind", "[r3d]") {
    const auto cam = first_person_camera(2); // facing east
    const auto f = cam.view_frustum(1.5f);
    const auto stretched_eye = cam.eye();
    const auto eye = r3d::vec3{stretched_eye.x, stretched_eye.y, stretched_eye.z / cam.z_scale};
    const auto box_at = [&eye](float dx, float dy) {
        return r3d::aabb{{eye.x + dx - 0.4f, eye.y + dy - 0.4f, eye.z - 0.4f},
                         {eye.x + dx + 0.4f, eye.y + dy + 0.4f, eye.z + 0.4f}};
    };
    CHECK(f.intersects(box_at(3.0f, 0.0f)));
    CHECK_FALSE(f.intersects(box_at(-3.0f, 0.0f)));
    // North and south of an east-facing viewer are outside a 90 degree field of view.
    CHECK_FALSE(f.intersects(box_at(0.0f, 10.0f)));
    CHECK_FALSE(f.intersects(box_at(0.0f, -10.0f)));
    // The wall of the neighbouring tile, half a tile away, is not cut off by the near plane.
    CHECK(f.intersects(box_at(0.9f, 0.0f)));
}

TEST_CASE("r3d_first_person_projection_keeps_the_horizontal_field_of_view", "[r3d]") {
    auto cam = first_person_camera(0);
    cam.fovx = 1.5708f;
    for (float aspect : {1.0f, 1.6f, 2.4f}) {
        // A point 5 ahead and 5 to the right sits at the right edge of a 90 degree view.
        const auto point = cam.eye() + r3d::vec3{5.0f, 5.0f, 0.0f};
        const auto ndc = r3d::transform_point(cam.view_projection(aspect), point);
        CHECK(approx(ndc.x, 1.0f, 1e-2f));
        CHECK(approx(cam.fp_fovx(aspect), 1.5708f));
    }
    // A tall, narrow picture keeps 90 degrees up and down instead (5 ahead, 5 up: the top
    // edge; z is stretched by z_scale in the view), and is narrower across.
    for (float aspect : {0.5f, 0.75f}) {
        const auto point = cam.eye() + r3d::vec3{0.0f, 5.0f, 5.0f};
        const auto world = r3d::vec3{point.x, point.y, point.z / cam.z_scale};
        const auto ndc = r3d::transform_point(cam.view_projection(aspect), world);
        CHECK(approx(ndc.y, 1.0f, 1e-2f));
        CHECK(approx(cam.fp_fovy(aspect), 1.5708f));
        CHECK(cam.fp_fovx(aspect) < 1.5708f);
        const auto side =
            cam.eye() + r3d::vec3{5.0f * std::tan(cam.fp_fovx(aspect) * 0.5f), 5.0f, 0.0f};
        const auto side_world = r3d::vec3{side.x, side.y, side.z / cam.z_scale};
        CHECK(approx(r3d::transform_point(cam.view_projection(aspect), side_world).x, 1.0f, 1e-2f));
    }
    // The orbit camera is untouched by first-person fields.
    auto orbit = r3d::camera{};
    CHECK(orbit.mode == r3d::camera_mode::orbit);
    CHECK(orbit.forward_eighths() == 0);
}

TEST_CASE("r3d_first_person_look_pitch_is_clamped", "[r3d]") {
    auto cam = first_person_camera(0);
    cam.pitch_steps(50);
    CHECK(approx(cam.look_pitch, r3d::camera::max_look_pitch));
    CHECK(cam.fp_forward().z > 0.9f);
    cam.pitch_steps(-100);
    CHECK(approx(cam.look_pitch, -r3d::camera::max_look_pitch));
    const auto distance = cam.distance;
    cam.zoom_steps(5);
    CHECK(approx(cam.distance, distance));
}

TEST_CASE("r3d_first_person_pick_finds_the_tile_ahead", "[r3d]") {
    // Open floor of height 0.1 around tile (0, 0); the centre ray looking along the heading
    // lands on a floor tile ahead of the viewer.
    const auto floor_world = [](int /*x*/, int /*y*/, int z) {
        return z == 0 ? r3d::floor_height : 0.0f;
    };
    // Looking down 45 degrees from an eye about 1 above the floor lands about 1 tile ahead.
    for (auto h = 0; h < 8; ++h) {
        auto cam = first_person_camera(h);
        cam.look_pitch = -0.7854f;
        const auto hit = r3d::pick_cell(
            {.cam = cam,
             .ndc_x = 0.0f,
             .ndc_y = 0.0f,
             .aspect = 1.5f,
             .max_t = 400.0f,
             .height = floor_world,
             .floor_z = r3d::floor_height});
        REQUIRE(hit.hit);
        const auto angle = static_cast<float>(h) * 0.78539816f;
        // Tile space: x east, y south.
        const auto dx = static_cast<int>(std::lround(std::sin(angle)));
        const auto dy = static_cast<int>(std::lround(-std::cos(angle)));
        CHECK(hit.x == dx);
        CHECK(hit.y == dy);
    }
}

TEST_CASE("r3d_eighths_towards_names_the_nearest_heading", "[r3d]") {
    // Map convention: x east, y south; heading 0 is north, 2 east, 4 south, 6 west.
    CHECK(r3d::eighths_towards(0, -3) == 0);
    CHECK(r3d::eighths_towards(2, -2) == 1);
    CHECK(r3d::eighths_towards(5, 0) == 2);
    CHECK(r3d::eighths_towards(1, 1) == 3);
    CHECK(r3d::eighths_towards(0, 4) == 4);
    CHECK(r3d::eighths_towards(-1, 1) == 5);
    CHECK(r3d::eighths_towards(-7, 0) == 6);
    CHECK(r3d::eighths_towards(-1, -1) == 7);
    // Off-axis offsets round to the closer of the two neighbouring headings.
    CHECK(r3d::eighths_towards(1, -5) == 0);
    CHECK(r3d::eighths_towards(5, -1) == 2);
    CHECK(r3d::eighths_towards(0, 0) == 0);
}

TEST_CASE("r3d_visible_faces_adds_a_ceiling_under_a_solid_level_above", "[r3d]") {
    auto floor = r3d::shape{};
    floor.kind = r3d::shape_kind::floor;
    floor.height = r3d::floor_height;
    auto block = r3d::shape{};
    block.kind = r3d::shape_kind::block;
    block.height = 1.0f;
    auto prism = r3d::shape{};
    prism.kind = r3d::shape_kind::prism;
    prism.height = 0.5f;
    auto nb = r3d::neighbourhood{};
    // Open air above: sky, no ceiling.
    CHECK((r3d::visible_faces(floor, nb) & r3d::face_ceiling) == 0);
    // A floor above (a room under a roof or a second storey): ceiling.
    nb.up = floor;
    CHECK((r3d::visible_faces(floor, nb) & r3d::face_ceiling) != 0);
    CHECK((r3d::visible_faces(floor, nb) & r3d::face_top) != 0);
    CHECK((r3d::visible_faces(prism, nb) & r3d::face_ceiling) != 0);
    // A thin wall occupies only part of its cell; the roof closes the rest.
    CHECK((r3d::visible_faces(block, nb) & r3d::face_ceiling) != 0);
    // A wall above closes the room as well.
    nb.up = block;
    CHECK((r3d::visible_faces(floor, nb) & r3d::face_ceiling) != 0);
}

TEST_CASE("r3d_minimap_turns_the_heading_up", "[r3d]") {
    // Screen space: x right, y down; the player sits at (100, 100). A tile one step from them
    // in the direction they face must end up straight above them, whatever the heading.
    const auto cx = 100.0f;
    const auto cy = 100.0f;
    for (auto h = 0; h < 8; ++h) {
        const auto angle = static_cast<float>(h) * 0.78539816f;
        // North-up minimap: map x east is screen right, map y south is screen down.
        auto x = cx + 10.0f * std::sin(angle);
        auto y = cy - 10.0f * std::cos(angle);
        r3d::turn_on_screen({.x = x, .y = y, .cx = cx, .cy = cy, .radians = r3d::minimap_turn(h)});
        CHECK(approx(x, cx));
        CHECK(approx(y, cy - 10.0f));
    }
    // Facing east: north swings to the left of the screen, south to the right.
    auto nx = cx;
    auto ny = cy - 10.0f;
    r3d::turn_on_screen({.x = nx, .y = ny, .cx = cx, .cy = cy, .radians = r3d::minimap_turn(2)});
    CHECK(approx(nx, cx - 10.0f));
    CHECK(approx(ny, cy));
    // Heading north is no turn at all, and the player stays where they are.
    CHECK(approx(r3d::minimap_turn(0), 0.0f));
    auto px = cx;
    auto py = cy;
    r3d::turn_on_screen({.x = px, .y = py, .cx = cx, .cy = cy, .radians = r3d::minimap_turn(3)});
    CHECK(approx(px, cx));
    CHECK(approx(py, cy));
}

TEST_CASE("r3d_map_bearing_is_clockwise_from_north_with_y_south", "[r3d]") {
    const auto pi = 3.14159265f;
    // The map's y axis points south, so north is dy < 0.
    CHECK(approx(r3d::signed_map_bearing(0.0f, -1.0f), 0.0f));
    CHECK(approx(r3d::signed_map_bearing(1.0f, -1.0f), pi * 0.25f));
    CHECK(approx(r3d::signed_map_bearing(1.0f, 0.0f), pi * 0.5f));
    CHECK(approx(r3d::signed_map_bearing(1.0f, 1.0f), pi * 0.75f));
    CHECK(approx(std::fabs(r3d::signed_map_bearing(0.0f, 1.0f)), pi));
    CHECK(approx(r3d::signed_map_bearing(-1.0f, 1.0f), -pi * 0.75f));
    CHECK(approx(r3d::signed_map_bearing(-1.0f, 0.0f), -pi * 0.5f));
    CHECK(approx(r3d::signed_map_bearing(-1.0f, -1.0f), -pi * 0.25f));
    // The bearing of each heading matches the bearing of the step that heading walks, and the
    // first-person camera's view yaw for it (what the compass strip measures against).
    const auto steps = std::array{
        std::array{0, -1}, std::array{1, -1}, std::array{1, 0},  std::array{1, 1},
        std::array{0, 1},  std::array{-1, 1}, std::array{-1, 0}, std::array{-1, -1}};
    for (auto h = 0; h < 8; ++h) {
        const auto step = r3d::
            signed_map_bearing(static_cast<float>(steps[h][0]), static_cast<float>(steps[h][1]));
        CHECK(approx(r3d::wrap_bearing(r3d::heading_bearing(h) - step), 0.0f));
        CHECK(r3d::eighths_towards(steps[h][0], steps[h][1]) == h);
        auto cam = r3d::camera{};
        cam.mode = r3d::camera_mode::first_person;
        cam.heading = h;
        CHECK(approx(r3d::wrap_bearing(cam.fp_yaw() - r3d::heading_bearing(h)), 0.0f));
        CHECK(cam.forward_eighths() == h);
    }
}

TEST_CASE("r3d_relative_bearing_puts_left_negative_and_wraps", "[r3d]") {
    const auto pi = 3.14159265f;
    for (auto h = 0; h < 8; ++h) {
        const auto yaw = r3d::heading_bearing(h);
        // Straight ahead of each heading, in map space (y south).
        const auto dx = std::sin(yaw);
        const auto dy = -std::cos(yaw);
        CHECK(approx(r3d::relative_bearing(dx, dy, yaw), 0.0f));
        // The offset turned a quarter clockwise on the map is at the right hand, and the other
        // way round at the left hand.
        CHECK(approx(r3d::relative_bearing(-dy, dx, yaw), pi * 0.5f));
        CHECK(approx(r3d::relative_bearing(dy, -dx, yaw), -pi * 0.5f));
        // Behind is +-pi, never outside that range.
        CHECK(approx(std::fabs(r3d::relative_bearing(-dx, -dy, yaw)), pi));
    }
    // Facing east (heading 2): something north is on the left, south on the right.
    const auto east = r3d::heading_bearing(2);
    CHECK(r3d::relative_bearing(0.0f, -3.0f, east) < 0.0f);
    CHECK(r3d::relative_bearing(0.0f, 3.0f, east) > 0.0f);
    // Facing north-west (heading 7): something due west is half a quadrant to the left.
    CHECK(approx(r3d::relative_bearing(-2.0f, 0.0f, r3d::heading_bearing(7)), -pi * 0.25f));
    for (auto a = -10.0f; a < 10.0f; a += 0.37f) {
        const auto w = r3d::wrap_bearing(a);
        CHECK(w > -pi - 1e-4f);
        CHECK(w <= pi + 1e-4f);
        CHECK(approx(std::cos(w), std::cos(a)));
        CHECK(approx(std::sin(w), std::sin(a)));
    }
}

TEST_CASE("r3d_compass_strip_places_bearings", "[r3d]") {
    const auto pi = 3.14159265f;
    // A 200 pixel strip showing half a turn: ahead in the middle, the hands at the ends.
    CHECK(approx(r3d::compass_offset(0.0f, 200.0f, pi), 0.0f));
    CHECK(approx(r3d::compass_offset(pi * 0.5f, 200.0f, pi), 100.0f));
    CHECK(approx(r3d::compass_offset(-pi * 0.5f, 200.0f, pi), -100.0f));
    CHECK(approx(r3d::compass_offset(pi * 0.25f, 200.0f, pi), 50.0f));
    // Behind: pinned to the nearer end.
    CHECK(approx(r3d::compass_offset(pi * 0.9f, 200.0f, pi), 100.0f));
    CHECK(approx(r3d::compass_offset(-pi * 0.9f, 200.0f, pi), -100.0f));
    CHECK(r3d::on_compass(pi * 0.5f, pi));
    CHECK_FALSE(r3d::on_compass(pi * 0.6f, pi));
    // The compass letters: facing east, N is at the left end and S at the right end.
    const auto east = r3d::heading_bearing(2);
    CHECK(approx(r3d::compass_offset(r3d::wrap_bearing(r3d::heading_bearing(0) - east), 200.0f, pi),
                 -100.0f));
    CHECK(approx(r3d::compass_offset(r3d::wrap_bearing(r3d::heading_bearing(4) - east), 200.0f, pi),
                 100.0f));
    CHECK_FALSE(r3d::on_compass(r3d::wrap_bearing(r3d::heading_bearing(6) - east), pi));
}

TEST_CASE("r3d_edge_alert_sectors", "[r3d]") {
    const auto pi = 3.14159265f;
    const auto fovx = pi * 0.5f; // 90 degrees
    using r3d::edge_side;
    // The eight neighbours of the player, indexed like the headings.
    const auto steps = std::array{
        std::array{0, -1}, std::array{1, -1}, std::array{1, 0},  std::array{1, 1},
        std::array{0, 1},  std::array{-1, 1}, std::array{-1, 0}, std::array{-1, -1}};
    // By how many eighths clockwise the neighbour is from the heading -> the edge that lights.
    // The diagonals ahead are exactly on the picture's edge at 90 degrees, so they count as in it.
    const edge_side expected[8] =
        {edge_side::none,   edge_side::none, edge_side::right, edge_side::right,
         edge_side::behind, edge_side::left, edge_side::left,  edge_side::none};
    for (auto h = 0; h < 8; ++h) {
        const auto yaw = r3d::heading_bearing(h);
        for (auto n = 0; n < 8; ++n) {
            const auto rel = r3d::relative_bearing(
                static_cast<float>(steps[n][0]), static_cast<float>(steps[n][1]), yaw);
            const auto off = ((n - h) % 8 + 8) % 8;
            INFO("heading " << h << " neighbour " << n);
            CHECK(r3d::edge_alert_side(rel, fovx) == expected[off]);
        }
    }
    // A wider view takes the side neighbours (90 degrees off) into the picture.
    CHECK(r3d::edge_alert_side(pi * 0.5f, pi * 0.9f) == edge_side::right);
    CHECK(r3d::edge_alert_side(-pi * 0.5f, pi * 1.1f) == edge_side::none);
}

TEST_CASE("r3d_ceilings_close_everything_but_open_air", "[r3d]") {
    auto floor = r3d::shape{};
    floor.kind = r3d::shape_kind::floor;
    floor.height = r3d::floor_height;
    auto pane = r3d::shape{};
    pane.kind = r3d::shape_kind::pane;
    pane.height = 1.0f;
    auto table = r3d::shape{};
    table.kind = r3d::shape_kind::prism;
    table.height = 0.4f;
    table.slab = 0.08f;
    auto upright = r3d::shape{};
    upright.kind = r3d::shape_kind::upright;
    upright.height = 1.4f;
    auto nb = r3d::neighbourhood{};
    // A window or door in a wall line gets the room's ceiling under a floor above.
    nb.up = floor;
    CHECK((r3d::visible_faces(pane, nb) & r3d::face_ceiling) != 0);
    // It has no other faces: the mesher draws the frame itself.
    CHECK((r3d::visible_faces(pane, nb) & ~r3d::face_ceiling) == 0);
    // A window pane, a table or a shrub upstairs leaves no hole in the ceiling below.
    for (const r3d::shape& up : {pane, table, upright}) {
        nb.up = up;
        CHECK((r3d::visible_faces(floor, nb) & r3d::face_ceiling) != 0);
    }
    // Trees reach into the level above: never a ceiling over one.
    nb.up = floor;
    CHECK((r3d::visible_faces(upright, nb) & r3d::face_ceiling) == 0);
    // Open air above a window: sky.
    nb.up = r3d::shape();
    CHECK((r3d::visible_faces(pane, nb) & r3d::face_ceiling) == 0);
}

TEST_CASE("r3d_first_person_level_tilt_adds_to_the_look_pitch", "[r3d]") {
    auto cam = r3d::camera{};
    cam.mode = r3d::camera_mode::first_person;
    cam.heading = 2;
    // Looking one level up tilts the view 45 degrees up, still facing the heading.
    cam.tilt_pitch = 0.7854f;
    const auto up = cam.fp_forward();
    CHECK(approx(up.z, std::sin(0.7854f)));
    CHECK(up.x > 0.0f);
    CHECK(approx(up.y, 0.0f));
    // Together with a look up it stays within the clamp.
    cam.look_pitch = 0.7854f;
    CHECK(approx(cam.fp_forward().z, std::sin(r3d::camera::max_look_pitch)));
    // Looking a level down tilts the view down; walking directions keep the heading.
    cam.look_pitch = 0.0f;
    cam.tilt_pitch = -0.7854f;
    CHECK(cam.fp_forward().z < 0.0f);
    CHECK(cam.forward_eighths() == 2);
}

TEST_CASE("r3d_minimap_disc_stays_whole_when_turned", "[r3d]") {
    // The heading-up minimap keeps only cells within a disc around the player that fits in the
    // drawn square; turning keeps distances, so every kept cell stays inside the square at every
    // heading and the square is never left with an empty corner inside the disc.
    const auto cx = 50.0f;
    const auto cy = 60.0f;
    const auto radius = 40.0f;
    for (auto h = 0; h < 8; ++h) {
        for (auto a = 0.0f; a < 6.28f; a += 0.3f) {
            for (auto d = 0.0f; d <= radius; d += 7.5f) {
                auto x = cx + d * std::cos(a);
                auto y = cy + d * std::sin(a);
                r3d::turn_on_screen(
                    {.x = x, .y = y, .cx = cx, .cy = cy, .radians = r3d::minimap_turn(h)});
                CHECK(approx(std::hypot(x - cx, y - cy), d));
                CHECK(std::fabs(x - cx) <= radius + 1e-3f);
                CHECK(std::fabs(y - cy) <= radius + 1e-3f);
            }
        }
    }
}

TEST_CASE("r3d_clear_line_is_blocked_by_walls_but_not_by_the_target", "[r3d]") {
    // Floors everywhere on level 0, a wall at (3, 0), a counter at (5, 2).
    const auto world = [](int x, int y, int z) {
        if (z != 0) { return 0.0f; }
        if (x == 3 && y == 0) { return 1.0f; }
        if (x == 5 && y == 2) { return 0.4f; }
        return r3d::floor_height;
    };
    const auto eye = r3d::vec3{0.5f, 0.5f, 0.55f};
    // Behind the wall: hidden. In front of it: seen.
    CHECK_FALSE(r3d::clear_line(eye, {6.5f, 0.5f, 0.5f}, world));
    CHECK(r3d::clear_line(eye, {2.5f, 0.5f, 0.5f}, world));
    // Off to the side of the wall: seen.
    CHECK(r3d::clear_line(eye, {4.5f, 3.5f, 0.5f}, world));
    // Standing on the counter: its own column does not hide it.
    CHECK(r3d::clear_line(eye, {5.5f, 2.5f, 0.6f}, world));
    // The same point as the eye is trivially seen.
    CHECK(r3d::clear_line(eye, eye, world));
}

TEST_CASE("r3d_sprites_slide_away_from_the_eye", "[r3d]") {
    // Eye at (0, 0) looking +x.
    // A sprite 0.2 ahead slides to 0.35.
    CHECK(approx(
        r3d::slide_from_eye(
            {.x = 0.2f,
             .y = 0.0f,
             .ex = 0.0f,
             .ey = 0.0f,
             .fx = 1.0f,
             .fy = 0.0f,
             .min_depth = 0.35f}),
        0.15f));
    // Off to the side, its depth along the view counts, not its distance.
    CHECK(approx(
        r3d::slide_from_eye(
            {.x = 0.1f,
             .y = 0.4f,
             .ex = 0.0f,
             .ey = 0.0f,
             .fx = 1.0f,
             .fy = 0.0f,
             .min_depth = 0.35f}),
        0.25f));
    // Far enough already, or behind the eye: left alone.
    CHECK(approx(
        r3d::slide_from_eye(
            {.x = 1.0f,
             .y = 0.0f,
             .ex = 0.0f,
             .ey = 0.0f,
             .fx = 1.0f,
             .fy = 0.0f,
             .min_depth = 0.35f}),
        0.0f));
    CHECK(approx(
        r3d::slide_from_eye(
            {.x = -0.2f,
             .y = 0.0f,
             .ex = 0.0f,
             .ey = 0.0f,
             .fx = 1.0f,
             .fy = 0.0f,
             .min_depth = 0.35f}),
        0.0f));
    // A neighbour seen from the tile centre (a whole tile away) never slides, at any heading.
    for (auto h = 0; h < 8; ++h) {
        const auto a = static_cast<float>(h) * 0.78539816f;
        const auto fx = std::sin(a);
        const auto fy = -std::cos(a);
        CHECK(approx(
            r3d::slide_from_eye(
                {.x = 0.5f + fx * 1.0f,
                 .y = 0.5f + fy * 1.0f,
                 .ex = 0.5f,
                 .ey = 0.5f,
                 .fx = fx,
                 .fy = fy,
                 .min_depth = 0.35f}),
            0.0f));
    }
    // An eye off its tile centre and close to a sprite (any frame): it slides.
    CHECK(
        r3d::slide_from_eye(
            {.x = 1.5f,
             .y = 0.5f,
             .ex = 1.3f,
             .ey = 0.5f,
             .fx = 1.0f,
             .fy = 0.0f,
             .min_depth = 0.35f})
        > 0.0f);
}

TEST_CASE("r3d_roof_closes_the_space_over_thin_perimeter_walls", "[r3d]") {
    auto wall = r3d::shape{.kind = r3d::shape_kind::block, .height = 1.0f};
    auto neighbours = r3d::neighbourhood{};
    neighbours.up = r3d::shape{.kind = r3d::shape_kind::floor, .height = r3d::floor_height};
    CHECK((r3d::visible_faces(wall, neighbours) & r3d::face_ceiling) != 0);
    neighbours.up = r3d::shape{};
    CHECK((r3d::visible_faces(wall, neighbours) & r3d::face_ceiling) == 0);
}
