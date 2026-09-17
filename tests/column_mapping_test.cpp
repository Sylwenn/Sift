//
// Created by rain on 07/08/26.
//
#include <arrow/api.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "src/ingestion/column_mapping.hpp"
#include "tests/test_support.hpp"

namespace {
bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

void write_file(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream output(path);
    output << contents;
}

std::shared_ptr<arrow::Schema> canonical_schema() {
    return arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::utf8()),
        arrow::field("user_id", arrow::utf8())});
}
}

int main(int argc, char* argv[]) {
    const std::filesystem::path tmp =
        argc > 1 ? std::filesystem::path{argv[1]}
                 : std::filesystem::temp_directory_path() / "sift_column_mapping_test";
    std::filesystem::create_directories(tmp);

    const auto valid_path = tmp / "valid.yaml";
    write_file(valid_path,
               "timestamp: occurred_at\nevent_type: action\nentity_id: user_id\n");

    const auto mapping_result = sift::load_mapping(valid_path.string());
    CHECK(mapping_result.ok());
    if (mapping_result.ok()) {
        CHECK_EQ(mapping_result->timestamp_column, std::string{"occurred_at"});
        CHECK_EQ(mapping_result->event_type_column, std::string{"action"});
        CHECK_EQ(mapping_result->primary_entity_id_column, std::string{"user_id"});
    }

    const auto missing_key_path = tmp / "missing_key.yaml";
    write_file(missing_key_path, "timestamp: occurred_at\nevent_type: action\n");
    const auto missing_key_result = sift::load_mapping(missing_key_path.string());
    CHECK(!missing_key_result.ok());
    CHECK(contains(missing_key_result.status().message(),
                   "config must define timestamp, event_type, and entity_id"));

    const auto sequence_path = tmp / "sequence.yaml";
    write_file(sequence_path, "- one\n- two\n");
    const auto sequence_result = sift::load_mapping(sequence_path.string());
    CHECK(!sequence_result.ok());

    const auto absent_result = sift::load_mapping((tmp / "absent.yaml").string());
    CHECK(!absent_result.ok());
    CHECK(contains(absent_result.status().message(), "failed to read config"));

    const sift::SchemaMapping mapping{"occurred_at", "action", "user_id"};
    const auto schema = canonical_schema();
    CHECK(sift::validate_schema(*schema, mapping).ok());

    CHECK(!sift::validate_schema(*schema, {"", "action", "user_id"}).ok());
    CHECK(contains(
        sift::validate_schema(*schema, {"", "action", "user_id"}).message(),
        "empty column name"));
    CHECK(contains(
        sift::validate_schema(*schema, {"occurred_at", "occurred_at", "user_id"})
            .message(),
        "assigns one column to multiple fields"));
    CHECK(contains(
        sift::validate_schema(*schema, {"missing", "action", "user_id"}).message(),
        "expected exactly one column named 'missing'"));
    CHECK(contains(
        sift::validate_schema(*schema, {"occurred_at", "action", "missing"}).message(),
        "expected exactly one column named 'missing'"));

    const auto wrong_type = arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::int64()),
        arrow::field("user_id", arrow::utf8())});
    CHECK(contains(sift::validate_schema(*wrong_type, mapping).message(),
                   "column 'action' has an unexpected type"));

    const auto wrong_unit = arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::SECOND)),
        arrow::field("action", arrow::utf8()),
        arrow::field("user_id", arrow::utf8())});
    CHECK(contains(sift::validate_schema(*wrong_unit, mapping).message(),
                   "timestamp column must use millisecond precision"));

    const auto duplicate_field = arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::utf8()),
        arrow::field("user_id", arrow::utf8())});
    CHECK(contains(sift::validate_schema(*duplicate_field, mapping).message(),
                   "expected exactly one column named 'occurred_at'"));

    return sift_test::failures == 0 ? 0 : 1;
}
