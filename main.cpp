#include <iostream>
#include <string>

#include "src/analysis/daily_exports.hpp"
#include "src/config/app_config.hpp"
#include "src/config/cli_options.hpp"
#include "src/ingestion/parquet_event_reader.hpp"

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
    std::cout << schema->ToString() << '\n';
    std::cout << "events: " << events.size() << '\n';

    switch (config.analysis.mode) {
        case sift::AnalysisMode::native: {
            if (!events.empty()) {
                const auto counts = sift::count_daily_exports(events.events());
                for (const auto& [entity_id, daily_counts] : counts) {
                    std::cout << entity_id << " daily_exports:";
                    for (const auto& [day, count] : daily_counts) {
                        std::cout << ' ' << count;
                    }
                    std::cout << '\n';
                }
            }
            return 0;
        }
        case sift::AnalysisMode::jev:
            std::cerr << "Jev analysis support was not built "
                         "(rebuild with -DSIFT_ENABLE_JEV=ON)\n";
            return 1;
    }
    return 0;
}
