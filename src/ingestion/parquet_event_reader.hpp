//
// Created by rain on 07/08/26.
//

#ifndef SIFT_PARQUET_EVENT_READER_HPP
#define SIFT_PARQUET_EVENT_READER_HPP
#include <arrow/result.h>
#include <arrow/type_fwd.h>

#include <memory>
#include <string>

#include "src/ingestion/column_mapping.hpp"
#include "src/storage/event_store.hpp"

namespace sift {
struct ParquetIngestResult {
    std::shared_ptr<arrow::Schema> schema;
    EventStore events;
};

arrow::Result<ParquetIngestResult> ingest_parquet(const std::string& path,
                                                  const SchemaMapping& mapping);
}

#endif //SIFT_PARQUET_EVENT_READER_HPP
