//
// Created by rain on 17/09/26.
//
#include <string>
#include <vector>

#include "src/config/cli_options.hpp"
#include "tests/test_support.hpp"

namespace {
bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

sift::CliOptions parse(std::vector<std::string> args, arrow::Status* status) {
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("Sift"));
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    const auto result = sift::parse_cli(static_cast<int>(argv.size()), argv.data());
    if (!result.ok()) {
        *status = result.status();
        return {};
    }
    return *result;
}
}

int main() {
    arrow::Status status = arrow::Status::OK();

    const auto defaults = parse({"events.parquet"}, &status);
    CHECK(status.ok());
    CHECK_EQ(defaults.parquet_path, std::string{"events.parquet"});
    CHECK_EQ(defaults.config_path, std::string{"sift.yaml"});
    CHECK(!defaults.mode_override.has_value());

    const auto with_config = parse({"events.parquet", "custom.yaml"}, &status);
    CHECK(status.ok());
    CHECK_EQ(with_config.config_path, std::string{"custom.yaml"});

    const auto flag_before = parse({"--analysis-mode", "jev", "events.parquet"}, &status);
    CHECK(status.ok());
    CHECK(flag_before.mode_override.has_value());
    CHECK(*flag_before.mode_override == sift::AnalysisMode::jev);
    CHECK_EQ(flag_before.parquet_path, std::string{"events.parquet"});

    const auto flag_after = parse({"events.parquet", "--analysis-mode", "native"},
                                  &status);
    CHECK(status.ok());
    CHECK(flag_after.mode_override.has_value());
    CHECK(*flag_after.mode_override == sift::AnalysisMode::native);

    const auto flag_equals = parse({"events.parquet", "--analysis-mode=jev"}, &status);
    CHECK(status.ok());
    CHECK_EQ(flag_equals.parquet_path, std::string{"events.parquet"});
    CHECK(flag_equals.mode_override.has_value());
    CHECK(*flag_equals.mode_override == sift::AnalysisMode::jev);

    const auto missing_value = parse({"--analysis-mode"}, &status);
    CHECK(!status.ok());
    CHECK(contains(status.message(), "requires a value"));

    const auto unknown_mode = parse({"--analysis-mode", "hybrid", "events.parquet"},
                                    &status);
    CHECK(!status.ok());
    CHECK(contains(status.message(), "analysis mode must be"));

    const auto unknown_flag = parse({"--verbose", "events.parquet"}, &status);
    CHECK(!status.ok());
    CHECK(contains(status.message(), "unknown option"));

    const auto no_path = parse({}, &status);
    CHECK(!status.ok());
    CHECK(contains(status.message(), "expected a Parquet path"));

    const auto too_many = parse({"a.parquet", "b.yaml", "c.yaml"}, &status);
    CHECK(!status.ok());
    CHECK(contains(status.message(), "expected a Parquet path"));

    return sift_test::failures == 0 ? 0 : 1;
}
