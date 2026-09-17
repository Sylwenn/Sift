//
// Created by rain on 17/09/26.
//
#include "src/config/app_config.hpp"

#include <yaml-cpp/yaml.h>

#include <cstdlib>

namespace sift {
namespace {

arrow::Result<AnalysisConfig> parse_analysis(const YAML::Node& root) {
    AnalysisConfig config;
    const auto analysis = root["analysis"];
    if (!analysis) {
        return config;
    }
    if (!analysis.IsMap()) {
        return arrow::Status::Invalid("analysis must be a mapping");
    }

    const auto mode = analysis["mode"];
    if (mode) {
        auto parsed = parse_analysis_mode(mode.as<std::string>());
        if (!parsed.ok()) {
            return parsed.status();
        }
        config.mode = *parsed;
    }

    const auto jev = analysis["jev"];
    if (jev) {
        if (!jev.IsMap()) {
            return arrow::Status::Invalid("analysis.jev must be a mapping");
        }
        if (jev["model"]) {
            config.jev.model = jev["model"].as<std::string>();
        }
        if (jev["base_url"]) {
            config.jev.base_url = jev["base_url"].as<std::string>();
        }
        if (jev["timeout_ms"]) {
            config.jev.timeout_ms = jev["timeout_ms"].as<long>();
        }
        if (jev["max_retries"]) {
            config.jev.max_retries = jev["max_retries"].as<int>();
        }
    }
    return config;
}

}  // namespace

std::string_view to_string(AnalysisMode mode) {
    switch (mode) {
        case AnalysisMode::native:
            return "native";
        case AnalysisMode::jev:
            return "jev";
    }
    return "unknown";
}

arrow::Result<AnalysisMode> parse_analysis_mode(const std::string& value) {
    if (value == "native") {
        return AnalysisMode::native;
    }
    if (value == "jev") {
        return AnalysisMode::jev;
    }
    return arrow::Status::Invalid(
        "analysis mode must be 'native' or 'jev', got '", value, "'");
}

arrow::Result<AppConfig> load_app_config(const std::string& path) {
    try {
        const auto root = YAML::LoadFile(path);
        auto schema = mapping_from_yaml(root);
        if (!schema.ok()) {
            return schema.status();
        }
        auto analysis = parse_analysis(root);
        if (!analysis.ok()) {
            return analysis.status();
        }
        if (const char* base = std::getenv("SIFT_TYPESAFE_BASE_URL");
            base != nullptr && *base != '\0') {
            analysis->jev.base_url = base;
        }
        return AppConfig{*schema, *analysis};
    } catch (const YAML::Exception& error) {
        return arrow::Status::Invalid("failed to read config: ", error.what());
    }
}

}  // namespace sift
