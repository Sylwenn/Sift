//
// Created by rain on 07/08/26.
//

#include <iostream>
#include <string>

#include "src/analysis/daily_exports.hpp"
#include "src/ingestion/column_mapping.hpp"
#include "src/ingestion/parquet_event_reader.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: Sift <events.parquet> [config.yaml]\n";
        return 2;
    }

    const std::string config_path = argc == 3 ? argv[2] : "sift.yaml";
    const auto mapping_result = sift::load_mapping(config_path);
    if (!mapping_result.ok()) {
        std::cerr << mapping_result.status().ToString() << '\n';
        return 1;
    }
    const auto mapping = *mapping_result;

    const auto ingest_result = sift::ingest_parquet(argv[1], mapping);
    if (!ingest_result.ok()) {
        std::cerr << ingest_result.status().ToString() << '\n';
        return 1;
    }

    const auto& [schema, events] = *ingest_result;
    std::cout << schema->ToString() << '\n';
    std::cout << "events: " << events.size() << '\n';
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
