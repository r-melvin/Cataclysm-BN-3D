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

} // namespace r3d
