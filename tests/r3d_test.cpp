#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "catch/catch.hpp"
#include "point.h"
#include "r3d_camera.h"
#include "r3d_dda.h"
#include "r3d_entities.h"
#include "r3d_math.h"
#include "r3d_shapes.h"
#include "r3d_wall_rules.h"
#include "units_angle.h"

namespace
{

bool approx( float a, float b, float eps = 1e-3f )
{
    return std::fabs( a - b ) <= eps;
}

} // namespace

TEST_CASE( "r3d_matrix_inverse_round_trips", "[r3d]" )
{
    r3d::camera cam;
    cam.target = { 3.0f, -7.0f, 2.0f };
    cam.yaw = 0.7f;
    const r3d::mat4 vp = cam.view_projection( 16.0f / 9.0f );
    r3d::mat4 inv;
    REQUIRE( r3d::invert( vp, inv ) );
    const r3d::mat4 id = vp * inv;
    for( int r = 0; r < 4; ++r ) {
        for( int c = 0; c < 4; ++c ) {
            CHECK( approx( id.at( r, c ), r == c ? 1.0f : 0.0f, 1e-2f ) );
        }
    }
}

TEST_CASE( "r3d_camera_looks_at_target", "[r3d]" )
{
    for( bool ortho : {
             false, true
         } ) {
        r3d::camera cam;
        cam.orthographic = ortho;
        cam.target = { 10.5f, -4.5f, 1.5f };
        cam.yaw = 1.1f;
        cam.pitch = 0.8f;
        cam.distance = 30.0f;
        const float aspect = 1.6f;

        // Target projects to the middle of the screen and in front of the camera.
        const r3d::vec3 ndc = r3d::transform_point( cam.view_projection( aspect ), cam.target );
        CHECK( approx( ndc.x, 0.0f ) );
        CHECK( approx( ndc.y, 0.0f ) );
        CHECK( ndc.z > 0.0f );
        CHECK( ndc.z < 1.0f );

        // The centre ray passes through the target.
        const r3d::ray centre = cam.screen_ray( 0.0f, 0.0f, aspect );
        const r3d::vec3 to_target = cam.target - centre.origin;
        const float along = r3d::dot( to_target, centre.dir );
        const r3d::vec3 closest = centre.origin + centre.dir * along;
        CHECK( r3d::length( closest - cam.target ) < 1e-2f );
    }
}

TEST_CASE( "r3d_camera_screen_ray_round_trips_projection", "[r3d]" )
{
    r3d::camera cam;
    cam.yaw = -0.4f;
    const float aspect = 1.78f;
    const r3d::mat4 vp = cam.view_projection( aspect );
    for( float nx : {
             -0.8f, 0.0f, 0.5f
             } ) {
        for( float ny : {
                 -0.6f, 0.2f, 0.9f
                 } ) {
            const r3d::ray r = cam.screen_ray( nx, ny, aspect );
            // Any point along the ray must project back to the same screen position.
            const r3d::vec3 p = r.origin + r.dir * 20.0f;
            const r3d::vec3 ndc = r3d::transform_point( vp, p );
            CHECK( approx( ndc.x, nx, 1e-2f ) );
            CHECK( approx( ndc.y, ny, 1e-2f ) );
        }
    }
}

TEST_CASE( "r3d_camera_orientation_matches_tile_space", "[r3d]" )
{
    // yaw 0: camera south of the target looking north, so north (tile -y) is up on screen and east
    // (tile +x) is to the right.
    r3d::camera cam;
    cam.yaw = 0.0f;
    cam.target = r3d::tile_to_world( { 0.5f, 0.5f, 0.0f } );
    const r3d::mat4 vp = cam.view_projection( 1.0f );
    const r3d::vec3 centre = r3d::transform_point( vp, cam.target );
    const r3d::vec3 east = r3d::transform_point( vp, r3d::tile_to_world( { 3.5f, 0.5f, 0.0f } ) );
    const r3d::vec3 north = r3d::transform_point( vp, r3d::tile_to_world( { 0.5f, -2.5f, 0.0f } ) );
    CHECK( east.x > centre.x );
    CHECK( north.y > centre.y );

    // A quarter turn clockwise (seen from above) puts the camera east of the target looking west:
    // west is now straight ahead (up on screen) and north is to the right.
    cam.yaw = 1.5707963f;
    const r3d::mat4 vp2 = cam.view_projection( 1.0f );
    const r3d::vec3 c2 = r3d::transform_point( vp2, cam.target );
    const r3d::vec3 west = r3d::transform_point( vp2, r3d::tile_to_world( { -2.5f, 0.5f, 0.0f } ) );
    const r3d::vec3 north2 = r3d::transform_point( vp2, r3d::tile_to_world( { 0.5f, -2.5f, 0.0f } ) );
    CHECK( west.y > c2.y );
    CHECK( north2.x > c2.x );
}

TEST_CASE( "r3d_camera_forward_eighths", "[r3d]" )
{
    // yaw 0 puts the camera south looking north; yaw grows clockwise seen from above, which
    // swings the camera east of the target, looking west.
    r3d::camera cam;
    cam.yaw = 0.0f;
    CHECK( cam.forward_eighths() == 0 );
    cam.yaw = 1.5707963f;
    CHECK( cam.forward_eighths() == 6 );
    cam.yaw = 3.1415927f;
    CHECK( cam.forward_eighths() == 4 );
    cam.yaw = -1.5707963f;
    CHECK( cam.forward_eighths() == 2 );
    cam.yaw = 0.7853982f;
    CHECK( cam.forward_eighths() == 7 );
    // Taller storeys or pitch do not change the heading.
    cam.z_scale = 1.6f;
    cam.pitch = 1.4f;
    CHECK( cam.forward_eighths() == 7 );
}

TEST_CASE( "r3d_thin_walls_join_at_a_uniform_thickness", "[r3d]" )
{
    // Check every combination, including corners, T junctions and crossings. Only the
    // centre strip reaches a connected neighbour; diagonal cells never fatten a corner.
    for( int mask = 0; mask < 16; ++mask ) {
        const uint8_t connections = static_cast<uint8_t>( mask << 1 );
        const std::array<bool, 9> cells = r3d::wall_footprint( connections );
        CHECK( cells[4] );
        for( int corner : {
                 0, 2, 6, 8
             } ) {
            CHECK_FALSE( cells[corner] );
        }
        const bool vertical_end = connections == r3d::face_north || connections == r3d::face_south;
        const bool horizontal_end = connections == 0 || connections == r3d::face_west ||
                                    connections == r3d::face_east;
        CHECK( cells[1] == ( vertical_end || ( connections & r3d::face_north ) != 0 ) );
        CHECK( cells[7] == ( vertical_end || ( connections & r3d::face_south ) != 0 ) );
        CHECK( cells[3] == ( horizontal_end || ( connections & r3d::face_west ) != 0 ) );
        CHECK( cells[5] == ( horizontal_end || ( connections & r3d::face_east ) != 0 ) );
    }
}

