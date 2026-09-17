//
// Created by rain on 17/09/26.
//

#ifndef SIFT_CLI_OPTIONS_HPP
#define SIFT_CLI_OPTIONS_HPP

#include <arrow/result.h>

#include <optional>
#include <string>

#include "src/config/app_config.hpp"

namespace sift {

inline constexpr const char* kUsage =
    "Usage: Sift <events.parquet> [config.yaml] [--analysis-mode native|jev]\n";

struct CliOptions {
    std::string parquet_path;
    std::string config_path = "sift.yaml";
    std::optional<AnalysisMode> mode_override;
};

arrow::Result<CliOptions> parse_cli(int argc, char* argv[]);

}  // namespace sift

#endif  // SIFT_CLI_OPTIONS_HPP
