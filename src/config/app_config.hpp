//
// Created by rain on 17/09/26.
//

#ifndef SIFT_APP_CONFIG_HPP
#define SIFT_APP_CONFIG_HPP

#include <arrow/result.h>

#include <string>
#include <string_view>

#include "src/ingestion/column_mapping.hpp"

namespace sift {

enum class AnalysisMode {
    native,
    jev,
};

std::string_view to_string(AnalysisMode mode);

arrow::Result<AnalysisMode> parse_analysis_mode(const std::string& value);

struct JevConfig {
    std::string model = "jev-1.13.0";
    std::string base_url = "https://api.typesafe.ai";
    long timeout_ms = 10000;
    int max_retries = 2;
};

struct AnalysisConfig {
    AnalysisMode mode = AnalysisMode::native;
    JevConfig jev;
};

struct AppConfig {
    SchemaMapping schema;
    AnalysisConfig analysis;
};

arrow::Result<AppConfig> load_app_config(const std::string& path);

}  // namespace sift

#endif  // SIFT_APP_CONFIG_HPP