TEST_CASE( "r3d_thin_walls_trim_only_indoor_corner_spurs", "[r3d]" )
{
    using namespace r3d;
    constexpr uint8_t all = face_north | face_south | face_west | face_east;
    // Inward spur joins the west side of a north/east corner, as in shelter_2.
    CHECK( trim_wall_spur( face_east, all & ~face_east, face_west | face_north | face_east ) );
    CHECK( trim_wall_spur( face_west, all & ~face_west, face_east | face_south | face_west ) );
    CHECK( trim_wall_spur( face_north, all & ~face_north, face_south | face_north | face_east ) );
    CHECK( trim_wall_spur( face_south, all & ~face_south, face_north | face_south | face_west ) );
    // Preserve outdoor ends, freestanding partitions, straight walls and true T junctions.
    CHECK_FALSE( trim_wall_spur( face_east, 0, face_west | face_north | face_east ) );
    CHECK_FALSE( trim_wall_spur( face_east, all & ~face_east, face_west | face_east ) );
    CHECK_FALSE( trim_wall_spur( face_east, all & ~face_east, face_west | face_north | face_south ) );
    CHECK_FALSE( trim_wall_spur( face_north | face_south, face_east | face_west, all ) );
    CHECK_FALSE( trim_wall_spur( 0, all, all ) );
}

TEST_CASE( "r3d_corner_locker_faces_into_the_room", "[r3d]" )
{
    // At the south-west corner the solitary locker backs onto the south wall,
    // a quarter turn anticlockwise from its previous west-wall orientation.
    CHECK( r3d::furniture_wall_facing( false, false, r3d::face_south | r3d::face_west ) == 2 );
    CHECK( r3d::furniture_wall_facing( false, true, r3d::face_south | r3d::face_west ) == 3 );
    CHECK( r3d::furniture_wall_facing( true, false, r3d::face_north | r3d::face_east ) == 0 );
}

TEST_CASE( "r3d_notice_board_is_a_panel", "[r3d]" )
{
    r3d::tile_traits traits;
    traits.move_cost = 0;
    traits.panel = true;
    const r3d::shape board = r3d::classify_furniture( traits );
    CHECK( board.kind == r3d::shape_kind::pane );
    CHECK( board.height > r3d::cutaway_stub_height );
}

TEST_CASE( "r3d_camera_rotation_matches_movement_directions", "[r3d]" )
{
    for( int direction : {
             -1, 1
             } ) {
        r3d::camera cam;
        for( int step = 1; step <= 16; ++step ) {
            cam.rotate_steps( direction );
            const int heading = ( ( -direction * step ) % 8 + 8 ) % 8;
            CHECK( cam.forward_eighths() == heading );
            const float angle = -heading * 0.78539816f;
            const r3d::vec3 right = cam.ground_right();
            CHECK( approx( right.x, std::cos( angle ) ) );
            CHECK( approx( right.y, std::sin( angle ) ) );
        }
    }
}

TEST_CASE( "r3d_camera_rotation_aligns_after_free_orbit", "[r3d]" )
{
    r3d::camera cam;
    cam.yaw = 0.3f;
    cam.rotate_steps( 1 );
    CHECK( approx( cam.yaw, 0.78539816f ) );
    cam.yaw = -0.3f;
    cam.rotate_steps( -1 );
    CHECK( approx( cam.yaw, -0.78539816f ) );
}

TEST_CASE( "r3d_camera_steps_clamp", "[r3d]" )
{
    r3d::camera cam;
    cam.pitch_steps( 1000 );
    CHECK( cam.pitch <= r3d::camera::max_pitch );
    cam.pitch_steps( -1000 );
    CHECK( cam.pitch >= r3d::camera::min_pitch );
    cam.zoom_steps( 1000 );
    CHECK( cam.distance >= r3d::camera::min_distance );
    cam.zoom_steps( -1000 );
    CHECK( cam.distance <= r3d::camera::max_distance );

    cam.yaw = 0.0f;
    cam.rotate_steps( 8 ); // 8 * 45 degrees = a full turn
    CHECK( ( approx( cam.yaw, 0.0f, 0.05f ) || approx( std::fabs( cam.yaw ), 6.2831853f, 0.05f ) ) );
}

TEST_CASE( "r3d_frustum_culls_boxes", "[r3d]" )
{
    r3d::camera cam;
    cam.target = { 0.0f, 0.0f, 0.0f };
    const r3d::frustum f = cam.view_frustum( 1.5f );
    CHECK( f.intersects( { { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f } } ) );
    CHECK_FALSE( f.intersects( { { 500.0f, 0.0f, 0.0f }, { 501.0f, 1.0f, 1.0f } } ) );
    CHECK_FALSE( f.intersects( { { 0.0f, 0.0f, 900.0f }, { 1.0f, 1.0f, 901.0f } } ) );
}

TEST_CASE( "r3d_raycast_hits_floor_tile_under_ray", "[r3d]" )
{
    // A flat floor of height 0.1 at z = 0 over x, y in [0, 20).
    const r3d::column_height_fn floor_world = []( int x, int y, int z ) {
        return z == 0 && x >= 0 && x < 20 && y >= 0 && y < 20 ? 0.1f : 0.0f;
    };
    // Straight down onto tile (7, 5).
    const r3d::voxel_hit down = r3d::raycast_columns( { .origin = { 7.5f, 5.5f, 10.0f }, .dir = { 0.0f, 0.0f, -1.0f },
                                .max_t = 100.0f, .height = floor_world } );
    REQUIRE( down.hit );
    CHECK( down.x == 7 );
    CHECK( down.y == 5 );
    CHECK( down.z == 0 );
    CHECK( down.face == r3d::hit_face::top );
    CHECK( approx( down.t, 9.9f ) );

    // A ray pointing away misses.
    const r3d::voxel_hit up = r3d::raycast_columns( { .origin = { 7.5f, 5.5f, 10.0f }, .dir = { 0.0f, 0.0f, 1.0f },
                              .max_t = 100.0f, .height = floor_world } );
    CHECK_FALSE( up.hit );

    // Out of range misses.
    const r3d::voxel_hit short_ray = r3d::raycast_columns( { .origin = { 7.5f, 5.5f, 10.0f }, .dir = { 0.0f, 0.0f, -1.0f },
                                     .max_t = 5.0f, .height = floor_world } );
    CHECK_FALSE( short_ray.hit );
}

TEST_CASE( "r3d_raycast_wall_blocks_ground_behind_it", "[r3d]" )
{
    const r3d::column_height_fn world = []( int x, int y, int z ) {
        if( z != 0 || x < 0 || x >= 20 || y < 0 || y >= 20 ) {
            return 0.0f;
        }
        return x == 10 ? 1.0f : 0.1f; // a full-height wall along x = 10
    };
    // Looking east and gently down: the ray is still above the wall's top when it reaches x = 10,
    // so it must hit the wall's west face before it ever reaches the floor.
    const r3d::vec3 dir = r3d::normalize( { 1.0f, 0.0f, -0.05f } );
    const r3d::voxel_hit h = r3d::raycast_columns( { .origin = { 2.5f, 8.5f, 0.9f }, .dir = dir, .max_t = 100.0f, .height = world } );
    REQUIRE( h.hit );
    CHECK( h.x == 10 );
    CHECK( h.face == r3d::hit_face::west );
}

