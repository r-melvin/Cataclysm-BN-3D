#pragma once
#ifndef CATA_SRC_WALL_EDGES_H
#define CATA_SRC_WALL_EDGES_H

#include <optional>

#include "coordinates.h"
#include "map/mapbuffer.h"
#include "wall_edge_dimensions.h"

class map;
class mapbuffer;
struct ter_t;

namespace wall_edges
{

enum class trace_kind { movement, vision, projectile };

struct barrier {
    tripoint_abs_ms tile;
    double fraction = 0.0;
};

struct trace_options {
    mapbuffer &buffer;
    tripoint_abs_ms from;
    tripoint_abs_ms to;
    trace_kind kind = trace_kind::movement;
    double padding = 0.0;
    mapbuffer_lookup_options lookup{};
};

/// Edge collision follows the enabled thin-wall mode; non-tiles builds retain native cells.
auto enabled() -> bool;
/// Publish backend capability on renderer creation; logic-only simulations need no backend.
auto set_rendering_available( bool available ) -> void;
/// Shared renderer/physics door classification, excluding hatches and gate controls.
auto is_door_terrain( const ter_t &terrain ) -> bool;
/// Native wall/opening terrain whose collision is represented by an edge, not a filled cell.
auto is_edge_terrain( const ter_t &terrain ) -> bool;
/// Preserve terrain state/materials while making the empty portion of wall cells usable.
auto terrain_move_cost( const ter_t &terrain ) -> int;
/// First physical boundary crossed by a centre-to-centre segment on the same level.
auto trace( const trace_options &options ) -> std::optional<barrier>;

struct local_trace_options {
    const map &here;
    tripoint_bub_ms from;
    tripoint_bub_ms to;
    trace_kind kind = trace_kind::movement;
    double padding = 0.0;
};

auto trace( const local_trace_options &options ) -> std::optional<tripoint_bub_ms>;

} // namespace wall_edges
#endif // CATA_SRC_WALL_EDGES_H
