//
// Created by rain on 07/08/26.
//

#include <arrow/api.h>
#include <arrow/io/file.h>
#include <parquet/arrow/writer.h>

#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
    const std::string path = argc == 2 ? argv[1] : "tests/fixtures/events.parquet";
    auto* pool = arrow::default_memory_pool();

    arrow::TimestampBuilder timestamps(arrow::timestamp(arrow::TimeUnit::MILLI), pool);
    arrow::StringBuilder actions(pool);
    arrow::StringBuilder users(pool);

    const auto start = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::sys_days{
            std::chrono::year{2026} / std::chrono::January / 1}.time_since_epoch()).count();
    const auto append_event = [&](int64_t timestamp, const char* action,
                                  const char* user) {
        return timestamps.Append(timestamp).ok() &&
               actions.Append(action).ok() && users.Append(user).ok();
    };

    for (int64_t day = 0; day < 10; ++day) {
        const auto day_start = start + day * 86'400'000;
        const int64_t user_142_exports = day == 9 ? 20 : 4;
        if (!append_event(day_start, "login", "user-142") ||
            !append_event(day_start + 3'600'000, "login", "user-007")) {
            std::cerr << "failed to append fixture data\n";
            return 1;
        }
        for (int64_t i = 0; i < user_142_exports; ++i) {
            if (!append_event(day_start + 60'000 * (i + 1), "export", "user-142")) {
                std::cerr << "failed to append fixture data\n";
                return 1;
            }
        }
        for (int64_t i = 0; i < 4; ++i) {
            if (!append_event(day_start + 3'660'000 + 60'000 * i,
                              "export", "user-007")) {
                std::cerr << "failed to append fixture data\n";
                return 1;
            }
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
