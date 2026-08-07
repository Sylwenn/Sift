//
// Created by rain on 07/08/26.
//

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <iostream>

int main(int argc, char* argv[]) {
    const std::string path = argc == 2 ? argv[1] : "tests/fixtures/events.parquet";
    auto* pool = arrow::default_memory_pool();

    arrow::TimestampBuilder timestamps(arrow::timestamp(arrow::TimeUnit::MILLI), pool);
    arrow::StringBuilder actions(pool);
    arrow::StringBuilder users(pool);
    for (int64_t i = 0; i < 4; ++i) {
        if (!timestamps.Append(i * 60'000).ok() ||
            !actions.Append(i == 3 ? "export" : "login").ok() ||
            !users.Append("user-142").ok()) {
            std::cerr << "failed to append fixture data\n";
            return 1;
        }
    }

    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> action_array;
    std::shared_ptr<arrow::Array> user_array;
    if (!timestamps.Finish(&timestamp_array).ok() ||
        !actions.Finish(&action_array).ok() ||
        !users.Finish(&user_array).ok()) {
        std::cerr << "failed to finish fixture arrays\n";
        return 1;
    }

    auto schema = arrow::schema({
        arrow::field("occurred_at", arrow::timestamp(arrow::TimeUnit::MILLI)),
        arrow::field("action", arrow::utf8()),
        arrow::field("user_id", arrow::utf8())});
    auto table = arrow::Table::Make(schema, {timestamp_array, action_array, user_array});
    auto output = arrow::io::FileOutputStream::Open(path);
    if (!output.ok()) {
        std::cerr << output.status().ToString() << '\n';
        return 1;
    }

    const auto status = parquet::arrow::WriteTable(
        *table, pool, *output, table->num_rows());
    if (!status.ok()) {
        std::cerr << status.ToString() << '\n';
        return 1;
    }
    return 0;
}