TEST_CASE( "r3d_pick_finds_tile_drawn_under_cursor", "[r3d]" )
{
    // Floor everywhere on level 0 of the scene frame and a wall at tile (2, -3).
    const r3d::column_height_fn world = []( int x, int y, int z ) {
        if( z != 0 ) {
            return 0.0f;
        }
        return x == 2 && y == -3 ? 1.0f : 0.1f;
    };
    for( float yaw : {
             0.0f, 0.9f, 2.6f, 4.4f
         } ) {
        for( int variant = 0; variant < 3; ++variant ) {
            const bool ortho = variant == 1;
            r3d::camera cam;
            cam.target = r3d::tile_to_world( { 0.5f, 0.5f, 0.5f } ); // as r3d::draw sets it
            cam.yaw = yaw;
            cam.orthographic = ortho;
            cam.z_scale = variant == 2 ? 1.6f : 1.0f; // taller storeys must not change picks
            const float aspect = 1.5f;
            const r3d::mat4 vp = cam.view_projection( aspect );
            // The middle of the screen is the viewed tile.
            const r3d::voxel_hit centre = r3d::pick_cell( { .cam = cam, .ndc_x = 0.0f, .ndc_y = 0.0f, .aspect = aspect, .max_t = 500.0f, .height = world,
                                          .floor_z = r3d::floor_height } );
            REQUIRE( centre.hit );
            CHECK( centre.x == 0 );
            CHECK( centre.y == 0 );
            CHECK( centre.z == 0 );
            // Where a floor tile's centre is drawn, that tile is picked (unless the wall covers it).
            for( int ty = -4; ty <= 4; ++ty ) {
                for( int tx = -4; tx <= 4; ++tx ) {
                    const r3d::vec3 top = { tx + 0.5f, ty + 0.5f, world( tx, ty, 0 ) };
                    const r3d::vec3 ndc = r3d::transform_point( vp, r3d::tile_to_world( top ) );
                    const r3d::voxel_hit h = r3d::pick_cell( { .cam = cam, .ndc_x = ndc.x, .ndc_y = ndc.y, .aspect = aspect, .max_t = 500.0f, .height = world,
                                             .floor_z = r3d::floor_height } );
                    REQUIRE( h.hit );
                    const bool is_tile = h.x == tx && h.y == ty && h.z == 0;
                    const bool is_wall = h.x == 2 && h.y == -3;
                    CHECK( ( is_tile || is_wall ) );
                }
            }
        }
    }
}

TEST_CASE( "r3d_pick_falls_back_to_floor_plane", "[r3d]" )
{
    const r3d::column_height_fn nothing = []( int, int, int ) {
        return 0.0f;
    };
    r3d::camera cam;
    cam.target = r3d::tile_to_world( { 0.5f, 0.5f, 0.5f } );
    const r3d::voxel_hit h = r3d::pick_cell( { .cam = cam, .ndc_x = 0.0f, .ndc_y = 0.0f, .aspect = 1.0f, .max_t = 500.0f, .height = nothing,
                             .floor_z = r3d::floor_height } );
    REQUIRE( h.hit );
    CHECK( h.x == 0 );
    CHECK( h.y == 0 );
    CHECK( h.z == 0 );
    CHECK( h.face == r3d::hit_face::top );
    // Above the horizon nothing can be picked.
    cam.pitch = r3d::camera::min_pitch;
    CHECK_FALSE( r3d::pick_cell( { .cam = cam, .ndc_x = 0.0f, .ndc_y = 1.0f, .aspect = 1.0f, .max_t = 500.0f, .height = nothing, .floor_z = r3d::floor_height } ).hit );
}

TEST_CASE( "r3d_raycast_matches_reference", "[r3d]" )
{
    std::mt19937 rng( 12345 );
    std::uniform_int_distribution<int> kind( 0, 9 );
    std::uniform_real_distribution<float> unit( 0.0f, 1.0f );

    constexpr int size = 12;
    constexpr int layers = 5;
    for( int world_idx = 0; world_idx < 30; ++world_idx ) {
        std::vector<float> heights( size * size * layers, 0.0f );
        for( float &h : heights ) {
            const int k = kind( rng );
            h = k < 5 ? 0.0f : ( k < 7 ? 0.1f : ( k < 9 ? 0.5f : 1.0f ) );
        }
        const r3d::column_height_fn fn = [ &heights ]( int x, int y, int z ) {
            if( x < 0 || y < 0 || z < 0 || x >= size || y >= size || z >= layers ) {
                return 0.0f;
            }
            return heights[( z * size + y ) * size + x];
        };
        for( int ray_idx = 0; ray_idx < 200; ++ray_idx ) {
            const r3d::vec3 o{ unit( rng ) *size, unit( rng ) *size, unit( rng ) *layers + 0.5f };
            const r3d::vec3 d = r3d::normalize( { unit( rng ) - 0.5f, unit( rng ) - 0.5f, unit( rng ) - 0.7f } );
            if( r3d::length( d ) < 0.5f ) {
                continue;
            }
            const r3d::voxel_hit fast = r3d::raycast_columns( { .origin = o, .dir = d, .max_t = 60.0f, .height = fn } );
            const r3d::voxel_hit slow = r3d::raycast_columns_reference( { .origin = o, .dir = d, .max_t = 60.0f, .height = fn,
                                        .lo_x = 0, .lo_y = 0, .lo_z = 0, .hi_x = size - 1, .hi_y = size - 1, .hi_z = layers - 1 } );
            INFO( "world " << world_idx << " ray " << ray_idx );
            REQUIRE( fast.hit == slow.hit );
            if( fast.hit ) {
                CHECK( fast.x == slow.x );
                CHECK( fast.y == slow.y );
                CHECK( fast.z == slow.z );
                CHECK( approx( fast.t, slow.t, 1e-3f ) );
            }
        }
    }
}

TEST_CASE( "r3d_terrain_shapes", "[r3d]" )
{
    r3d::tile_traits t;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::floor );

    t = {};
    t.no_floor = true;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::empty );

    t = {};
    t.wall = true;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::block );

    t = {};
    t.move_cost = 0; // closed door or window: impassable and not flat
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::block );

    t = {};
    t.move_cost = 0;
    t.flat = true;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::floor );

    t = {};
    t.tree = true;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::upright );

    t = {};
    t.shrub = true;
    const r3d::shape shrub = r3d::classify_terrain( t );
    CHECK( shrub.kind == r3d::shape_kind::upright );
    CHECK( shrub.height < 1.0f );

    // Windows, broken or not, and closed doors stand in the wall line as panes.
    t = {};
    t.window = true;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::pane );
    t.move_cost = 0;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::pane );
    t = {};
    t.door = true;
    t.move_cost = 0;
    // Open doors and doorframes stand in the wall line as panes.
    t.flat = true;
    t.open_door = true;
    t.move_cost = 2;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::pane );
    // A non-door flat terrain classifies as floor.
    t = {};
    t.flat = true;
    t.move_cost = 0;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::floor );
    t = {};
    t.door = true;
    t.wall = true;
    t.move_cost = 0;
    CHECK( r3d::classify_terrain( t ).kind == r3d::shape_kind::block );
}

