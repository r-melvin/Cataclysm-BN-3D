#include "cata_utility.h"
#include "catch/catch.hpp"
#include "filesystem.h"
#include "path_info.h"
#include "r3d_asset_loader.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct asset_directory {
    std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("bn-r3d-assets-" + get_pid_string());
    asset_directory() {
        REQUIRE(std::filesystem::create_directory(root));
        std::filesystem::create_directory(root / "items");
        std::filesystem::create_directory(root / "vehicle_parts");
    }
    ~asset_directory() {
        auto error = std::error_code{};
        std::filesystem::remove_all(root, error);
    }
    auto item(const std::string& name) const -> void {
        std::filesystem::copy_file(
            PATH_INFO::datadir() + "raw/r3d/items/carbondioxide_tank.vox", root / "items" / name);
    }
};

class failing_stream_buffer: public std::stringbuf {
public:
    failing_stream_buffer(): std::stringbuf(std::string(8192, 'x')) {}
    auto underflow() -> int_type override { throw std::runtime_error("injected disk failure"); }
};
} // namespace

TEST_CASE("r3d_asset_reads_distinguish_eof_from_io_failure_and_bound_input", "[r3d][assets]") {
    auto empty = std::istringstream("");
    const auto empty_result = r3d::read_asset_stream(empty);
    REQUIRE(empty_result.has_value());
    CHECK(empty_result->empty());
    auto exact = std::istringstream("abcd");
    REQUIRE(r3d::read_asset_stream(exact, 4).has_value());
    auto oversized = std::istringstream("abcd");
    const auto limited = r3d::read_asset_stream(oversized, 3);
    REQUIRE_FALSE(limited.has_value());
    CHECK(limited.error() == "asset exceeds size limit");
    auto buffer = failing_stream_buffer{};
    auto input = std::istream(&buffer);
    const auto failed = r3d::read_asset_stream(input);
    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error() == "asset read failed");
}

TEST_CASE("r3d_asset_file_failures_do_not_skip_valid_neighbors", "[r3d][assets]") {
    const auto mode = GENERATE("malformed", "exception", "read failure");
    CAPTURE(mode);
    const auto directory = asset_directory{};
    for (const auto* name : {"a.vox", "b.vox", "c.vox"}) { directory.item(name); }
    if (mode == std::string("malformed")) {
        auto file = std::ofstream(directory.root / "items/b.vox");
        file << "not a voxel model";
    }
    const auto reader = [mode](const std::filesystem::path& path) -> r3d::asset_bytes {
        if (path.filename() == "b.vox") {
            if (mode == std::string("exception")) { throw std::runtime_error("injected failure"); }
            if (mode == std::string("read failure")) {
                return std::unexpected("asset read failed");
            }
        }
        return r3d::read_asset_file(path);
    };
    const auto result = r3d::prepare_assets({.root = directory.root, .read_file = reader});
    REQUIRE(result.items.size() == 2);
    CHECK(result.items.contains("a"));
    CHECK(result.items.contains("c"));
    CHECK_FALSE(result.items.at("a").surface.faces.empty());
    CHECK(result.items.at("a").model.cells.size() == result.items.at("c").model.cells.size());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings.front().find("b.vox") != std::string::npos);
}

