#include "r3d_vehicle_details.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace r3d
{
auto rotor_pieces( const rotor_geometry_options &options ) -> std::vector<fixture_piece>
{
    if( options.diameter <= 0.0f ) { return {}; }
    auto result = std::vector<fixture_piece> {
        { .box = { .center = { 0.5f, 0.5f, 0.10f }, .size = { 0.12f, 0.12f, 0.20f } }, .detail = fixture_detail::leg },
        { .box = { .center = { 0.5f, 0.5f, 0.22f }, .size = { 0.32f, 0.32f, 0.08f } } }
    };
    const auto count = std::clamp( options.blades, 2, 6 );
    const auto radius = options.diameter * 0.5f;
    const auto length = options.broken ? std::min( radius * 0.15f, 0.45f ) : std::max( radius - 0.14f,
                        0.01f );
    const auto width = std::clamp( options.diameter * 0.025f, 0.08f, 0.28f );
    for( auto blade = 0; blade < count; ++blade ) {
        const auto angle = std::numbers::pi_v<float> *( 0.125f + 2.0f * blade / count );
        const auto offset = 0.14f + length * 0.5f;
        result.push_back( { .box = { .center = { 0.5f + offset * std::cos( angle ), 0.5f + offset * std::sin( angle ), 0.23f }, .size = { length, width, 0.035f }, .yaw = angle }, .finish = fixture_finish::dark } );
    }
    return result;
}

auto roof_crown( const float normalized_right ) -> float
{
    const auto across = std::clamp( normalized_right, -1.0f, 1.0f );
    return 0.06f * ( 1.0f - across * across );
}
} // namespace r3d