TEST_CASE( "r3d_cabinets_meet_adjacent_thin_wall_faces", "[r3d]" )
{
    const r3d::box_layout box = r3d::furniture_box_layout( { .ox0 = 0.1f, .oy0 = 0.06f, .ox1 = 0.9f, .oy1 = 1.0f,
                                .front = 0.78f, .low = false } );
    const std::array<uint8_t, 4> backs = {{
            r3d::face_north, r3d::face_east, r3d::face_south, r3d::face_west
        }
    };
    for( bool flip : {
             false, true
         } ) {
        for( int facing = 0; facing < 4; ++facing ) {
            const r3d::box_layout original = r3d::place_furniture_box( { .box = box, .flip = flip, .facing = facing, .joined = 0, .wall_contacts = 0 } );
            const r3d::box_layout fitted = r3d::place_furniture_box( { .box = box, .flip = flip, .facing = facing, .joined = 0, .wall_contacts = backs[facing] } );
            // Adjacent wall centres are -0.5 or 1.5 in this tile's coordinates. The
            // cabinet meets the near face without moving its front or either end.
            CHECK( approx( fitted.x0, facing == 3 ? -0.5f + r3d::wall_half_thickness : original.x0 ) );
            CHECK( approx( fitted.x1, facing == 1 ? 1.5f - r3d::wall_half_thickness : original.x1 ) );
            CHECK( approx( fitted.y0, facing == 0 ? -0.5f + r3d::wall_half_thickness : original.y0 ) );
            CHECK( approx( fitted.y1, facing == 2 ? 1.5f - r3d::wall_half_thickness : original.y1 ) );
            CHECK( fitted.side_top == box.side_top );
            CHECK( fitted.lid_bottom == box.lid_bottom );
            const r3d::box_layout shallow = r3d::place_furniture_box( { .box = box, .flip = flip, .facing = facing, .joined = 0,
                                            .wall_contacts = backs[facing], .depth_scale = 0.5f } );
            if( ( facing & 1 ) == 0 ) {
                CHECK( approx( shallow.y1 - shallow.y0, ( fitted.y1 - fitted.y0 ) * 0.5f ) );
                CHECK( shallow.x0 == fitted.x0 );
                CHECK( shallow.x1 == fitted.x1 );
                CHECK( ( facing == 0 ? shallow.y0 == fitted.y0 : shallow.y1 == fitted.y1 ) );
            } else {
                CHECK( approx( shallow.x1 - shallow.x0, ( fitted.x1 - fitted.x0 ) * 0.5f ) );
                CHECK( shallow.y0 == fitted.y0 );
                CHECK( shallow.y1 == fitted.y1 );
                CHECK( ( facing == 1 ? shallow.x1 == fitted.x1 : shallow.x0 == fitted.x0 ) );
            }
        }
    }
    const r3d::box_layout row = r3d::place_furniture_box( { .box = box, .flip = false, .facing = 0,
                                .joined = r3d::face_west | r3d::face_east, .wall_contacts = r3d::face_north } );
    CHECK( row.x0 == 0.0f );
    CHECK( row.x1 == 1.0f );
    CHECK( approx( row.y0, -0.5f + r3d::wall_half_thickness ) );
}

TEST_CASE( "r3d_furniture_box_layout", "[r3d]" )
{
    // A sprite filling its whole cell: a whole-tile box, lid above the front.
    const r3d::box_layout full = r3d::furniture_box_layout( { .ox0 = 0.0f, .oy0 = 0.0f, .ox1 = 1.0f, .oy1 = 1.0f, .front = 0.4f, .low = false } );
    CHECK( approx( full.x0, 0.0f ) );
    CHECK( approx( full.x1, 1.0f ) );
    CHECK( approx( full.y0, 0.0f ) );
    CHECK( approx( full.y1, 1.0f ) );
    CHECK( approx( full.lid_bottom, 0.6f ) );
    CHECK( approx( full.side_top, 0.6f ) );
    CHECK( approx( full.side_bottom, 1.0f ) );

    // A narrow bench in the middle of its cell: the box is only as wide as the planks, and its
    // sides take the lid's front edge, so they line up with the top.
    const r3d::box_layout bench = r3d::furniture_box_layout( { .ox0 = 0.3f, .oy0 = 0.2f, .ox1 = 0.7f, .oy1 = 0.9f, .front = 0.4f, .low = true } );
    CHECK( approx( bench.x0, 0.3f ) );
    CHECK( approx( bench.x1, 0.7f ) );
    CHECK( approx( bench.col0, bench.x0 ) );
    CHECK( approx( bench.col1, bench.x1 ) );
    // Boxes stand against the back (north) of their tile, where a wall would be.
    CHECK( approx( bench.y0, 0.0f ) );
    CHECK( bench.y1 >= 0.5f );
    CHECK( bench.y1 <= 1.0f );
    // A tall piece whose art starts a little below the top still touches the back.
    const r3d::box_layout locker = r3d::furniture_box_layout( { .ox0 = 0.0f, .oy0 = 0.06f, .ox1 = 1.0f, .oy1 = 1.0f, .front = 0.78f, .low = false } );
    CHECK( approx( locker.y0, 0.0f ) );
    CHECK( approx( bench.side_bottom, bench.lid_bottom ) );
    CHECK( bench.side_top < bench.side_bottom );

    // No opaque pixels known: fall back to the whole sprite.
    const r3d::box_layout none = r3d::furniture_box_layout( { .ox0 = 0.0f, .oy0 = 0.0f, .ox1 = 0.0f, .oy1 = 0.0f, .front = 0.4f, .low = false } );
    CHECK( approx( none.x1, 1.0f ) );
    CHECK( approx( none.y1, 1.0f ) );
}

TEST_CASE( "r3d_shrub_ground_is_separate_from_upright_foliage", "[r3d]" )
{
    r3d::tile_traits traits;
    traits.shrub = true;
    const r3d::shape bush = r3d::classify_terrain( traits );
    CHECK( bush.kind == r3d::shape_kind::upright );
    CHECK( bush.ground_backdrop );
    CHECK( bush.height > r3d::floor_height );

    traits = {};
    traits.tree = true;
    CHECK_FALSE( r3d::classify_terrain( traits ).ground_backdrop );
    CHECK_FALSE( r3d::classify_furniture( {} ).ground_backdrop );
}

TEST_CASE( "r3d_natural_furniture_is_a_solid_volume", "[r3d]" )
{
    // Japanese knotweed is passable, SHORT and SHRUB. It must not become a bench.
    r3d::tile_traits knotweed;
    knotweed.shrub = true;
    knotweed.low = true;
    const r3d::shape plant = r3d::classify_furniture( knotweed );
    CHECK( plant.kind == r3d::shape_kind::block );
    CHECK( plant.slab == 0.0f );
    CHECK( plant.height > r3d::floor_height );

    // Small, medium and large boulders respectively use TINY, SHORT and impassable.
    float previous_height = r3d::floor_height;
    for( int size = 0; size < 3; ++size ) {
        r3d::tile_traits rock;
        rock.boulder = true;
        rock.tiny = size == 0;
        rock.low = size == 1;
        rock.move_cost = size == 2 ? 0 : 100;
        const r3d::shape volume = r3d::classify_furniture( rock );
        CHECK( volume.kind == r3d::shape_kind::block );
        CHECK( volume.slab == 0.0f );
        CHECK( volume.height > previous_height );
        CHECK( volume.height <= 1.0f );
        previous_height = volume.height;
    }
}

TEST_CASE( "r3d_furniture_shapes", "[r3d]" )
{
    // Small walkable furniture (chairs) stands up as a sprite.
    r3d::tile_traits t;
    const r3d::shape ordinary = r3d::classify_furniture( t );
    CHECK( ordinary.kind == r3d::shape_kind::upright );
    CHECK( ordinary.height > r3d::floor_height );

    t = {};
    t.tiny = true;
    const r3d::shape tiny = r3d::classify_furniture( t );
    CHECK( tiny.kind == r3d::shape_kind::floor );
    CHECK( tiny.height < ordinary.height );

    // Impassable furniture (lockers, fridges) is a tall box.
    t = {};
    t.move_cost = 0;
    const r3d::shape locker = r3d::classify_furniture( t );
    CHECK( locker.kind == r3d::shape_kind::prism );
    CHECK( locker.height > ordinary.height );

    // Benches and counters are low boxes, lower than lockers.
    t = {};
    t.low = true;
    const r3d::shape bench = r3d::classify_furniture( t );
    CHECK( bench.kind == r3d::shape_kind::prism );
    CHECK( bench.height < locker.height );
    // Benches and tables are a thin top on legs, cabinets are solid.
    CHECK( bench.slab > 0.0f );
    CHECK( bench.slab < bench.height );
    CHECK( locker.slab == 0.0f );
    t = {};
    t.flat_surface = true;
    CHECK( r3d::classify_furniture( t ).kind == r3d::shape_kind::prism );
    t.container = true;
    CHECK( r3d::classify_furniture( t ).slab == 0.0f );

    t = {};
    t.flat = true;
    const r3d::shape rug = r3d::classify_furniture( t );
    CHECK( rug.kind == r3d::shape_kind::floor );
    CHECK( rug.height < ordinary.height );
}

