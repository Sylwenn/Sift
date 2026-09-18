#include <iostream>
#include <string>

#include "src/analysis/analysis_runner.hpp"
#include "src/analysis/feature_extraction.hpp"
#include "src/config/app_config.hpp"
#include "src/config/cli_options.hpp"
#include "src/ingestion/parquet_event_reader.hpp"
#include "src/reporting/text_reporter.hpp"

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

    const auto ingest_result = sift::ingest_parquet(options.parquet_path, config.schema);
    if (!ingest_result.ok()) {
        std::cerr << ingest_result.status().ToString() << '\n';
        return 1;
    }

    const auto& [schema, events] = *ingest_result;
    const auto snapshots = sift::extract_features(events);
    const std::string schema_text = schema->ToString();

    switch (config.analysis.mode) {
        case sift::AnalysisMode::native: {
            const sift::AnalysisInput input{events.events(), snapshots};
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
        case sift::AnalysisMode::jev:
            std::cerr << "Jev analysis support was not built "
                         "(rebuild with -DSIFT_ENABLE_JEV=ON)\n";
            return 1;
    }
    return 0;
}
