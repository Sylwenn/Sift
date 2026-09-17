//
// Created by rain on 07/08/26.
//
#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "src/ingestion/parquet_event_reader.hpp"
#include "tests/test_support.hpp"

namespace {
const sift::SchemaMapping kMapping{"occurred_at", "action", "user_id"};

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

std::shared_ptr<arrow::Schema> canonical_schema() {
    return arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::utf8()),
        arrow::field("user_id", arrow::utf8())});
}

bool write_table(const std::shared_ptr<arrow::Table>& table, const std::string& path) {
    auto output = arrow::io::FileOutputStream::Open(path);
    if (!output.ok()) {
        return false;
    }
    const auto status = parquet::arrow::WriteTable(
        *table, arrow::default_memory_pool(), *output,
        std::max<int64_t>(table->num_rows(), 1));
    return status.ok();
}

int64_t fixture_start_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::sys_days{
                   std::chrono::year{2026} / std::chrono::January / 1}
                   .time_since_epoch())
        .count();
}

bool write_single_event(const std::filesystem::path& path, int64_t timestamp,
                        const std::string& action, const std::string& user) {
    arrow::TimestampBuilder timestamps(
        arrow::timestamp(arrow::TimeUnit::MILLI), arrow::default_memory_pool());
    arrow::StringBuilder actions(arrow::default_memory_pool());
    arrow::StringBuilder users(arrow::default_memory_pool());
    (void)timestamps.Append(timestamp);
    (void)actions.Append(action);
    (void)users.Append(user);
    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> action_array;
    std::shared_ptr<arrow::Array> user_array;
    (void)timestamps.Finish(&timestamp_array);
    (void)actions.Finish(&action_array);
    (void)users.Finish(&user_array);
    return write_table(
        arrow::Table::Make(canonical_schema(),
                           {timestamp_array, action_array, user_array}),
        path.string());
}

bool write_null_event(const std::filesystem::path& path, int64_t timestamp) {
    arrow::TimestampBuilder timestamps(
        arrow::timestamp(arrow::TimeUnit::MILLI), arrow::default_memory_pool());
    arrow::StringBuilder actions(arrow::default_memory_pool());
    arrow::StringBuilder users(arrow::default_memory_pool());
    (void)timestamps.Append(timestamp);
    (void)actions.Append("export");
    (void)users.AppendNull();
    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> action_array;
    std::shared_ptr<arrow::Array> user_array;
    (void)timestamps.Finish(&timestamp_array);
    (void)actions.Finish(&action_array);
    (void)users.Finish(&user_array);
    return write_table(
        arrow::Table::Make(canonical_schema(),
                           {timestamp_array, action_array, user_array}),
        path.string());
}

bool write_wrong_type(const std::filesystem::path& path, int64_t timestamp) {
    arrow::TimestampBuilder timestamps(
        arrow::timestamp(arrow::TimeUnit::MILLI), arrow::default_memory_pool());
    arrow::Int64Builder actions(arrow::default_memory_pool());
    arrow::StringBuilder users(arrow::default_memory_pool());
    (void)timestamps.Append(timestamp);
    (void)actions.Append(1);
    (void)users.Append("user-1");
    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> action_array;
    std::shared_ptr<arrow::Array> user_array;
    (void)timestamps.Finish(&timestamp_array);
    (void)actions.Finish(&action_array);
    (void)users.Finish(&user_array);
    const auto schema = arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::int64()),
        arrow::field("user_id", arrow::utf8())});
    return write_table(
        arrow::Table::Make(schema, {timestamp_array, action_array, user_array}),
        path.string());
}

bool write_empty(const std::filesystem::path& path) {
    arrow::TimestampBuilder timestamps(
        arrow::timestamp(arrow::TimeUnit::MILLI), arrow::default_memory_pool());
    arrow::StringBuilder actions(arrow::default_memory_pool());
    arrow::StringBuilder users(arrow::default_memory_pool());
    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> action_array;
    std::shared_ptr<arrow::Array> user_array;
    (void)timestamps.Finish(&timestamp_array);
    (void)actions.Finish(&action_array);
    (void)users.Finish(&user_array);
    return write_table(
        arrow::Table::Make(canonical_schema(),
                           {timestamp_array, action_array, user_array}),
        path.string());
}
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: parquet_event_reader_test <fixture.parquet> <tmpdir>\n";
        return 2;
    }
    const std::filesystem::path fixture_path = argv[1];
    const std::filesystem::path tmp = argv[2];
    std::filesystem::create_directories(tmp);

    const int64_t start = fixture_start_millis();

    const auto fixture = sift::ingest_parquet(fixture_path.string(), kMapping);
    CHECK(fixture.ok());
    if (fixture.ok()) {
        CHECK(fixture->schema != nullptr);
        CHECK_EQ(fixture->schema->num_fields(), 3);
        CHECK_EQ(fixture->events.size(), std::size_t{116});
        CHECK(!fixture->events.empty());
        const auto& events = fixture->events.events();
        CHECK_EQ(events.front().timestamp.time_since_epoch().count(), start);
        CHECK_EQ(events.front().event_type, std::string{"login"});
        CHECK_EQ(events.front().primary_entity_id, std::string{"user-142"});
        CHECK_EQ(events.back().timestamp.time_since_epoch().count(),
                 start + 9 * 86'400'000LL + 3'840'000LL);
        CHECK_EQ(events.back().event_type, std::string{"export"});
        CHECK_EQ(events.back().primary_entity_id, std::string{"user-007"});
    }

    const auto small_path = tmp / "small.parquet";
    CHECK(write_single_event(small_path, start + 1234, "export", "user-1"));
    const auto small = sift::ingest_parquet(small_path.string(), kMapping);
    CHECK(small.ok());
    if (small.ok()) {
        CHECK_EQ(small->events.size(), std::size_t{1});
        CHECK_EQ(small->events.events()[0].timestamp.time_since_epoch().count(),
                 start + 1234);
        CHECK_EQ(small->events.events()[0].event_type, std::string{"export"});
        CHECK_EQ(small->events.events()[0].primary_entity_id, std::string{"user-1"});
    }

    const auto null_path = tmp / "null.parquet";
    CHECK(write_null_event(null_path, start));
    const auto null_result = sift::ingest_parquet(null_path.string(), kMapping);
    CHECK(!null_result.ok());
    CHECK(contains(null_result.status().message(),
                   "event column contains null at row 0"));

    const auto wrong_type_path = tmp / "wrong_type.parquet";
    CHECK(write_wrong_type(wrong_type_path, start));
    const auto wrong_type_result =
        sift::ingest_parquet(wrong_type_path.string(), kMapping);
    CHECK(!wrong_type_result.ok());
    CHECK(contains(wrong_type_result.status().message(),
                   "column 'action' has an unexpected type"));

    const auto empty_path = tmp / "empty.parquet";
    CHECK(write_empty(empty_path));
    const auto empty_result = sift::ingest_parquet(empty_path.string(), kMapping);
    CHECK(empty_result.ok());
    if (empty_result.ok()) {
        CHECK(empty_result->schema != nullptr);
        CHECK(empty_result->events.empty());
        CHECK_EQ(empty_result->events.size(), std::size_t{0});
        CHECK(empty_result->events.events().empty());
    }

    const auto absent_result =
        sift::ingest_parquet((tmp / "absent.parquet").string(), kMapping);
    CHECK(!absent_result.ok());
    CHECK(contains(absent_result.status().message(), "Failed to open local file"));

    return sift_test::failures == 0 ? 0 : 1;
}