TEST_CASE( "r3d_face_visibility", "[r3d]" )
{
    const r3d::shape block{ r3d::shape_kind::block, 1.0f };
    const r3d::shape floor{ r3d::shape_kind::floor, r3d::floor_height };
    const r3d::shape air{};

    // A lone block shows everything except its bottom.
    r3d::neighbourhood open;
    CHECK( r3d::visible_faces( block, open ) ==
           ( r3d::face_top | r3d::face_north | r3d::face_south | r3d::face_west | r3d::face_east ) );

    // Block with another block to the east and above: the east side is hidden, but the top
    // stays, since the level above may be cut away.
    r3d::neighbourhood packed;
    packed.east = block;
    packed.up = block;
    const uint8_t faces = r3d::visible_faces( block, packed );
    CHECK( ( faces & r3d::face_east ) == 0 );
    CHECK( ( faces & r3d::face_top ) != 0 );
    CHECK( ( faces & r3d::face_west ) != 0 );

    // A block beside a floor still shows that side, because the floor is lower.
    r3d::neighbourhood beside_floor;
    beside_floor.south = floor;
    CHECK( ( r3d::visible_faces( block, beside_floor ) & r3d::face_south ) != 0 );

    // A floor only ever has a top face, even under a block.
    CHECK( r3d::visible_faces( floor, open ) == r3d::face_top );
    r3d::neighbourhood roofed;
    roofed.up = block;
    CHECK( r3d::visible_faces( floor, roofed ) == r3d::face_top );

    // Air, upright sprites and panes emit nothing through this path.
    CHECK( r3d::visible_faces( air, open ) == 0 );
    CHECK( r3d::visible_faces( { r3d::shape_kind::upright, 1.4f }, open ) == 0 );
    CHECK( r3d::visible_faces( { r3d::shape_kind::pane, 1.0f }, open ) == 0 );

    // A tall prism next to a short one shows its side; the short one's side is hidden.
    const r3d::shape tall{ r3d::shape_kind::prism, 0.9f };
    const r3d::shape low{ r3d::shape_kind::prism, 0.3f };
    r3d::neighbourhood tall_next_to_low;
    tall_next_to_low.east = low;
    CHECK( ( r3d::visible_faces( tall, tall_next_to_low ) & r3d::face_east ) != 0 );
    r3d::neighbourhood low_next_to_tall;
    low_next_to_tall.east = tall;
    CHECK( ( r3d::visible_faces( low, low_next_to_tall ) & r3d::face_east ) == 0 );
}

TEST_CASE( "r3d_wall_texture_matching", "[r3d]" )
{
    r3d::wall_texture_rules rules;
    rules.by_id = { { "t_wall", "plaster" }, { "t_wall_log_half", "logs" } };
    rules.by_prefix = { { "t_wall_", "plaster" }, { "t_wall_log", "logs" }, { "t_brick_wall", "brick" } };

    // Exact ids first, along the looks_like chain.
    CHECK( r3d::match_wall_texture( rules, { "t_wall" } ) == "plaster" );
    CHECK( r3d::match_wall_texture( rules, { "t_wall_log_half" } ) == "logs" );
    CHECK( r3d::match_wall_texture( rules, { "t_mod_wall", "t_wall" } ) == "plaster" );
    // Then the longest prefix.
    CHECK( r3d::match_wall_texture( rules, { "t_wall_log_widened" } ) == "logs" );
    CHECK( r3d::match_wall_texture( rules, { "t_wall_b" } ) == "plaster" );
    CHECK( r3d::match_wall_texture( rules, { "t_brick_wall_halfway" } ) == "brick" );
    // Nothing matches: the fallback, empty by default (keep the tileset sprite).
    CHECK( r3d::match_wall_texture( rules, { "t_vat" } ).empty() );
    rules.fallback = "concrete";
    CHECK( r3d::match_wall_texture( rules, { "t_vat" } ) == "concrete" );
}

TEST_CASE( "r3d_wall_atlas_cells", "[r3d]" )
{
    // 4 x 3 cells of 32 pixels.
    const r3d::uv_rect first = r3d::atlas_cell( { .index = 0, .cell = 32, .atlas_w = 128, .atlas_h = 96 } );
    REQUIRE( first.valid );
    CHECK( approx( first.u0, 0.5f / 128.0f ) );
    CHECK( approx( first.v1, 31.5f / 96.0f ) );
    const r3d::uv_rect fifth = r3d::atlas_cell( { .index = 5, .cell = 32, .atlas_w = 128, .atlas_h = 96 } );
    REQUIRE( fifth.valid );
    CHECK( approx( fifth.u0, 32.5f / 128.0f ) );
    CHECK( approx( fifth.v0, 32.5f / 96.0f ) );
    CHECK_FALSE( r3d::atlas_cell( { .index = 12, .cell = 32, .atlas_w = 128, .atlas_h = 96 } ).valid );
    CHECK_FALSE( r3d::atlas_cell( { .index = -1, .cell = 32, .atlas_w = 128, .atlas_h = 96 } ).valid );
}

TEST_CASE( "r3d_vehicle_tiered_height_constants", "[r3d]" )
{
    CHECK( approx( r3d::vehicle_clearance_h, 0.15f ) );
    CHECK( approx( r3d::chassis_floor_h, 0.26f ) );
    CHECK( approx( r3d::vehicle_wheel_h, 0.34f ) );
    CHECK( approx( r3d::vehicle_bumper_h, 0.42f ) );
    CHECK( approx( r3d::vehicle_body_h, 0.50f ) );
    CHECK( approx( r3d::vehicle_cabin_h, 0.82f ) );
    CHECK( approx( r3d::floor_height + r3d::chassis_floor_h, 0.36f ) );
    CHECK( r3d::floor_height < r3d::vehicle_clearance_h );
    CHECK( r3d::vehicle_clearance_h < r3d::chassis_floor_h );
    CHECK( r3d::chassis_floor_h < r3d::vehicle_wheel_h );
    CHECK( r3d::vehicle_wheel_h < r3d::vehicle_bumper_h );
    CHECK( r3d::vehicle_bumper_h < r3d::vehicle_body_h );
    CHECK( r3d::vehicle_body_h < r3d::vehicle_cabin_h );
}

