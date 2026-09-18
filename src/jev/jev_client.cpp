//
// Created by rain on 17/09/26.
//
#include "src/jev/jev_client.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace sift {
namespace {

constexpr std::string_view kChoiceType = "choice";
constexpr std::string_view kSystemOnePath = "/v1/systemone";
constexpr double kProbabilityTolerance = 1e-3;

bool is_retryable_status(long status) {
    return status == 408 || status == 429 || (status >= 500 && status <= 599);
}

std::string join_url(std::string base, std::string_view path) {
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    return base + std::string{path};
}

std::string truncate(std::string value, std::size_t limit) {
    if (value.size() <= limit) {
        return value;
    }
    value.resize(limit);
    value += "...";
    return value;
}

std::optional<std::chrono::milliseconds> parse_retry_after(
    const JevHttpResponse& response) {
    const auto millis = response.headers.find("retry-after-ms");
    if (millis != response.headers.end()) {
        try {
            const auto value = std::stoll(millis->second);
            if (value >= 0) {
                return std::chrono::milliseconds{value};
            }
        } catch (const std::exception&) {
        }
    }
    const auto seconds = response.headers.find("retry-after");
    if (seconds != response.headers.end()) {
        try {
            const auto value = std::stod(seconds->second);
            if (value >= 0.0) {
                return std::chrono::milliseconds{
                    static_cast<std::int64_t>(value * 1000.0)};
            }
        } catch (const std::exception&) {
        }
    }
    return std::nullopt;
}

nlohmann::json build_request(const JevClientOptions& options,
                             const nlohmann::json& state,
                             const std::string& question_id,
                             const JevChoiceQuestion& question) {
    nlohmann::json question_json;
    question_json["type"] = kChoiceType;
    question_json["instructions"] = question.instructions;
    nlohmann::json criteria = nlohmann::json::object();
    for (const auto& [option, description] : question.criteria) {
        criteria[option] = description;
    }
    question_json["criteria"] = std::move(criteria);

    nlohmann::json body;
    body["state"] = state;
    body["model"] = options.model;
    body["questions"] = nlohmann::json::object();
    body["questions"][question_id] = std::move(question_json);
    return body;
}

arrow::Result<JevChoiceAnswer> parse_choice_answer(const std::string& body,
                                                   const std::string& question_id,
                                                   const std::string& request_id) {
    nlohmann::json document;
    try {
        document = nlohmann::json::parse(body);
    } catch (const std::exception& error) {
        return arrow::Status::Invalid("Jev response is not valid JSON: ",
                                      error.what());
    }
    if (!document.is_object()) {
        return arrow::Status::Invalid("Jev response must be a JSON object");
    }
    if (!document.contains("model") || !document["model"].is_string()) {
        return arrow::Status::Invalid(
            "Jev response is missing a string 'model' field");
    }
    if (!document.contains("answers") || !document["answers"].is_object()) {
        return arrow::Status::Invalid(
            "Jev response is missing an 'answers' object");
    }
    const auto& answers = document["answers"];
    if (!answers.contains(question_id)) {
        return arrow::Status::Invalid("Jev response is missing an answer for '",
                                      question_id, "'");
    }
    const auto& answer = answers[question_id];
    if (!answer.is_object()) {
        return arrow::Status::Invalid("Jev answer for '", question_id,
                                      "' must be an object");
    }
    if (!answer.contains("type") || !answer["type"].is_string() ||
        answer["type"].get<std::string>() != kChoiceType) {
        return arrow::Status::Invalid("Jev answer for '", question_id,
                                      "' is not a choice");
    }
    if (!answer.contains("choice") || !answer["choice"].is_string()) {
        return arrow::Status::Invalid("Jev choice answer for '", question_id,
                                      "' is missing a string 'choice'");
    }
    if (!answer.contains("probabilities") ||
        !answer["probabilities"].is_object()) {
        return arrow::Status::Invalid("Jev choice answer for '", question_id,
                                      "' is missing a 'probabilities' object");
    }

    JevChoiceAnswer parsed;
    parsed.model = document["model"].get<std::string>();
    parsed.request_id = request_id;
    parsed.selected_value = answer["choice"].get<std::string>();

    double probability_sum = 0.0;
    for (const auto& [option, value] : answer["probabilities"].items()) {
        if (!value.is_number()) {
            return arrow::Status::Invalid(
                "Jev probability for option '", option, "' is not a number");
        }
        const auto probability = value.get<double>();
        if (probability < 0.0 || probability > 1.0) {
            return arrow::Status::Invalid(
                "Jev probability for option '", option, "' is outside [0, 1]");
        }
        probability_sum += probability;
        parsed.probabilities[option] = probability;
    }
    if (parsed.probabilities.find(parsed.selected_value) ==
        parsed.probabilities.end()) {
        return arrow::Status::Invalid(
            "Jev selected option '", parsed.selected_value,
            "' has no probability");
    }
    if (std::abs(probability_sum - 1.0) > kProbabilityTolerance) {
        return arrow::Status::Invalid(
            "Jev choice probabilities do not sum to 1");
    }

    if (answer.contains("confidence")) {
        if (!answer["confidence"].is_number()) {
            return arrow::Status::Invalid(
                "Jev choice confidence is not a number");
        }
        parsed.confidence = answer["confidence"].get<double>();
        if (parsed.confidence < 0.0 || parsed.confidence > 1.0) {
            return arrow::Status::Invalid(
                "Jev choice confidence is outside [0, 1]");
        }
    }

    if (document.contains("usage") && document["usage"].is_object()) {
        const auto& usage = document["usage"];
        if (usage.contains("input_tokens") && usage["input_tokens"].is_number_integer()) {
            parsed.usage.input_tokens = usage["input_tokens"].get<std::int64_t>();
        }
        if (usage.contains("output_tokens") &&
            usage["output_tokens"].is_number_integer()) {
            parsed.usage.output_tokens = usage["output_tokens"].get<std::int64_t>();
        }
    }
    return parsed;
}

}  // namespace

