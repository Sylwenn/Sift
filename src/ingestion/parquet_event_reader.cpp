//
// Created by rain on 07/08/26.
//
#include "src/ingestion/parquet_event_reader.hpp"

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/memory_pool.h>
#include <parquet/arrow/reader.h>

#include <chrono>
#include <utility>
#include <vector>

namespace sift {
namespace {
arrow::Result<std::vector<Event>> read_events(parquet::arrow::FileReader& reader,
                                              const SchemaMapping& mapping) {
    auto table_result = reader.ReadTable();
    if (!table_result.ok()) {
        return table_result.status();
    }
    auto combined_result = (*table_result)->CombineChunks();
    if (!combined_result.ok()) {
        return combined_result.status();
    }

    const auto table = *combined_result;
    std::vector<Event> events;
    events.reserve(table->num_rows());
    if (table->num_rows() == 0) {
        return events;
    }

    const auto timestamps = std::static_pointer_cast<arrow::TimestampArray>(
        table->GetColumnByName(mapping.timestamp_column)->chunk(0));
    const auto actions = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName(mapping.event_type_column)->chunk(0));
    const auto users = std::static_pointer_cast<arrow::StringArray>(
        table->GetColumnByName(mapping.primary_entity_id_column)->chunk(0));

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
}  // namespace

arrow::Result<ParquetIngestResult> ingest_parquet(const std::string& path,
                                                  const SchemaMapping& mapping) {
    auto file_result = arrow::io::ReadableFile::Open(path);
    if (!file_result.ok()) {
        return file_result.status();
    }

    auto parq_file = parquet::arrow::OpenFile(*file_result, arrow::default_memory_pool());
    if (!parq_file.ok()) {
        return parq_file.status();
    }
    auto reader = std::move(*parq_file);

    std::shared_ptr<arrow::Schema> schema;
    auto reader_file = reader->GetSchema(&schema);
    if (!reader_file.ok()) {
        return reader_file;
    }

    const auto validation = validate_schema(*schema, mapping);
    if (!validation.ok()) {
        return validation;
    }

    auto events_result = read_events(*reader, mapping);
    if (!events_result.ok()) {
        return events_result.status();
    }

    return ParquetIngestResult{schema, EventStore{std::move(*events_result)}};
}
}