TEST_CASE("r3d_asset_preparation_checks_missing_files_and_reloads_profiles", "[r3d][assets]") {
    const auto directory = asset_directory{};
    CHECK_FALSE(r3d::read_asset_file(directory.root / "absent.vox").has_value());
    directory.item("tank.vox");
    std::filesystem::copy_file(
        PATH_INFO::datadir() + "raw/r3d/vehicle_parts/wheel.vox",
        directory.root / "vehicle_parts/wheel.vox");
    const auto profile = directory.root / "presentation_profiles.json";
    {
        auto output = std::ofstream(profile);
        output << R"({"version":1,"profiles":[{"category":"item","id":"tank","size":[1,1,1]}]})";
    }
    const auto first = r3d::prepare_assets({.root = directory.root});
    CHECK(first.warnings.empty());
    CHECK(first.vehicle_parts.contains("wheel"));
    CHECK(first.profiles.at("item:tank").size[0] == Approx(1.0f));
    {
        auto output = std::ofstream(profile);
        output << R"({"version":1,"profiles":[{"category":"item","id":"tank","size":[2,1,1]}]})";
    }
    const auto reloaded = r3d::prepare_assets({.root = directory.root});
    CHECK(reloaded.profiles.at("item:tank").size[0] == Approx(2.0f));
    {
        auto output = std::ofstream(profile);
        output << R"({"version":99})";
    }
    const auto bad_profile = r3d::prepare_assets({.root = directory.root});
    CHECK(bad_profile.items.contains("tank"));
    CHECK(bad_profile.vehicle_parts.contains("wheel"));
    CHECK(bad_profile.profiles.empty());
    REQUIRE(bad_profile.warnings.size() == 1);
    CHECK(bad_profile.warnings.front().find("unsupported presentation profile version")
          != std::string::npos);
}

TEST_CASE("r3d_asset_reload_discards_stale_results_without_waiting", "[r3d][assets]") {
    auto jobs = std::vector<std::shared_ptr<std::promise<r3d::prepared_assets>>>{};
    auto roots = std::vector<std::filesystem::path>{};
    auto stops = std::vector<std::stop_token>{};
    auto loader = r3d::asset_preparer([&](std::filesystem::path root, std::stop_token stop) {
        roots.push_back(std::move(root));
        stops.push_back(stop);
        const auto job = std::make_shared<std::promise<r3d::prepared_assets>>();
        jobs.push_back(job);
        return job->get_future();
    });
    loader.request("old-tileset");
    REQUIRE(jobs.size() == 1);
    CHECK(loader.busy());
    CHECK_FALSE(loader.poll().has_value());
    loader.request("superseded-tileset");
    loader.request("current-tileset");
    CHECK(stops.front().stop_requested());
    REQUIRE(jobs.size() == 1);
    jobs.front()->set_value(
        {.vehicle_parts = {}, .items = {}, .profiles = {}, .warnings = {"stale"}});
    CHECK_FALSE(loader.poll().has_value());
    REQUIRE(jobs.size() == 2);
    CHECK(roots.back() == "current-tileset");
    CHECK_FALSE(stops.back().stop_requested());
    jobs.back()->set_value(
        {.vehicle_parts = {}, .items = {}, .profiles = {}, .warnings = {"current"}});
    const auto current = loader.poll();
    REQUIRE(current.has_value());
    CHECK(current->warnings == std::vector<std::string>{"current"});
    CHECK_FALSE(loader.busy());

    loader.request("lost-device");
    loader.cancel();
    CHECK(stops.back().stop_requested());
    loader.request("new-device");
    jobs.back()->set_exception(std::make_exception_ptr(std::runtime_error("old device result")));
    CHECK_FALSE(loader.poll().has_value());
    CHECK(roots.back() == "new-device");
    jobs.back()->set_value({});
    REQUIRE(loader.poll().has_value());
    CHECK_FALSE(loader.busy());
}

TEST_CASE("r3d_asset_worker_start_failure_keeps_fallback_available", "[r3d][assets]") {
    auto loader = r3d::asset_preparer(
        [] [[noreturn]] (std::filesystem::path /*root*/, std::stop_token /*stop*/)
            -> std::future<r3d::prepared_assets> {
            throw std::runtime_error("worker unavailable");
        });
    loader.request("assets");
    const auto result = loader.poll();
    REQUIRE(result.has_value());
    CHECK(result->items.empty());
    REQUIRE(result->warnings.size() == 1);
    CHECK(result->warnings.front().find("worker unavailable") != std::string::npos);
    CHECK_FALSE(loader.busy());
}