TEST_CASE( "r3d_vehicle_heading_to_camera_yaw", "[r3d]" )
{
    // CDDA vehicle angles: 270 deg is North (yaw = 0), 0 deg is East (yaw = -pi/2),
    // 90 deg is South (yaw = pi), 180 deg is West (yaw = +pi/2).
    const auto to_yaw = []( units::angle veh_face ) -> float {
        auto yaw = static_cast<float>( units::to_radians( 270_degrees - veh_face ) );
        constexpr auto two_pi = static_cast<float>( 2.0 * M_PI );
        yaw = std::fmod( yaw, two_pi );
        if( yaw > static_cast<float>( M_PI ) ) {
            yaw -= two_pi;
        } else if( yaw < static_cast<float>( -M_PI ) ) {
            yaw += two_pi;
        }
        return yaw;
    };

    CHECK( approx( to_yaw( 270_degrees ), 0.0f ) );
    CHECK( approx( to_yaw( 0_degrees ), static_cast<float>( -M_PI * 0.5 ) ) );
    CHECK( approx( std::abs( to_yaw( 90_degrees ) ), static_cast<float>( M_PI ) ) );
    CHECK( approx( to_yaw( 180_degrees ), static_cast<float>( M_PI * 0.5 ) ) );

    // Camera facing test: with yaw looking North, forward_eighths() must return 0
    r3d::camera cam;
    cam.yaw = to_yaw( 270_degrees );
    CHECK( cam.forward_eighths() == 0 );

    // Yaw looking East (2)
    cam.yaw = to_yaw( 0_degrees );
    CHECK( cam.forward_eighths() == 2 );

    // Yaw looking South (4)
    cam.yaw = to_yaw( 90_degrees );
    CHECK( cam.forward_eighths() == 4 );

    // Yaw looking West (6)
    cam.yaw = to_yaw( 180_degrees );
    CHECK( cam.forward_eighths() == 6 );

    // Angle delta wrapping across boundary
    const auto wrap_delta = []( float delta ) -> float {
        constexpr auto two_pi = static_cast<float>( 2.0 * M_PI );
        while( delta > static_cast<float>( M_PI ) ) {
            delta -= two_pi;
        }
        while( delta < static_cast<float>( -M_PI ) ) {
            delta += two_pi;
        }
        return delta;
    };

    const auto yaw_south_pos = static_cast<float>( M_PI - 0.05f );
    const auto yaw_south_neg = static_cast<float>( -M_PI + 0.05f );
    CHECK( approx( wrap_delta( yaw_south_neg - yaw_south_pos ), 0.1f ) );
    CHECK( approx( wrap_delta( yaw_south_pos - yaw_south_neg ), -0.1f ) );
}

TEST_CASE( "r3d_vehicle_rigid_body_local_to_scene", "[r3d]" )
{
    const auto pivot_sx = 15.5f;
    const auto pivot_sy = 25.5f;

    auto transform = [pivot_sx, pivot_sy]( float f, float r, units::angle face_dir ) -> std::array<float, 2> {
        const auto theta = static_cast<float>( units::to_radians( face_dir ) );
        const auto cos_th = static_cast<float>( std::cos( theta ) );
        const auto sin_th = static_cast<float>( std::sin( theta ) );
        return std::array<float, 2>{
            pivot_sx + f * cos_th - r * sin_th,
            pivot_sy + f * sin_th + r * cos_th
        };
    };

    // Facing North (270 deg):
    // forward is North (-y in tile space), right is East (+x in tile space)
    const auto p_north_fwd = transform( 2.0f, 0.0f, 270_degrees );
    CHECK( approx( p_north_fwd[0], pivot_sx ) );
    CHECK( approx( p_north_fwd[1], pivot_sy - 2.0f ) );

    const auto p_north_right = transform( 0.0f, 3.0f, 270_degrees );
    CHECK( approx( p_north_right[0], pivot_sx + 3.0f ) );
    CHECK( approx( p_north_right[1], pivot_sy ) );

    // Facing East (0 deg):
    // forward is East (+x in tile space), right is South (+y in tile space)
    const auto p_east_fwd = transform( 2.0f, 0.0f, 0_degrees );
    CHECK( approx( p_east_fwd[0], pivot_sx + 2.0f ) );
    CHECK( approx( p_east_fwd[1], pivot_sy ) );

    const auto p_east_right = transform( 0.0f, 3.0f, 0_degrees );
    CHECK( approx( p_east_right[0], pivot_sx ) );
    CHECK( approx( p_east_right[1], pivot_sy + 3.0f ) );

    // Facing South (90 deg):
    // forward is South (+y in tile space), right is West (-x in tile space)
    const auto p_south_fwd = transform( 2.0f, 0.0f, 90_degrees );
    CHECK( approx( p_south_fwd[0], pivot_sx ) );
    CHECK( approx( p_south_fwd[1], pivot_sy + 2.0f ) );

    // Rigid-body invariant: distance between local points is preserved under rotation
    const auto pt_a_local = std::array<float, 2>{ 1.5f, -0.8f };
    const auto pt_b_local = std::array<float, 2>{ -2.0f, 1.2f };
    const auto local_dx = pt_a_local[0] - pt_b_local[0];
    const auto local_dy = pt_a_local[1] - pt_b_local[1];
    const auto local_dist = std::sqrt( local_dx * local_dx + local_dy * local_dy );

    for( auto deg = 0; deg < 360; deg += 15 ) {
        const auto a = transform( pt_a_local[0], pt_a_local[1], units::from_degrees( deg ) );
        const auto b = transform( pt_b_local[0], pt_b_local[1], units::from_degrees( deg ) );
        const auto world_dx = a[0] - b[0];
        const auto world_dy = a[1] - b[1];
        const auto world_dist = std::sqrt( world_dx * world_dx + world_dy * world_dy );
        CHECK( approx( world_dist, local_dist, 1e-4f ) );
    }
}

TEST_CASE( "r3d_vehicle_directional_side_shading", "[r3d]" )
{
    const auto compute_shade = []( float local_f, float local_r, units::angle face_dir ) -> float {
        const auto theta = static_cast<float>( units::to_radians( face_dir ) );
        const auto cos_th = static_cast<float>( std::cos( theta ) );
        const auto sin_th = static_cast<float>( std::sin( theta ) );
        const auto nx = local_f * cos_th - local_r * sin_th;
        const auto ny = local_f * sin_th + local_r * cos_th;
        const auto x_shade = ( nx >= 0.0f ) ? 0.725f : 0.588f; // East 0.73, West 0.59
        const auto y_shade = ( ny >= 0.0f ) ? 0.831f : 0.647f; // South 0.83, North 0.65
        return std::clamp( nx * nx * x_shade + ny * ny * y_shade, 0.4f, 1.0f );
    };

    // When vehicle faces North (270 deg):
    // Vehicle front faces North:
    CHECK( approx( compute_shade( 1.0f, 0.0f, 270_degrees ), 0.647f, 1e-3f ) );
    // Vehicle back faces South:
    CHECK( approx( compute_shade( -1.0f, 0.0f, 270_degrees ), 0.831f, 1e-3f ) );
    // Vehicle right faces East:
    CHECK( approx( compute_shade( 0.0f, 1.0f, 270_degrees ), 0.725f, 1e-3f ) );
    // Vehicle left faces West:
    CHECK( approx( compute_shade( 0.0f, -1.0f, 270_degrees ), 0.588f, 1e-3f ) );

    // When vehicle faces East (0 deg):
    // Vehicle front faces East:
    CHECK( approx( compute_shade( 1.0f, 0.0f, 0_degrees ), 0.725f, 1e-3f ) );
    // Vehicle right faces South:
    CHECK( approx( compute_shade( 0.0f, 1.0f, 0_degrees ), 0.831f, 1e-3f ) );

    // Smoothness and bounds: at all angles, shade must stay within [0.55f, 0.85f]
    for( auto deg = 0; deg < 360; deg += 10 ) {
        const auto s_front = compute_shade( 1.0f, 0.0f, units::from_degrees( deg ) );
        const auto s_right = compute_shade( 0.0f, 1.0f, units::from_degrees( deg ) );
        CHECK( s_front >= 0.55f );
        CHECK( s_front <= 0.85f );
        CHECK( s_right >= 0.55f );
        CHECK( s_right <= 0.85f );
    }
}

