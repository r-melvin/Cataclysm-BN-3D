#include "r3d_asset_loader.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <sstream>
#include <utility>

namespace r3d
{
namespace
{

struct directory_load_options {
    std::filesystem::path directory;
    std::map<std::string, prepared_vox_asset> &models;
    std::vector<std::string> &warnings;
    const asset_load_options &load;
};

auto prepare_directory( const directory_load_options &options ) -> void
{
    const auto warn = [&]( const std::filesystem::path & path, const std::string & message ) {
        options.warnings.push_back( path.string() + ": " + message );
    };
    auto paths = std::vector<std::filesystem::path> {};
    try {
        if( !std::filesystem::exists( options.directory ) ) { return; }
        for( const auto &entry : std::filesystem::directory_iterator( options.directory ) ) {
            if( entry.path().extension() == ".vox" ) { paths.push_back( entry.path() ); }
        }
    } catch( const std::exception &error ) {
        warn( options.directory, error.what() );
    }
    std::ranges::sort( paths );
    for( const auto &path : paths ) {
        if( options.load.stop.stop_requested() ) { return; }
        try {
            if( !std::filesystem::is_regular_file( path ) ) { continue; }
            const auto bytes = options.load.read_file( path );
            if( !bytes ) { warn( path, bytes.error() ); continue; }
            auto model = read_vox_model( *bytes );
            if( !model ) { warn( path, model.error() ); continue; }
            auto surface = extract_vox_surface( *model );
            if( !surface ) { warn( path, surface.error() ); continue; }
            if( surface->faces.empty() ) { warn( path, "voxel model is empty" ); continue; }
            options.models.emplace( path.stem().string(), prepared_vox_asset{
                .model = std::move( *model ), .surface = std::move( *surface ) } );
        } catch( const std::exception &error ) {
            warn( path, error.what() );
        }
    }
}

auto launch_preparation( std::filesystem::path root, std::stop_token stop ) ->
std::future<prepared_assets>
{
    return std::async( std::launch::async, [root = std::move( root ), stop]() {
        return prepare_assets( { .root = root, .stop = stop } );
    } );
}

} // namespace

auto read_asset_stream( std::istream &input, size_t maximum ) -> asset_bytes
{
    auto bytes = std::vector<uint8_t> {};
    auto buffer = std::array<char, 8192> {};
    try {
        for( ;; ) {
            input.read( buffer.data(), buffer.size() );
            const auto count = static_cast<size_t>( input.gcount() );
            if( count > maximum - bytes.size() ) { return std::unexpected( "asset exceeds size limit" ); }
            bytes.insert( bytes.end(), buffer.data(), buffer.data() + count );
            if( input.bad() || ( input.fail() && !input.eof() ) ) {
                return std::unexpected( "asset read failed" );
            }
            if( input.eof() ) { return bytes; }
        }
    } catch( const std::exception &error ) {
        return std::unexpected( "asset read failed: " + std::string( error.what() ) );
    }
}

auto read_asset_file( const std::filesystem::path &path ) -> asset_bytes
{
    auto input = std::ifstream( path, std::ios::binary );
    if( !input.is_open() ) { return std::unexpected( "cannot open asset" ); }
    return read_asset_stream( input );
}

auto prepare_assets( const asset_load_options &options ) -> prepared_assets
{
    auto result = prepared_assets{};
    prepare_directory( { .directory = options.root / "vehicle_parts", .models = result.vehicle_parts,
                         .warnings = result.warnings, .load = options } );
    prepare_directory( { .directory = options.root / "items", .models = result.items,
                         .warnings = result.warnings, .load = options } );
    if( options.stop.stop_requested() ) { return result; }
    const auto profile_path = options.root / "presentation_profiles.json";
    try {
        if( std::filesystem::exists( profile_path ) ) {
            const auto bytes = options.read_file( profile_path );
            if( !bytes ) { result.warnings.push_back( profile_path.string() + ": " + bytes.error() ); }
            else {
                auto input = std::istringstream( std::string( bytes->begin(), bytes->end() ) );
                result.profiles = load_presentation_profiles( input );
            }
        }
    } catch( const std::exception &error ) {
        result.warnings.push_back( profile_path.string() + ": " + error.what() );
    }
    return result;
}

asset_preparer::asset_preparer( asset_launch launch ) :
    launch_( launch ? std::move( launch ) : launch_preparation ) {}

auto asset_preparer::request( const std::filesystem::path &root ) -> void
{
    pending_ = root;
    if( active_.valid() ) { discard_ = true; stop_.request_stop(); }
    else { start_pending(); }
}

auto asset_preparer::cancel() -> void
{
    pending_.reset();
    discard_ = true;
    stop_.request_stop();
}

auto asset_preparer::start_pending() -> void
{
    if( !pending_ ) { return; }
    auto root = std::move( *pending_ );
    pending_.reset();
    stop_ = std::stop_source{};
    discard_ = false;
    try {
        active_ = launch_( std::move( root ), stop_.get_token() );
    } catch( const std::exception &error ) {
        auto ready = std::promise<prepared_assets> {};
        active_ = ready.get_future();
        ready.set_value( { .vehicle_parts = {}, .items = {}, .profiles = {},
                           .warnings = { "cannot start asset preparation: " + std::string( error.what() ) } } );
    }
}

auto asset_preparer::poll() -> std::optional<prepared_assets>
{
    if( !active_.valid() ||
        active_.wait_for( std::chrono::seconds( 0 ) ) != std::future_status::ready ) {
        return std::nullopt;
    }
    auto result = prepared_assets{};
    try { result = active_.get(); }
    catch( const std::exception &error ) { result.warnings.push_back( error.what() ); }
    if( discard_ ) { start_pending(); return std::nullopt; }
    return result;
}

auto asset_preparer::busy() const -> bool { return active_.valid() || pending_.has_value(); }

} // namespace r3d
