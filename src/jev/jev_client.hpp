//
// Created by rain on 17/09/26.
//

#ifndef SIFT_JEV_CLIENT_HPP
#define SIFT_JEV_CLIENT_HPP

#include <arrow/result.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "src/analysis/analysis_types.hpp"

namespace sift {

struct JevChoiceQuestion {
    nlohmann::json instructions;
    std::map<std::string, std::string> criteria;
};

struct JevChoiceAnswer {
    std::string selected_value;
    std::map<std::string, double> probabilities;
    double confidence = 0.0;
    std::string model;
    std::string request_id;
    TokenUsage usage;
};

struct JevHttpRequest {
    std::string method;
    std::string url;
    std::vector<std::string> headers;
    std::string body;
    long timeout_ms = 10000;
};

struct JevHttpResponse {
    long status_code = 0;
    std::map<std::string, std::string> headers;
    std::string body;
};

// Narrow transport seam. Real builds use libcurl; tests use a fake.
class JevTransport {
public:
    virtual ~JevTransport() = default;
    virtual arrow::Result<JevHttpResponse> send(
        const JevHttpRequest& request) const = 0;
};

// libcurl-backed transport. It is declared here so shared code never includes
// libcurl headers.
class CurlJevTransport : public JevTransport {
public:
    arrow::Result<JevHttpResponse> send(const JevHttpRequest& request) const override;
};

using JevSleeper = std::function<void(std::chrono::milliseconds)>;

struct JevClientOptions {
    std::string api_key;
    std::string base_url = "https://api.typesafe.ai";
    std::string model = "jev-1.13.0";
    long timeout_ms = 10000;
    int max_retries = 2;
};

// Reads TYPESAFE_API_KEY. It fails before any network request when missing.
arrow::Result<std::string> read_typesafe_api_key();

// Deterministic bounded backoff. A valid Retry-After value takes precedence.
std::chrono::milliseconds retry_backoff(
    int attempt, std::optional<std::chrono::milliseconds> retry_after);

class JevClient {
public:
    JevClient(JevClientOptions options, const JevTransport& transport,
              JevSleeper sleeper = nullptr);

    arrow::Result<JevChoiceAnswer> ask_choice(const nlohmann::json& state,
                                              const std::string& question_id,
                                              const JevChoiceQuestion& question) const;

    const JevClientOptions& options() const { return options_; }

private:
    arrow::Result<JevHttpResponse> send_with_retries(
        const JevHttpRequest& request) const;

    JevClientOptions options_;
    const JevTransport* transport_;
    JevSleeper sleeper_;
};

}  // namespace sift

#endif  // SIFT_JEV_CLIENT_HPP
