//
// Created by rain on 17/09/26.
//
#include "src/config/cli_options.hpp"

#include <vector>

namespace sift {
namespace {

constexpr std::string_view kModeFlag = "--analysis-mode";
constexpr std::string_view kModeFlagPrefix = "--analysis-mode=";

arrow::Result<AnalysisMode> mode_from_value(const char* value) {
    if (value == nullptr) {
        return arrow::Status::Invalid("--analysis-mode requires a value");
    }
    return parse_analysis_mode(value);
}

}  // namespace

arrow::Result<CliOptions> parse_cli(int argc, char* argv[]) {
    CliOptions options;
    std::vector<std::string> positionals;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == kModeFlag) {
            auto mode = mode_from_value(i + 1 < argc ? argv[++i] : nullptr);
            if (!mode.ok()) {
                return mode.status();
            }
            options.mode_override = *mode;
        } else if (arg.rfind(kModeFlagPrefix, 0) == 0) {
            const auto value = arg.substr(kModeFlagPrefix.size());
            auto mode = parse_analysis_mode(value);
            if (!mode.ok()) {
                return mode.status();
            }
            options.mode_override = *mode;
        } else if (!arg.empty() && arg[0] == '-') {
            return arrow::Status::Invalid("unknown option '", arg, "'");
        } else {
            positionals.push_back(arg);
        }
    }

    if (positionals.empty() || positionals.size() > 2) {
        return arrow::Status::Invalid(
            "expected a Parquet path and an optional config path");
    }
    options.parquet_path = positionals[0];
    if (positionals.size() == 2) {
        options.config_path = positionals[1];
    }
    return options;
}

}  // namespace sift
