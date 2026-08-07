#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/memory_pool.h>
#include <parquet/arrow/reader.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include "src/domain/event.hpp"
#include "src/ingestion/schema_mapping.hpp"
#include "src/ingestion/schema_validator.hpp"

arrow::Result<sift::SchemaMapping> load_mapping(const std::string& path) {
    try {
        const auto config = YAML::LoadFile(path);
        if (!config.IsMap() || !config["timestamp"] ||
            !config["event_type"] || !config["entity_id"]) {
            return arrow::Status::Invalid(
                "config must define timestamp, event_type, and entity_id");
        }
        return sift::SchemaMapping{
            config["timestamp"].as<std::string>(),
            config["event_type"].as<std::string>(),
            config["entity_id"].as<std::string>()};
    } catch (const YAML::Exception& error) {
        return arrow::Status::Invalid("failed to read config: ", error.what());
    }
}

arrow::Result<std::vector<sift::Event>> read_events(
    parquet::arrow::FileReader& reader,
    const sift::SchemaMapping& mapping) {
    auto table_result = reader.ReadTable();
    if (!table_result.ok()) {
        return table_result.status();
    }
    auto combined_result = (*table_result)->CombineChunks();
    if (!combined_result.ok()) {
        return combined_result.status();
    }

    const auto table = *combined_result;
    const auto timestamps = std::static_pointer_cast<arrow::TimestampArray>(
        table->GetColumnByName(mapping.timestamp_column)->chunk(0));
    const auto actions = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName(mapping.event_type_column)->chunk(0));
    const auto users = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName(mapping.primary_entity_id_column)->chunk(0));

    std::vector<sift::Event> events;
    events.reserve(table->num_rows());
    for (int64_t row = 0; row < table->num_rows(); ++row) {
        if (timestamps->IsNull(row) || actions->IsNull(row) || users->IsNull(row)) {
            return arrow::Status::Invalid("event column contains null at row ", row);
        }
        events.push_back({
            std::chrono::sys_time<std::chrono::milliseconds>{
                std::chrono::milliseconds(timestamps->Value(row))},
            actions->GetString(row),
            users->GetString(row)});
    }
    return events;
}

using DailyExportCounts =
    std::map<std::string, std::map<std::chrono::sys_days, std::size_t>>;

DailyExportCounts count_daily_exports(std::span<const sift::Event> events) {
    DailyExportCounts counts;
    std::map<std::string,
             std::pair<std::chrono::sys_days, std::chrono::sys_days>> spans;

    for (const auto& event : events) {
        const auto day = std::chrono::floor<std::chrono::days>(event.timestamp);
        auto [span, inserted] = spans.try_emplace(
            event.primary_entity_id, day, day);
        if (!inserted) {
            span->second.first = std::min(span->second.first, day);
            span->second.second = std::max(span->second.second, day);
        }
        auto& count = counts[event.primary_entity_id][day];
        if (event.event_type == "export") {
            ++count;
        }
    }

    for (const auto& [entity_id, span] : spans) {
        for (auto day = span.first; day <= span.second;
             day += std::chrono::days{1}) {
            counts[entity_id].try_emplace(day, 0);
        }
    }
    return counts;
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: Sift <events.parquet> [config.yaml]\n";
        return 2;
    }

    const std::string config_path = argc == 3 ? argv[2] : "sift.yaml";
    const auto mapping_result = load_mapping(config_path);
    if (!mapping_result.ok()) {
        std::cerr << mapping_result.status().ToString() << '\n';
        return 1;
    }
    const auto mapping = *mapping_result;

    auto file_result = arrow::io::ReadableFile::Open(argv[1]);

    if (!file_result.ok()) {
        std::cerr << file_result.status().ToString() << '\n';
        return 1;
    }

    auto file = *file_result;

    auto parq_file = parquet::arrow::OpenFile(file, arrow::default_memory_pool());
    if (!parq_file.ok()) {
        std::cerr << parq_file.status().ToString() << '\n';
        return 1;
    }
    auto reader = std::move(*parq_file);
    std::shared_ptr<arrow::Schema> schema;
    auto reader_file = reader->GetSchema(&schema);
    if (!reader_file.ok()) {
        std::cerr << reader_file.ToString() << '\n';
        return 1;
    }

    const auto validation = sift::validate_schema(*schema, mapping);
    if (!validation.ok()) {
        std::cerr << validation.ToString() << '\n';
        return 1;
    }

    auto events_result = read_events(*reader, mapping);
    if (!events_result.ok()) {
        std::cerr << events_result.status().ToString() << '\n';
        return 1;
    }

    std::cout << schema->ToString() << '\n';
    std::cout << "events: " << events_result->size() << '\n';
    if (!events_result->empty()) {
        const auto counts = count_daily_exports(*events_result);
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
