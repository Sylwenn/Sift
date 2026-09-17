//
// Created by rain on 07/08/26.
//
#include <sys/wait.h>

#include <array>
#include <cstdio>
#include <iostream>
#include <string>

#include "tests/test_support.hpp"

namespace {
std::string quote(const std::string& value) {
    return "'" + value + "'";
}

std::string run_capture(const std::string& command, int& exit_code) {
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        exit_code = -1;
        return {};
    }
    std::string output;
    std::array<char, 4096> buffer{};
    std::size_t read = 0;
    while ((read = std::fread(buffer.data(), 1, buffer.size(), pipe)) > 0) {
        output.append(buffer.data(), read);
    }
    const int status = pclose(pipe);
    exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return output;
}
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "usage: cli_output_test <Sift> <fixture.parquet> <config.yaml>\n";
        return 2;
    }
    const std::string sift = quote(argv[1]);
    const std::string fixture = quote(argv[2]);
    const std::string config = quote(argv[3]);

    int exit_code = -1;
    const std::string output = run_capture(sift + " " + fixture + " " + config, exit_code);
    CHECK_EQ(exit_code, 0);
    const std::string expected =
        "occurred_at: timestamp[ms]\n"
        "action: string\n"
        "user_id: string\n"
        "events: 116\n"
        "user-007 daily_exports: 4 4 4 4 4 4 4 4 4 4\n"
        "user-142 daily_exports: 4 4 4 4 4 4 4 4 4 20\n";
    CHECK_EQ(output, expected);

    int usage_exit = -1;
    const std::string usage = run_capture(sift + " 2>&1", usage_exit);
    CHECK_EQ(usage_exit, 2);
    CHECK_EQ(usage, std::string{"Usage: Sift <events.parquet> [config.yaml]\n"});

    int config_exit = -1;
    const std::string bad_config =
        run_capture(sift + " " + fixture + " '/nonexistent-sift.yaml' 2>&1", config_exit);
    CHECK_EQ(config_exit, 1);
    CHECK(bad_config.find("failed to read config") != std::string::npos);

    return sift_test::failures == 0 ? 0 : 1;
}