arrow::Result<std::string> read_typesafe_api_key() {
    const char* key = std::getenv("TYPESAFE_API_KEY");
    if (key == nullptr || *key == '\0') {
        return arrow::Status::Invalid(
            "TYPESAFE_API_KEY is not set; Jev mode requires an API key");
    }
    return std::string{key};
}

std::chrono::milliseconds retry_backoff(
    int attempt, std::optional<std::chrono::milliseconds> retry_after) {
    constexpr auto base = std::chrono::milliseconds{500};
    constexpr auto max_backoff = std::chrono::milliseconds{5000};
    constexpr auto max_retry_after = std::chrono::milliseconds{60000};
    if (retry_after) {
        return std::min(*retry_after, max_retry_after);
    }
    auto delay = base;
    for (int i = 0; i < attempt && delay < max_backoff; ++i) {
        delay *= 2;
    }
    return std::min(delay, max_backoff);
}

JevClient::JevClient(JevClientOptions options, const JevTransport& transport,
                     JevSleeper sleeper)
    : options_(std::move(options)),
      transport_(&transport),
      sleeper_(std::move(sleeper)) {
    if (!sleeper_) {
        sleeper_ = [](std::chrono::milliseconds duration) {
            std::this_thread::sleep_for(duration);
        };
    }
}

arrow::Result<JevHttpResponse> JevClient::send_with_retries(
    const JevHttpRequest& request) const {
    const int total_attempts = std::max(0, options_.max_retries) + 1;
    arrow::Status last_status =
        arrow::Status::Invalid("Jev request was not attempted");

    for (int attempt = 0; attempt < total_attempts; ++attempt) {
        auto response = transport_->send(request);
        const bool final_attempt = attempt + 1 >= total_attempts;
        if (!response.ok()) {
            last_status = response.status();
            if (final_attempt) {
                break;
            }
            sleeper_(retry_backoff(attempt, std::nullopt));
            continue;
        }
        const auto status_code = response->status_code;
        if (status_code >= 200 && status_code < 300) {
            return *response;
        }
        const auto detail = truncate(response->body, 500);
        last_status = arrow::Status::Invalid("Jev request failed with HTTP ",
                                             std::to_string(status_code), ": ",
                                             detail);
        if (!is_retryable_status(status_code) || final_attempt) {
            break;
        }
        sleeper_(retry_backoff(attempt, parse_retry_after(*response)));
    }
    return last_status;
}

arrow::Result<JevChoiceAnswer> JevClient::ask_choice(
    const nlohmann::json& state, const std::string& question_id,
    const JevChoiceQuestion& question) const {
    if (options_.api_key.empty()) {
        return arrow::Status::Invalid(
            "TYPESAFE_API_KEY is not set; Jev mode requires an API key");
    }

    JevHttpRequest request;
    request.method = "POST";
    request.url = join_url(options_.base_url, kSystemOnePath);
    request.headers = {"Authorization: Bearer " + options_.api_key,
                       "Content-Type: application/json"};
    request.body = build_request(options_, state, question_id, question).dump();
    request.timeout_ms = options_.timeout_ms;

    auto response = send_with_retries(request);
    if (!response.ok()) {
        return response.status();
    }

    std::string request_id;
    const auto id_header = response->headers.find("x-typesafe-request-id");
    if (id_header != response->headers.end()) {
        request_id = id_header->second;
    }
    return parse_choice_answer(response->body, question_id, request_id);
}

}  // namespace sift
