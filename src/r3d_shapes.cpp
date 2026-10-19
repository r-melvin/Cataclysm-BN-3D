#include "r3d_shapes.h"

#include <algorithm>

namespace r3d
{

namespace
{
constexpr auto tree_height = static_cast<float>( 1.4f );
constexpr auto shrub_height = static_cast<float>( 0.7f );
// Furniture heights are in z-levels, which the view draws about 1.6-1.8 tiles (2.5 m) tall: a
// bench seat about knee high as the isometric tilesets draw it, a table or counter ~0.75 m, a
// locker ~1.8 m.
constexpr auto tiny_height = static_cast<float>( 0.15f );
constexpr auto low_height = static_cast<float>( 0.36f );
constexpr auto surface_height = static_cast<float>( 0.4f );
constexpr auto seat_height = static_cast<float>( 0.58f );
constexpr auto default_furniture_height = static_cast<float>( 0.5f );
constexpr auto tall_furniture_height = static_cast<float>( 0.9f );
constexpr auto slab_thickness = static_cast<float>( 0.06f );
// Floor-like furniture (rugs, carpets) sits just above the terrain floor.
constexpr auto decal_height = static_cast<float>( floor_height + 0.01f );
} // namespace

auto classify_terrain( const tile_traits &t ) -> shape
{
    if( t.no_floor ) {
        return { .kind = shape_kind::empty, .height = 0.0f };
    }
    if( t.tree ) {
        return { .kind = shape_kind::upright, .height = tree_height };
    }
    if( t.shrub ) {
        return { .kind = shape_kind::upright, .height = shrub_height, .slab = 0.0f, .ground_backdrop = true };
    }
    // Windows, doors and doorframes stand in the wall line as panels rather than filling the tile.
    if( !t.wall && ( t.window || t.door ) ) {
        return { .kind = shape_kind::pane, .height = 1.0f };
    }
    if( t.wall || ( t.move_cost <= 0 && !t.flat ) ) {
        return { .kind = shape_kind::block, .height = 1.0f };
    }
    return { .kind = shape_kind::floor, .height = floor_height };
}

auto classify_furniture( const tile_traits &t ) -> shape
{
    if( t.panel ) {
        return { .kind = shape_kind::pane, .height = tall_furniture_height };
    }
    if( t.flat ) {
        return { .kind = shape_kind::floor, .height = decal_height };
    }
    // Natural obstacles need a filled volume, not the lid/front/legs of manufactured
    // furniture. Use the same simple box for plants and rocks, with size-based heights.
    if( t.shrub || t.boulder ) {
        const auto height = static_cast<float>( t.tiny ? 0.3f : t.low ? 0.55f : tall_furniture_height );
        return { .kind = shape_kind::block, .height = height };
    }
    if( t.tiny ) {
        return { .kind = shape_kind::floor, .height = tiny_height };
    }
    // Solid furniture is a box: tilesets draw it in 3/4 view, so the top of the sprite becomes
    // the box's top and the rest its sides. Low or flat-topped pieces (benches, counters, tables)
    // are low boxes, other impassable ones (lockers, fridges) tall ones.
    if( t.low || t.flat_surface ) {
        const auto height = static_cast<float>( t.flat_surface ? surface_height : low_height );
        // Benches and tables are a top on legs; cabinets and counters holding items are solid.
        return { .kind = shape_kind::prism, .height = height, .slab = t.container ? 0.0f : slab_thickness };
    }
    // Passable chairs and couches still need a stable solid volume in the orbit view.
    // Reuse the furniture-box path so their source art supplies the top and side materials.
    if( t.seat && t.move_cost > 0 ) {
        return { .kind = shape_kind::prism, .height = seat_height, .slab = slab_thickness };
    }
    if( t.move_cost <= 0 ) {
        return { .kind = shape_kind::prism, .height = tall_furniture_height };
    }
    // Small passable things (plants, for example) stand up as a sprite.
    return { .kind = shape_kind::upright, .height = default_furniture_height };
}

auto furniture_box_layout( const furniture_box_layout_options &options ) -> box_layout
{
    auto ox0 = options.ox0;
    auto oy0 = options.oy0;
    auto ox1 = options.ox1;
    auto oy1 = options.oy1;
    auto front = options.front;
    auto low = options.low;

    auto b = box_layout{};
    if( ox1 - ox0 < 0.05f || oy1 - oy0 < 0.05f ) {
        // no usable bounds: the whole sprite on a whole tile
        ox0 = 0.0f;
        oy0 = 0.0f;
        ox1 = 1.0f;
        oy1 = 1.0f;
    }
    const auto span = static_cast<float>( oy1 - oy0 );
    const auto split = static_cast<float>( oy1 - span * front );
    b.col0 = ox0;
    b.col1 = ox1;
    b.x0 = ox0;
    b.x1 = ox1;
    // Furniture art stands with its back to the north (turned pieces turn with the box), so the
    // box starts at the back of the tile, against whatever wall is there. Its depth follows the
    // lid rows, which cover the tile's depth over the rows above the front of a full sprite.
    const auto depth_rows = static_cast<float>( std::max( 1.0f - front, 0.05f ) );
    b.y0 = 0.0f;
    b.y1 = std::clamp( ( split - oy0 ) / depth_rows, 0.5f, 1.0f );
    b.lid_top = oy0;
    b.lid_bottom = split;
    if( low ) {
        constexpr auto lid_edge = static_cast<float>( 0.12f );
        b.side_top = std::max( oy0, split - span * lid_edge );
        b.side_bottom = split;
    } else {
        b.side_top = split;
        b.side_bottom = oy1;
    }
    return b;
}

auto place_furniture_box( const place_furniture_box_options &options ) -> box_layout
{
    const auto &box = options.box;
    auto flip = options.flip;
    auto facing = options.facing;
    auto joined = options.joined;
    auto wall_contacts = options.wall_contacts;
    auto depth_scale = options.depth_scale;

    auto b = box_layout( box );
    b.x0 = flip ? 1.0f - box.x1 : box.x0;
    b.x1 = flip ? 1.0f - box.x0 : box.x1;
    for( auto turn = static_cast<int>( 0 ); turn < ( facing & 3 ); ++turn ) {
        const auto x0 = static_cast<float>( 1.0f - b.y1 );
        const auto x1 = static_cast<float>( 1.0f - b.y0 );
        b.y0 = b.x0;
        b.y1 = b.x1;
        b.x0 = x0;
        b.x1 = x1;
    }
    if( ( joined & face_north ) != 0 ) {
        b.y0 = 0.0f;
    }
    if( ( joined & face_south ) != 0 ) {
        b.y1 = 1.0f;
    }
    if( ( joined & face_west ) != 0 ) {
        b.x0 = 0.0f;
    }
    if( ( joined & face_east ) != 0 ) {
        b.x1 = 1.0f;
    }
    constexpr auto gap = static_cast<float>( 0.5f - wall_half_thickness );
    if( ( wall_contacts & face_north ) != 0 ) {
        b.y0 = -gap;
    }
    if( ( wall_contacts & face_south ) != 0 ) {
        b.y1 = 1.0f + gap;
    }
    if( ( wall_contacts & face_west ) != 0 ) {
        b.x0 = -gap;
    }
    if( ( wall_contacts & face_east ) != 0 ) {
        b.x1 = 1.0f + gap;
    }
    // Reduce depth after fitting to the wall so the back stays flush in every orientation.
    const auto depth_x = static_cast<float>( ( b.x1 - b.x0 ) * depth_scale );
    const auto depth_y = static_cast<float>( ( b.y1 - b.y0 ) * depth_scale );
    switch( facing & 3 ) {
        case 0:
            b.y1 = b.y0 + depth_y;
            break;
        case 1:
            b.x0 = b.x1 - depth_x;
            break;
        case 2:
            b.y0 = b.y1 - depth_y;
            break;
        case 3:
            b.x1 = b.x0 + depth_x;
            break;
    }
    return b;
}

auto furniture_wall_facing( bool row_x, bool row_y, uint8_t walls ) -> int
{
    if( !row_y ) {
        if( ( walls & face_north ) != 0 ) {
            return 0;
        }
        if( ( walls & face_south ) != 0 ) {
            return 2;
        }
    }
    if( !row_x ) {
        if( ( walls & face_east ) != 0 ) {
            return 1;
        }
        if( ( walls & face_west ) != 0 ) {
            return 3;
        }
    }
    return 0;
}

auto trim_wall_spur( uint8_t connections, uint8_t indoor_sides,
                     uint8_t neighbour_connections ) -> bool
{
    constexpr auto all = uint8_t( face_north | face_south | face_west | face_east );
    if( connections == 0 || ( connections & ( connections - 1 ) ) != 0 ||
        ( indoor_sides & all ) != ( all & ~connections ) ) {
        return false;
    }
    const auto opposite = uint8_t( connections == face_north ? face_south :
                                   connections == face_south ? face_north :
                                   connections == face_west ? face_east : face_west );
    const auto corner = uint8_t( neighbour_connections & ~opposite );
    const auto horizontal = static_cast<bool>( ( corner & ( face_west | face_east ) ) == face_west ||
                            ( corner & ( face_west | face_east ) ) == face_east );
    const auto vertical = static_cast<bool>( ( corner & ( face_north | face_south ) ) == face_north ||
                          ( corner & ( face_north | face_south ) ) == face_south );
    return ( neighbour_connections & opposite ) != 0 && horizontal && vertical;
}

auto wall_footprint( uint8_t connections ) -> std::array<bool, 9>
{
    // A straight wall, including its last tile, spans the whole tile. Only actual
    // turns and junctions stop an arm at the centre to avoid projecting past a corner.
    if( connections == 0 || connections == face_west || connections == face_east ) {
        connections = face_west | face_east;
    } else if( connections == face_north || connections == face_south ) {
        connections = face_north | face_south;
    }
    return {{
            false, ( connections & face_north ) != 0, false,
            ( connections & face_west ) != 0, true, ( connections & face_east ) != 0,
            false, ( connections & face_south ) != 0, false
        }};
}

namespace
{

auto hides_side( const shape &self, const shape &other ) -> bool
{
    const auto solid = static_cast<bool>( other.kind == shape_kind::block ||
                                          other.kind == shape_kind::prism );
    return solid && other.height >= self.height;
}

// A cell has a ceiling unless the level above is open air: a floor, a wall, a window or door
// frame, a shrub on a roof or solid furniture all stand on (or are) something that closes it.
auto ceiling_under( const shape &up ) -> uint8_t
{ return up.kind != shape_kind::empty ? face_ceiling : 0; }

} // namespace

auto visible_faces( const shape &self, const neighbourhood &n ) -> uint8_t
{
    auto faces = uint8_t( 0 );
    switch( self.kind ) {
        case shape_kind::empty:
        case shape_kind::upright:

            // Trees reach into the level above (their top is drawn there), so no ceiling.
            return 0;
        case shape_kind::pane:
            return ceiling_under( n.up );
        case shape_kind::floor:
            // Tops are kept even under a block: the levels above may be cut away.
            return  static_cast<uint8_t>( face_top | ceiling_under( n.up ) );
        case shape_kind::block:
        case shape_kind::prism:
            break;
    }
    faces |= face_top;

    faces |= ceiling_under( n.up );
    if( !hides_side( self, n.north ) ) {
        faces |= face_north;
    }
    if( !hides_side( self, n.south ) ) {
        faces |= face_south;
    }
    if( !hides_side( self, n.west ) ) {
        faces |= face_west;
    }
    if( !hides_side( self, n.east ) ) {
        faces |= face_east;
    }
    return faces;
}

} // namespace r3d
