#include "r3d_sprite_voxels.h"

#include <algorithm>

namespace r3d
{

auto orient_sprite_alpha( const orient_sprite_alpha_options &options ) -> sprite_alpha_image
{
    const auto width = options.width;
    const auto height = options.height;
    if( width <= 0 || height <= 0 || options.alpha.size() < static_cast<size_t>( width * height ) ) {
        return {};
    }
    const auto turns = options.quarter_turns & 3;
    const auto output_width = ( turns & 1 ) == 0 ? width : height;
    const auto output_height = ( turns & 1 ) == 0 ? height : width;
    auto output = sprite_alpha_image{ .width = output_width, .height = output_height,
                                      .pixels = std::vector<uint8_t>( static_cast<size_t>( output_width * output_height ) ) };
    for( auto y = 0; y < output_height; ++y ) {
        for( auto x = 0; x < output_width; ++x ) {
            const auto gx = ( static_cast<float>( x ) + 0.5f ) / static_cast<float>( output_width );
            const auto gy = ( static_cast<float>( y ) + 0.5f ) / static_cast<float>( output_height );
            auto source_x = 0.0f;
            auto source_y = 0.0f;
            switch( turns ) {
                case 0:
                    source_x = gx;
                    source_y = gy;
                    break;
                case 1:
                    source_x = gy;
                    source_y = 1.0f - gx;
                    break;
                case 2:
                    source_x = 1.0f - gx;
                    source_y = 1.0f - gy;
                    break;
                case 3:
                    source_x = 1.0f - gy;
                    source_y = gx;
                    break;
            }
            if( options.flip ) {
                source_x = 1.0f - source_x;
            }
            const auto sx = std::clamp( static_cast<int>( source_x * static_cast<float>( width ) ), 0,
                                        width - 1 );
            const auto sy = std::clamp( static_cast<int>( source_y * static_cast<float>( height ) ), 0,
                                        height - 1 );
            output.pixels[static_cast<size_t>( y * output_width + x )] =
                      options.alpha[static_cast<size_t>( sy * width + sx )];
        }
    }
    return output;
}

auto voxelize_sprite_alpha( int width, int height, std::span<const uint8_t> alpha )
-> sprite_voxel_surface
{
    auto surface = sprite_voxel_surface{ .width = width, .height = height, .faces = {} };
    if( width <= 0 || height <= 0 || alpha.size() < static_cast<size_t>( width * height ) ) {
        return surface;
    }
    const auto occupied = [width, height, alpha]( int x, int y ) {
        return x >= 0 && x < width && y >= 0 && y < height &&
               alpha[static_cast<size_t>( y * width + x )] != 0;
    };
    const auto add_face = [&surface]( sprite_voxel_face face, float x0, float z0, float x1, float z1,
    int sample_x, int sample_y ) {
        surface.faces.push_back( { .face = face, .x0 = x0, .z0 = z0, .x1 = x1, .z1 = z1,
                                   .sample_x = sample_x, .sample_y = sample_y } );
    };

    for( auto y = 0; y < height; ++y ) {
        auto x = 0;
        while( x < width ) {
            if( !occupied( x, y ) ) {
                ++x;
                continue;
            }
            const auto begin = x;
            while( x < width && occupied( x, y ) ) {
                ++x;
            }
            add_face( sprite_voxel_face::front, static_cast<float>( begin ), static_cast<float>( y ),
                      static_cast<float>( x ), static_cast<float>( y + 1 ), begin, y );
            add_face( sprite_voxel_face::back, static_cast<float>( begin ), static_cast<float>( y ),
                      static_cast<float>( x ), static_cast<float>( y + 1 ), begin, y );
        }
    }

    for( auto y = 0; y < height; ++y ) {
        for( auto x = 0; x < width; ++x ) {
            if( !occupied( x, y ) ) {
                continue;
            }
            if( !occupied( x - 1, y ) ) {
                add_face( sprite_voxel_face::left, static_cast<float>( x ), static_cast<float>( y ),
                          static_cast<float>( x ), static_cast<float>( y + 1 ), x, y );
            }
            if( !occupied( x + 1, y ) ) {
                add_face( sprite_voxel_face::right, static_cast<float>( x + 1 ), static_cast<float>( y ),
                          static_cast<float>( x + 1 ), static_cast<float>( y + 1 ), x, y );
            }
            if( !occupied( x, y - 1 ) ) {
                add_face( sprite_voxel_face::top, static_cast<float>( x ), static_cast<float>( y ),
                          static_cast<float>( x + 1 ), static_cast<float>( y ), x, y );
            }
            if( !occupied( x, y + 1 ) ) {
                add_face( sprite_voxel_face::bottom, static_cast<float>( x ), static_cast<float>( y + 1 ),
                          static_cast<float>( x + 1 ), static_cast<float>( y + 1 ), x, y );
            }
        }
    }
    return surface;
}

auto generate_orthogonal_voxel_shape( const orthogonal_voxel_options &options )
-> orthogonal_voxel_shape
{
    const auto width = options.width;
    const auto height = options.height;
    if( width <= 0 || height <= 0 || options.mask.size() < static_cast<size_t>( width * height ) ) {
        return {};
    }
    auto result = orthogonal_voxel_shape{
        .size_x = width,
        .size_y = width,
        .size_z = height,
        .voxels = std::vector<uint8_t>( static_cast<size_t>( width *width * height ), uint8_t{ 0 } )
    };

    const auto is_mask_set = [width, height, mask = options.mask]( int u, int v ) -> bool {
        if( u < 0 || u >= width || v < 0 || v >= height ) {
            return false;
        }
        return mask[static_cast<size_t>( v * width + u )] != 0;
    };

    for( auto z = 0; z < height; ++z ) {
        const auto row = ( height - 1 ) - z;
        auto profile = std::vector<uint8_t>( static_cast<size_t>( width ), uint8_t{ 0 } );
        for( auto u = 0; u < width; ++u ) {
            if( is_mask_set( u, row ) && is_mask_set( width - 1 - u, row ) ) {
                profile[static_cast<size_t>( u )] = 1;
            }
        }
        for( auto y = 0; y < width; ++y ) {
            if( profile[static_cast<size_t>( y )] == 0 ) {
                continue;
            }
            for( auto x = 0; x < width; ++x ) {
                if( profile[static_cast<size_t>( x )] != 0 ) {
                    result.voxels[static_cast<size_t>( ( z * width + y ) * width + x )] = 1;
                }
            }
        }
    }

    return result;
}

auto extract_orthogonal_voxel_mesh( const orthogonal_voxel_shape &shape )
-> orthogonal_voxel_mesh
{
    const auto width = shape.size_x;
    const auto height = shape.size_z;
    if( width <= 0 || height <= 0 || shape.size_y <= 0 ) {
        return {};
    }
    auto mesh = orthogonal_voxel_mesh{ .size_x = width, .size_y = width, .size_z = height, .quads = {} };

    // 1. Top faces (+Z)
    for( auto z = 0; z < height; ++z ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * width ), uint8_t{ 0 } );
        auto count = 0;
        for( auto y = 0; y < width; ++y ) {
            for( auto x = 0; x < width; ++x ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x, y, z + 1 ) ) {
                    exposed[static_cast<size_t>( y * width + x )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto y = 0; y < width; ++y ) {
            for( auto x = 0; x < width; ) {
                if( !exposed[static_cast<size_t>( y * width + x )] ) {
                    ++x;
                    continue;
                }
                auto span_x = 1;
                while( x + span_x < width && exposed[static_cast<size_t>( y * width + x + span_x )] ) {
                    ++span_x;
                }
                auto span_y = 1;
                while( y + span_y < width ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_x; ++k ) {
                        if( !exposed[static_cast<size_t>( ( y + span_y ) * width + x + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_y;
                }
                for( auto dy = 0; dy < span_y; ++dy ) {
                    for( auto dx = 0; dx < span_x; ++dx ) {
                        exposed[static_cast<size_t>( ( y + dy ) * width + x + dx )] = 0;
                    }
                }
                const auto x0 = static_cast<float>( x ) / static_cast<float>( width );
                const auto x1 = static_cast<float>( x + span_x ) / static_cast<float>( width );
                const auto y0 = static_cast<float>( y ) / static_cast<float>( width );
                const auto y1 = static_cast<float>( y + span_y ) / static_cast<float>( width );
                const auto z_val = static_cast<float>( z + 1 ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::top,
                    .x0 = x0, .y0 = y0, .z0 = z_val,
                    .x1 = x1, .y1 = y1, .z1 = z_val,
                    .u0 = x0, .v0 = y0, .u1 = x1, .v1 = y1
                } );
                x += span_x;
            }
        }
    }

    // 2. Bottom faces (-Z)
    for( auto z = 0; z < height; ++z ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * width ), uint8_t{ 0 } );
        auto count = 0;
        for( auto y = 0; y < width; ++y ) {
            for( auto x = 0; x < width; ++x ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x, y, z - 1 ) ) {
                    exposed[static_cast<size_t>( y * width + x )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto y = 0; y < width; ++y ) {
            for( auto x = 0; x < width; ) {
                if( !exposed[static_cast<size_t>( y * width + x )] ) {
                    ++x;
                    continue;
                }
                auto span_x = 1;
                while( x + span_x < width && exposed[static_cast<size_t>( y * width + x + span_x )] ) {
                    ++span_x;
                }
                auto span_y = 1;
                while( y + span_y < width ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_x; ++k ) {
                        if( !exposed[static_cast<size_t>( ( y + span_y ) * width + x + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_y;
                }
                for( auto dy = 0; dy < span_y; ++dy ) {
                    for( auto dx = 0; dx < span_x; ++dx ) {
                        exposed[static_cast<size_t>( ( y + dy ) * width + x + dx )] = 0;
                    }
                }
                const auto x0 = static_cast<float>( x ) / static_cast<float>( width );
                const auto x1 = static_cast<float>( x + span_x ) / static_cast<float>( width );
                const auto y0 = static_cast<float>( y ) / static_cast<float>( width );
                const auto y1 = static_cast<float>( y + span_y ) / static_cast<float>( width );
                const auto z_val = static_cast<float>( z ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::bottom,
                    .x0 = x0, .y0 = y0, .z0 = z_val,
                    .x1 = x1, .y1 = y1, .z1 = z_val,
                    .u0 = x0, .v0 = y0, .u1 = x1, .v1 = y1
                } );
                x += span_x;
            }
        }
    }

    // 3. South faces (+Y)
    for( auto y = 0; y < width; ++y ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * height ), uint8_t{ 0 } );
        auto count = 0;
        for( auto z = 0; z < height; ++z ) {
            for( auto x = 0; x < width; ++x ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x, y + 1, z ) ) {
                    exposed[static_cast<size_t>( z * width + x )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto z = 0; z < height; ++z ) {
            for( auto x = 0; x < width; ) {
                if( !exposed[static_cast<size_t>( z * width + x )] ) {
                    ++x;
                    continue;
                }
                auto span_x = 1;
                while( x + span_x < width && exposed[static_cast<size_t>( z * width + x + span_x )] ) {
                    ++span_x;
                }
                auto span_z = 1;
                while( z + span_z < height ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_x; ++k ) {
                        if( !exposed[static_cast<size_t>( ( z + span_z ) * width + x + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_z;
                }
                for( auto dz = 0; dz < span_z; ++dz ) {
                    for( auto dx = 0; dx < span_x; ++dx ) {
                        exposed[static_cast<size_t>( ( z + dz ) * width + x + dx )] = 0;
                    }
                }
                const auto x0 = static_cast<float>( x ) / static_cast<float>( width );
                const auto x1 = static_cast<float>( x + span_x ) / static_cast<float>( width );
                const auto y_val = static_cast<float>( y + 1 ) / static_cast<float>( width );
                const auto z0 = static_cast<float>( z ) / static_cast<float>( height );
                const auto z1 = static_cast<float>( z + span_z ) / static_cast<float>( height );
                const auto u0 = x0;
                const auto u1 = x1;
                const auto v0 = static_cast<float>( height - ( z + span_z ) ) / static_cast<float>( height );
                const auto v1 = static_cast<float>( height - z ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::south,
                    .x0 = x0, .y0 = y_val, .z0 = z0,
                    .x1 = x1, .y1 = y_val, .z1 = z1,
                    .u0 = u0, .v0 = v0, .u1 = u1, .v1 = v1
                } );
                x += span_x;
            }
        }
    }

    // 4. North faces (-Y)
    for( auto y = 0; y < width; ++y ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * height ), uint8_t{ 0 } );
        auto count = 0;
        for( auto z = 0; z < height; ++z ) {
            for( auto x = 0; x < width; ++x ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x, y - 1, z ) ) {
                    exposed[static_cast<size_t>( z * width + x )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto z = 0; z < height; ++z ) {
            for( auto x = 0; x < width; ) {
                if( !exposed[static_cast<size_t>( z * width + x )] ) {
                    ++x;
                    continue;
                }
                auto span_x = 1;
                while( x + span_x < width && exposed[static_cast<size_t>( z * width + x + span_x )] ) {
                    ++span_x;
                }
                auto span_z = 1;
                while( z + span_z < height ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_x; ++k ) {
                        if( !exposed[static_cast<size_t>( ( z + span_z ) * width + x + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_z;
                }
                for( auto dz = 0; dz < span_z; ++dz ) {
                    for( auto dx = 0; dx < span_x; ++dx ) {
                        exposed[static_cast<size_t>( ( z + dz ) * width + x + dx )] = 0;
                    }
                }
                const auto x0 = static_cast<float>( x ) / static_cast<float>( width );
                const auto x1 = static_cast<float>( x + span_x ) / static_cast<float>( width );
                const auto y_val = static_cast<float>( y ) / static_cast<float>( width );
                const auto z0 = static_cast<float>( z ) / static_cast<float>( height );
                const auto z1 = static_cast<float>( z + span_z ) / static_cast<float>( height );
                const auto u0 = static_cast<float>( width - ( x + span_x ) ) / static_cast<float>( width );
                const auto u1 = static_cast<float>( width - x ) / static_cast<float>( width );
                const auto v0 = static_cast<float>( height - ( z + span_z ) ) / static_cast<float>( height );
                const auto v1 = static_cast<float>( height - z ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::north,
                    .x0 = x0, .y0 = y_val, .z0 = z0,
                    .x1 = x1, .y1 = y_val, .z1 = z1,
                    .u0 = u0, .v0 = v0, .u1 = u1, .v1 = v1
                } );
                x += span_x;
            }
        }
    }

    // 5. West faces (-X)
    for( auto x = 0; x < width; ++x ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * height ), uint8_t{ 0 } );
        auto count = 0;
        for( auto z = 0; z < height; ++z ) {
            for( auto y = 0; y < width; ++y ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x - 1, y, z ) ) {
                    exposed[static_cast<size_t>( z * width + y )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto z = 0; z < height; ++z ) {
            for( auto y = 0; y < width; ) {
                if( !exposed[static_cast<size_t>( z * width + y )] ) {
                    ++y;
                    continue;
                }
                auto span_y = 1;
                while( y + span_y < width && exposed[static_cast<size_t>( z * width + y + span_y )] ) {
                    ++span_y;
                }
                auto span_z = 1;
                while( z + span_z < height ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_y; ++k ) {
                        if( !exposed[static_cast<size_t>( ( z + span_z ) * width + y + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_z;
                }
                for( auto dz = 0; dz < span_z; ++dz ) {
                    for( auto dy = 0; dy < span_y; ++dy ) {
                        exposed[static_cast<size_t>( ( z + dz ) * width + y + dy )] = 0;
                    }
                }
                const auto x_val = static_cast<float>( x ) / static_cast<float>( width );
                const auto y0 = static_cast<float>( y ) / static_cast<float>( width );
                const auto y1 = static_cast<float>( y + span_y ) / static_cast<float>( width );
                const auto z0 = static_cast<float>( z ) / static_cast<float>( height );
                const auto z1 = static_cast<float>( z + span_z ) / static_cast<float>( height );
                const auto u0 = y0;
                const auto u1 = y1;
                const auto v0 = static_cast<float>( height - ( z + span_z ) ) / static_cast<float>( height );
                const auto v1 = static_cast<float>( height - z ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::west,
                    .x0 = x_val, .y0 = y0, .z0 = z0,
                    .x1 = x_val, .y1 = y1, .z1 = z1,
                    .u0 = u0, .v0 = v0, .u1 = u1, .v1 = v1
                } );
                y += span_y;
            }
        }
    }

    // 6. East faces (+X)
    for( auto x = 0; x < width; ++x ) {
        auto exposed = std::vector<uint8_t>( static_cast<size_t>( width * height ), uint8_t{ 0 } );
        auto count = 0;
        for( auto z = 0; z < height; ++z ) {
            for( auto y = 0; y < width; ++y ) {
                if( shape.is_occupied( x, y, z ) && !shape.is_occupied( x + 1, y, z ) ) {
                    exposed[static_cast<size_t>( z * width + y )] = 1;
                    ++count;
                }
            }
        }
        if( count == 0 ) {
            continue;
        }
        for( auto z = 0; z < height; ++z ) {
            for( auto y = 0; y < width; ) {
                if( !exposed[static_cast<size_t>( z * width + y )] ) {
                    ++y;
                    continue;
                }
                auto span_y = 1;
                while( y + span_y < width && exposed[static_cast<size_t>( z * width + y + span_y )] ) {
                    ++span_y;
                }
                auto span_z = 1;
                while( z + span_z < height ) {
                    auto can_extend = true;
                    for( auto k = 0; k < span_y; ++k ) {
                        if( !exposed[static_cast<size_t>( ( z + span_z ) * width + y + k )] ) {
                            can_extend = false;
                            break;
                        }
                    }
                    if( !can_extend ) {
                        break;
                    }
                    ++span_z;
                }
                for( auto dz = 0; dz < span_z; ++dz ) {
                    for( auto dy = 0; dy < span_y; ++dy ) {
                        exposed[static_cast<size_t>( ( z + dz ) * width + y + dy )] = 0;
                    }
                }
                const auto x_val = static_cast<float>( x + 1 ) / static_cast<float>( width );
                const auto y0 = static_cast<float>( y ) / static_cast<float>( width );
                const auto y1 = static_cast<float>( y + span_y ) / static_cast<float>( width );
                const auto z0 = static_cast<float>( z ) / static_cast<float>( height );
                const auto z1 = static_cast<float>( z + span_z ) / static_cast<float>( height );
                const auto u0 = static_cast<float>( width - ( y + span_y ) ) / static_cast<float>( width );
                const auto u1 = static_cast<float>( width - y ) / static_cast<float>( width );
                const auto v0 = static_cast<float>( height - ( z + span_z ) ) / static_cast<float>( height );
                const auto v1 = static_cast<float>( height - z ) / static_cast<float>( height );
                mesh.quads.push_back( {
                    .face = orthogonal_face_kind::east,
                    .x0 = x_val, .y0 = y0, .z0 = z0,
                    .x1 = x_val, .y1 = y1, .z1 = z1,
                    .u0 = u0, .v0 = v0, .u1 = u1, .v1 = v1
                } );
                y += span_y;
            }
        }
    }

    return mesh;
}

} // namespace r3d