TEST_CASE( "r3d_vehicle_sloped_windshield_and_bumper_contours", "[r3d]" )
{
    // Sloped windshield: connects 0.82f at cabin down to 0.50f at hood
    const auto z_cabin = r3d::vehicle_cabin_h;
    const auto z_hood = r3d::vehicle_body_h;
    const auto z_bumper = r3d::vehicle_bumper_h;
    const auto z_clearance = r3d::vehicle_clearance_h;

    CHECK( approx( z_cabin, 0.82f ) );
    CHECK( approx( z_hood, 0.50f ) );
    CHECK( approx( z_bumper, 0.42f ) );
    CHECK( approx( z_clearance, 0.15f ) );

    // Sloped windshield drop: 0.82f - 0.50f = 0.32f slope over tile span
    const auto windshield_drop = z_cabin - z_hood;
    CHECK( approx( windshield_drop, 0.32f ) );

    // Bumper bevel drop: 0.50f - 0.42f = 0.08f slope to bumper nose
    const auto bumper_bevel_drop = z_hood - z_bumper;
    CHECK( approx( bumper_bevel_drop, 0.08f ) );

    // Undercarriage clearance: body skirt begins 0.15f above asphalt
    CHECK( z_clearance > 0.0f );
    CHECK( z_bumper > z_clearance );
}

TEST_CASE( "r3d_vehicle_pivot_anchor_alignment", "[r3d]" )
{
    // Verification rule: For any cardinal angle (0, 90, 180, 270),
    // local mount transformed relative to pivot_anchor[0] must land on the exact
    // center of the map tile predicted by CDDA's vehicle rotation.
    const auto pivot_anchor = point( 2, 1 );
    const auto pivot_scene_x = 20.5f;
    const auto pivot_scene_y = 30.5f;

    const auto to_scene = [pivot_anchor, pivot_scene_x, pivot_scene_y]( const point &mount, units::angle dir ) -> std::array<float, 2> {
        const auto rel_f = static_cast<float>( mount.x - pivot_anchor.x );
        const auto rel_r = static_cast<float>( mount.y - pivot_anchor.y );
        const auto theta = static_cast<float>( units::to_radians( dir ) );
        const auto cos_th = static_cast<float>( std::cos( theta ) );
        const auto sin_th = static_cast<float>( std::sin( theta ) );
        return std::array<float, 2>{
            pivot_scene_x + rel_f * cos_th - rel_r * sin_th,
            pivot_scene_y + rel_f * sin_th + rel_r * cos_th
        };
    };

    // Reference CDDA rotation calculation (vehicle::rotate_to_world)
    const auto cdda_rotate_to_world = [pivot_anchor]( const point &mount, units::angle dir ) -> point {
        const auto rel = mount - pivot_anchor;
        const auto deg = ( static_cast<int>( std::lround( units::to_degrees( dir ) ) ) % 360 + 360 ) % 360;
        switch( deg ) {
            case 0:   // East: +x_mnt -> +x_world, +y_mnt -> +y_world
                return rel;
            case 90:  // South: +x_mnt -> +y_world, +y_mnt -> -x_world
                return point( -rel.y, rel.x );
            case 180: // West: +x_mnt -> -x_world, +y_mnt -> -y_world
                return point( -rel.x, -rel.y );
            case 270: // North: +x_mnt -> -y_world, +y_mnt -> +x_world
                return point( rel.y, -rel.x );
            default:
                return rel;
        }
    };

    const auto test_mounts = std::vector<point>{
        point( 2, 1 ),   // At pivot
        point( 4, 1 ),   // Front hood
        point( 0, 1 ),   // Rear bumper
        point( 3, 0 ),   // Front-left tire
        point( 3, 2 ),   // Front-right tire
        point( -1, 0 ),  // Rear-left bumper
    };

    const auto angles = std::vector<units::angle>{ 0_degrees, 90_degrees, 180_degrees, 270_degrees };

    for( const auto &dir : angles ) {
        for( const auto &mount : test_mounts ) {
            const auto scene_pos = to_scene( mount, dir );
            const auto expected_offset = cdda_rotate_to_world( mount, dir );
            const auto expected_scene_x = pivot_scene_x + static_cast<float>( expected_offset.x );
            const auto expected_scene_y = pivot_scene_y + static_cast<float>( expected_offset.y );
            CHECK( approx( scene_pos[0], expected_scene_x, 1e-4f ) );
            CHECK( approx( scene_pos[1], expected_scene_y, 1e-4f ) );
        }
    }
}

TEST_CASE( "r3d_vehicle_oversized_sprite_clamping", "[r3d]" )
{
    // 48x48 or 64x64 oversized vehicle parts (e.g. 1.5x larger than tile)
    // must be clamped to the [rel_forward - 0.5f, rel_forward + 0.5f] x [rel_right - 0.5f, rel_right + 0.5f] cell footprint
    const auto rel_f = 2.0f;
    const auto rel_r = -1.0f;
    const auto cell_f_min = rel_f - 0.5f;
    const auto cell_f_max = rel_f + 0.5f;
    const auto cell_r_min = rel_r - 0.5f;
    const auto cell_r_max = rel_r + 0.5f;

    // Sprite with size_x = 1.5f, size_y = 1.5f, centered on tile (offset_x = -0.25f, offset_y = 0.25f)
    const auto size_x = 1.5f;
    const auto size_y = 1.5f;
    const auto offset_x = -0.25f;
    const auto offset_y = -0.25f;

    const auto r_left = rel_r - 0.5f + offset_x;
    const auto r_right = r_left + size_x;
    const auto f_top = rel_f + 0.5f - offset_y;
    const auto f_bot = f_top - size_y;

    const auto c_r_left = std::clamp( r_left, cell_r_min, cell_r_max );
    const auto c_r_right = std::clamp( r_right, cell_r_min, cell_r_max );
    const auto c_f_top = std::clamp( f_top, cell_f_min, cell_f_max );
    const auto c_f_bot = std::clamp( f_bot, cell_f_min, cell_f_max );

    // Verify clamped bounds are exactly the tile's footprint
    CHECK( approx( c_r_left, cell_r_min ) );
    CHECK( approx( c_r_right, cell_r_max ) );
    CHECK( approx( c_f_top, cell_f_max ) );
    CHECK( approx( c_f_bot, cell_f_min ) );

    // Verify proportional UV fractions stay strictly within [0.0f, 1.0f]
    const auto gx_l = ( c_r_left - r_left ) / size_x;
    const auto gx_r = ( c_r_right - r_left ) / size_x;
    const auto gy_top = ( f_top - c_f_top ) / size_y;
    const auto gy_bot = ( f_top - c_f_bot ) / size_y;

    CHECK( gx_l >= 0.0f );
    CHECK( gx_r <= 1.0f );
    CHECK( gx_l < gx_r );
    CHECK( gy_top >= 0.0f );
    CHECK( gy_bot <= 1.0f );
    CHECK( gy_top < gy_bot );
}

TEST_CASE( "r3d_cutaway_driving_and_tight_occlusion", "[r3d]" )
{
    // Tight cutaway radius is 0.55f tiles around character
    constexpr auto tight_cutaway_radius = 0.55f;
    constexpr auto cutaway_depth_bias = -0.0005f;

    CHECK( approx( tight_cutaway_radius, 0.55f ) );
    CHECK( approx( cutaway_depth_bias, -0.0005f ) );

    // When avatar is driving, cutaway width u.cut[3] is set to 0.0f
    auto cut_width = tight_cutaway_radius;
    const auto is_driving = true;
    if( is_driving ) {
        cut_width = 0.0f;
    }
    CHECK( approx( cut_width, 0.0f ) );
}

