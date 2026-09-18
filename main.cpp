#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "src/analysis/analysis_runner.hpp"
#include "src/analysis/feature_extraction.hpp"
#include "src/config/app_config.hpp"
#include "src/config/cli_options.hpp"
#include "src/ingestion/parquet_event_reader.hpp"
#include "src/reporting/text_reporter.hpp"

#ifdef SIFT_ENABLE_JEV
#include "src/jev/jev_analysis.hpp"
#endif

namespace {

#ifdef SIFT_ENABLE_JEV
int run_jev(const sift::AppConfig& config, const std::string& schema_text,
            const std::vector<sift::EntityFeatureSnapshot>& snapshots,
            std::size_t event_count, std::span<const sift::Event> events) {
    auto api_key = sift::read_typesafe_api_key();
    if (!api_key.ok()) {
        std::cerr << api_key.status().ToString() << '\n';
        return 1;
    }

    sift::JevClientOptions options;
    options.api_key = *api_key;
    options.base_url = config.analysis.jev.base_url;
    options.model = config.analysis.jev.model;
    options.timeout_ms = config.analysis.jev.timeout_ms;
    options.max_retries = config.analysis.jev.max_retries;

    const sift::CurlJevTransport transport;
    const sift::JevAnalysisBackend backend{sift::JevClient{options, transport}};
    const sift::AnalysisInput input{events, snapshots};
    auto result = sift::run_analysis(sift::AnalysisMode::jev, input, &backend);
    if (!result.ok()) {
        std::cerr << result.status().ToString() << '\n';
        return 1;
    }
    std::cout << sift::render_jev_report(schema_text, *result, event_count);
    return 0;
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
    const auto options_result = sift::parse_cli(argc, argv);
    if (!options_result.ok()) {
        std::cerr << sift::kUsage;
        return 2;
    }
    const auto options = *options_result;

    auto config_result = sift::load_app_config(options.config_path);
    if (!config_result.ok()) {
        std::cerr << config_result.status().ToString() << '\n';
        return 1;
    }
    auto config = *config_result;
    if (options.mode_override) {
        config.analysis.mode = *options.mode_override;
    }

#ifndef SIFT_ENABLE_JEV
    if (config.analysis.mode == sift::AnalysisMode::jev) {
        std::cerr << "Jev analysis support was not built "
                     "(rebuild with -DSIFT_ENABLE_JEV=ON)\n";
        return 1;
    }
#endif

    const auto ingest_result = sift::ingest_parquet(options.parquet_path, config.schema);
    if (!ingest_result.ok()) {
        std::cerr << ingest_result.status().ToString() << '\n';
        return 1;
    }

    const auto& [schema, events] = *ingest_result;
    const auto snapshots = sift::extract_features(events);
    const std::string schema_text = schema->ToString();
    const sift::AnalysisInput input{events.events(), snapshots};

    switch (config.analysis.mode) {
        case sift::AnalysisMode::native: {
            const auto analysis =
                sift::run_analysis(sift::AnalysisMode::native, input, nullptr);
            if (!analysis.ok()) {
                std::cerr << analysis.status().ToString() << '\n';
                return 1;
            }
            std::cout << sift::render_native_report(schema_text, snapshots,
                                                    events.size());
            return 0;
        }
        case sift::AnalysisMode::jev: {
#ifdef SIFT_ENABLE_JEV
            return run_jev(config, schema_text, snapshots, events.size(),
                           events.events());
#else
            std::cerr << "Jev analysis support was not built "
                         "(rebuild with -DSIFT_ENABLE_JEV=ON)\n";
            return 1;
#endif
        }
    }
    return 0;
}
