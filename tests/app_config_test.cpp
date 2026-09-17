//
// Created by rain on 17/09/26.
//
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "src/config/app_config.hpp"
#include "tests/test_support.hpp"

namespace {
bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

void write_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path);
    output << contents;
}
}

int main(int argc, char* argv[]) {
    unsetenv("SIFT_TYPESAFE_BASE_URL");
    const std::filesystem::path tmp =
        argc > 1 ? std::filesystem::path{argv[1]}
                 : std::filesystem::temp_directory_path() / "sift_app_config_test";
    std::filesystem::create_directories(tmp);

    const auto mapping_only = tmp / "mapping_only.yaml";
    write_file(mapping_only,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n");
    const auto default_result = sift::load_app_config(mapping_only.string());
    CHECK(default_result.ok());
    if (default_result.ok()) {
        CHECK_EQ(default_result->schema.timestamp_column, std::string{"occurred_at"});
        CHECK_EQ(default_result->schema.event_type_column, std::string{"action"});
        CHECK_EQ(default_result->schema.primary_entity_id_column, std::string{"user_id"});
        CHECK(default_result->analysis.mode == sift::AnalysisMode::native);
        CHECK_EQ(default_result->analysis.jev.model, std::string{"jev-1.13.0"});
    }

    const auto native_mode = tmp / "native_mode.yaml";
    write_file(native_mode,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n"
               "analysis:\n  mode: native\n");
    const auto native_result = sift::load_app_config(native_mode.string());
    CHECK(native_result.ok());
    if (native_result.ok()) {
        CHECK(native_result->analysis.mode == sift::AnalysisMode::native);
    }

    const auto jev_mode = tmp / "jev_mode.yaml";
    write_file(jev_mode,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n"
               "analysis:\n  mode: jev\n  jev:\n    model: jev-1.13.0\n"
               "    timeout_ms: 2500\n    max_retries: 1\n");
    const auto jev_result = sift::load_app_config(jev_mode.string());
    CHECK(jev_result.ok());
    if (jev_result.ok()) {
        CHECK(jev_result->analysis.mode == sift::AnalysisMode::jev);
        CHECK_EQ(jev_result->analysis.jev.timeout_ms, 2500L);
        CHECK_EQ(jev_result->analysis.jev.max_retries, 1);
    }

    const auto unknown_mode = tmp / "unknown_mode.yaml";
    write_file(unknown_mode,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n"
               "analysis:\n  mode: hybrid\n");
    const auto unknown_result = sift::load_app_config(unknown_mode.string());
    CHECK(!unknown_result.ok());
    CHECK(contains(unknown_result.status().message(), "analysis mode must be"));

    const auto bad_analysis = tmp / "bad_analysis.yaml";
    write_file(bad_analysis,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n"
               "analysis: native\n");
    const auto bad_analysis_result = sift::load_app_config(bad_analysis.string());
    CHECK(!bad_analysis_result.ok());
    CHECK(contains(bad_analysis_result.status().message(),
                   "analysis must be a mapping"));

    const auto missing_mapping = tmp / "missing_mapping.yaml";
    write_file(missing_mapping, "timestamp: occurred_at\nevent_type: action\n");
    const auto missing_result = sift::load_app_config(missing_mapping.string());
    CHECK(!missing_result.ok());
    CHECK(contains(missing_result.status().message(),
                   "config must define timestamp, event_type, and entity_id"));

    const auto absent = sift::load_app_config((tmp / "absent.yaml").string());
    CHECK(!absent.ok());
    CHECK(contains(absent.status().message(), "failed to read config"));

    const auto mode_parse = sift::parse_analysis_mode("jev");
    CHECK(mode_parse.ok());
    if (mode_parse.ok()) {
        CHECK(*mode_parse == sift::AnalysisMode::jev);
    }
    CHECK(!sift::parse_analysis_mode("nope").ok());
    CHECK_EQ(std::string{sift::to_string(sift::AnalysisMode::native)},
             std::string{"native"});
    CHECK_EQ(std::string{sift::to_string(sift::AnalysisMode::jev)}, std::string{"jev"});

    const auto env_override = tmp / "env_override.yaml";
    write_file(env_override,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n");
    setenv("SIFT_TYPESAFE_BASE_URL", "http://localhost:9999", 1);
    const auto env_result = sift::load_app_config(env_override.string());
    unsetenv("SIFT_TYPESAFE_BASE_URL");
    CHECK(env_result.ok());
    if (env_result.ok()) {
        CHECK_EQ(env_result->analysis.jev.base_url,
                 std::string{"http://localhost:9999"});
    }

    return sift_test::failures == 0 ? 0 : 1;
}