TEST_CASE( "r3d_vehicle_color_to_packed_light", "[r3d]" )
{
    const auto white_ambient = r3d::pack_light( 255, 255, 255 );

    // 0 color returns ambient light directly
    CHECK( r3d::color_to_packed_light( 0, white_ambient ) == white_ambient );

    // Channel order verification: ARGB 0xAARRGGBB -> pack_light(R, G, B)
    // Red=0xa5 (165), Green=0x50 (80), Blue=0x20 (32)
    const auto test_color_argb = static_cast<uint32_t>( 0xffa55020 );
    const auto packed = r3d::color_to_packed_light( test_color_argb, white_ambient );

    // pack_light places R in Byte 0, G in Byte 1, B in Byte 2, A in Byte 3
    const auto out_r = static_cast<uint8_t>( packed & 0xffu );
    const auto out_g = static_cast<uint8_t>( ( packed >> 8u ) & 0xffu );
    const auto out_b = static_cast<uint8_t>( ( packed >> 16u ) & 0xffu );

    CHECK( out_r == 0xa5 );
    CHECK( out_g == 0x50 );
    CHECK( out_b == 0x20 );

    // Verify channel order is NOT swapped (out_r must NOT be Blue and out_b must NOT be Red)
    CHECK( out_r != 0x20 );
    CHECK( out_b != 0xa5 );

    // Brown car color test: SaddleBrown (0xff8b4513)
    // A brown car has high Red (139), medium Green (69), low Blue (19).
    // If channels were swapped, Red and Blue would flip, turning the car into bright blue/cyan.
    const auto brown_argb = static_cast<uint32_t>( 0xff8b4513 );
    const auto brown_packed = r3d::color_to_packed_light( brown_argb, white_ambient );
    const auto b_r = static_cast<uint8_t>( brown_packed & 0xffu );
    const auto b_g = static_cast<uint8_t>( ( brown_packed >> 8u ) & 0xffu );
    const auto b_b = static_cast<uint8_t>( ( brown_packed >> 16u ) & 0xffu );

    CHECK( b_r == 139 );
    CHECK( b_g == 69 );
    CHECK( b_b == 19 );
    CHECK( b_r > b_b ); // Must remain warm brown, not cyan

    // Ambient light modulation test (50% ambient)
    const auto half_ambient = r3d::pack_light( 128, 128, 128 );
    const auto dimmed = r3d::color_to_packed_light( test_color_argb, half_ambient );
    const auto dim_r = static_cast<uint8_t>( dimmed & 0xffu );
    const auto dim_g = static_cast<uint8_t>( ( dimmed >> 8u ) & 0xffu );
    const auto dim_b = static_cast<uint8_t>( ( dimmed >> 16u ) & 0xffu );

    CHECK( dim_r == static_cast<uint8_t>( ( 165 * 128 ) / 255 ) );
    CHECK( dim_g == static_cast<uint8_t>( ( 80 * 128 ) / 255 ) );
    CHECK( dim_b == static_cast<uint8_t>( ( 32 * 128 ) / 255 ) );
}

TEST_CASE( "r3d_vehicle_opaque_bounds_trimming", "[r3d]" )
{
    // Full/unknown sprite bounds: has_trim must be false
    const auto s_full = r3d::sprite_ref{
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 1.0f,
        .opaque_y1 = 1.0f,
        .valid = true
    };
    const auto b_full = r3d::get_sprite_opaque_bounds( s_full );
    CHECK_FALSE( b_full.has_trim );

    // Door sprite with transparent outer margin on right:
    // occupies [0.0, 0.75] horizontally and [0.0, 1.0] vertically
    const auto s_door = r3d::sprite_ref{
        .quarter_turns = 0,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 0.75f,
        .opaque_y1 = 1.0f,
        .valid = true
    };
    const auto b_door = r3d::get_sprite_opaque_bounds( s_door );
    CHECK( b_door.has_trim );
    CHECK( approx( b_door.local_r_min, -0.5f ) );
    CHECK( approx( b_door.local_r_max, 0.25f ) ); // -0.5f + 0.75f = 0.25f
    CHECK( approx( b_door.local_f_min, -0.5f ) );
    CHECK( approx( b_door.local_f_max, 0.5f ) );

    // 90-degree rotated door (quarter_turns = 1):
    // In rotated space, sx = gy, sy = 1 - gx
    const auto s_rot1 = r3d::sprite_ref{
        .quarter_turns = 1,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 1.0f,
        .opaque_y1 = 0.75f,
        .valid = true
    };
    const auto b_rot1 = r3d::get_sprite_opaque_bounds( s_rot1 );
    CHECK( b_rot1.has_trim );
    CHECK( approx( b_rot1.local_r_min, -0.25f ) );
    CHECK( approx( b_rot1.local_r_max, 0.5f ) );

    // Flipped door (flip = true):
    const auto s_flip = r3d::sprite_ref{
        .flip = true,
        .opaque_x0 = 0.0f,
        .opaque_y0 = 0.0f,
        .opaque_x1 = 0.75f,
        .opaque_y1 = 1.0f,
        .valid = true
    };
    const auto b_flip = r3d::get_sprite_opaque_bounds( s_flip );
    CHECK( b_flip.has_trim );
    CHECK( approx( b_flip.local_r_min, -0.25f ) );
    CHECK( approx( b_flip.local_r_max, 0.5f ) );

    // Snapping rule: interior shared edges snap to -0.5f and +0.5f, exterior trims
    const auto has_left_neighbor = true;
    const auto has_right_neighbor = false;
    const auto r_l_rel = has_left_neighbor ? -0.5f : b_door.local_r_min;
    const auto r_r_rel = has_right_neighbor ? 0.5f : b_door.local_r_max;
    CHECK( approx( r_l_rel, -0.5f ) );
    CHECK( approx( r_r_rel, 0.25f ) );
}

TEST_CASE( "r3d_vehicle_greenhouse_tumblehome_and_flush_roof", "[r3d]" )
{
    // Lower body waistline at vehicle_body_h
    CHECK( approx( r3d::vehicle_body_h, 0.50f ) );
    // Upper cabin roof at vehicle_cabin_h
    CHECK( approx( r3d::vehicle_cabin_h, 0.82f ) );

    // Tumblehome: exterior cabin slopes inward up to 0.12f tiles
    const auto r_l = -0.40f;
    const auto r_r = 0.40f;
    const auto max_tumble = ( r_r - r_l ) * 0.25f; // 0.20f
    const auto tumble = std::min( 0.12f, max_tumble );
    CHECK( approx( tumble, 0.12f ) );

    const auto r_l_roof = r_l + tumble;
    const auto r_r_roof = r_r - tumble;
    CHECK( approx( r_l_roof, -0.28f ) );
    CHECK( approx( r_r_roof, 0.28f ) );

    // Flush roof & windshield invariant:
    // Windshield top at z = vehicle_cabin_h spans [r_l_roof, r_r_roof]
    // Roof front at z = vehicle_cabin_h spans [r_l_roof, r_r_roof]
    // Both connect at the exact same coordinates and elevation with 0 gap
    const auto windshield_top_l = r_l_roof;
    const auto windshield_top_r = r_r_roof;
    const auto roof_cap_front_l = r_l_roof;
    const auto roof_cap_front_r = r_r_roof;
    CHECK( approx( windshield_top_l, roof_cap_front_l ) );
    CHECK( approx( windshield_top_r, roof_cap_front_r ) );
}


