#pragma once
#ifndef CATA_SRC_R3D_ASSET_LOADER_H
#define CATA_SRC_R3D_ASSET_LOADER_H

#if defined(__APPLE__) && !defined(_LIBCPP_ENABLE_EXPERIMENTAL)
#define _LIBCPP_ENABLE_EXPERIMENTAL
#endif

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <future>
#include <istream>
#include <map>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>

#include "r3d_presentation_profiles.h"
#include "r3d_vox.h"

namespace r3d
{

struct prepared_vox_asset {
    vox_model model;
    vox_surface surface;
};

struct prepared_assets {
    std::map<std::string, prepared_vox_asset> vehicle_parts;
    std::map<std::string, prepared_vox_asset> items;
    presentation_profile_map profiles;
    std::vector<std::string> warnings;
};

using asset_bytes = std::expected<std::vector<uint8_t>, std::string>;
using asset_file_reader = std::function < auto( const std::filesystem::path & ) -> asset_bytes >;

/// Checked, bounded reads: EOF is normal; I/O failures must not publish a truncated asset.
auto read_asset_stream( std::istream &input, size_t maximum = 64 * 1024 * 1024 ) -> asset_bytes;
auto read_asset_file( const std::filesystem::path &path ) -> asset_bytes;

struct asset_load_options {
    const std::filesystem::path &root;
    std::stop_token stop{};
    asset_file_reader read_file = read_asset_file;
};

/// CPU-only preparation. Each failing file is isolated; no SDL, Lua or live map access.
auto prepare_assets( const asset_load_options &options ) -> prepared_assets;

using asset_launch = std::function < auto( std::filesystem::path, std::stop_token ) ->
                     std::future<prepared_assets >>;

/// One asynchronous preparation at a time. Reload/cancel never waits on the render thread;
/// completed results from superseded requests are discarded before publication.
class asset_preparer
{
    public:
        explicit asset_preparer( asset_launch launch = {} );
        auto request( const std::filesystem::path &root ) -> void;
        auto cancel() -> void;
        auto poll() -> std::optional<prepared_assets>;
        auto busy() const -> bool;

    private:
        auto start_pending() -> void;
        asset_launch launch_;
        std::optional<std::filesystem::path> pending_;
        std::future<prepared_assets> active_;
        std::stop_source stop_;
        bool discard_ = false;
};

#if defined(TILES)
/// Called by SDL's idle event pump so prepared assets appear without spending a game turn.
auto refresh_pending_assets() -> void;
#endif

} // namespace r3d
#endif
