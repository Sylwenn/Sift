//
// Created by rain on 17/09/26.
//
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "src/jev/jev_client.hpp"
#include "tests/test_support.hpp"

namespace {
using Handler = std::function<arrow::Result<sift::JevHttpResponse>()>;

class FakeTransport : public sift::JevTransport {
public:
    mutable std::vector<sift::JevHttpRequest> requests;
    mutable std::vector<Handler> handlers;
    mutable std::size_t calls = 0;

    arrow::Result<sift::JevHttpResponse> send(
        const sift::JevHttpRequest& request) const override {
        requests.push_back(request);
        if (calls >= handlers.size()) {
            return arrow::Status::Invalid("no scripted response");
        }
        return handlers[calls++]();
    }
};

const std::string kQuestionId = "change_characterization";

const std::string kValidBody =
    "{\"model\":\"jev-1.13.0\",\"answers\":{\"change_characterization\":{"
    "\"type\":\"choice\",\"choice\":\"clear_departure_from_prior_observations\","
    "\"probabilities\":{\"consistent_with_prior_observations\":0.05,"
    "\"clear_departure_from_prior_observations\":0.85,"
    "\"insufficient_prior_context\":0.10},\"confidence\":0.78}},"
    "\"usage\":{\"input_tokens\":312,\"output_tokens\":48}}";

std::pair<sift::JevChoiceQuestion, nlohmann::json> example_question() {
    sift::JevChoiceQuestion question{
        nlohmann::json{{"question", "Which description best characterizes the "
                                    "current export activity?"},
                       {"scope", nlohmann::json::array({"Use only the supplied facts."})}},
        {{"consistent_with_prior_observations", "No clear departure."},
         {"clear_departure_from_prior_observations", "Clear departure."},
         {"insufficient_prior_context", "Not enough prior context."}}};
    nlohmann::json state;
    state["entity_id"] = "user-142";
    return {question, state};
}

sift::JevClient make_client(const sift::JevTransport& transport,
                            std::vector<std::chrono::milliseconds>* sleeps,
                            int max_retries = 2) {
    sift::JevClientOptions options;
    options.api_key = "test-key";
    options.base_url = "https://example.test";
    options.model = "jev-1.13.0";
    options.max_retries = max_retries;
    sift::JevSleeper sleeper = [sleeps](std::chrono::milliseconds duration) {
        if (sleeps != nullptr) {
            sleeps->push_back(duration);
        }
    };
    return sift::JevClient{std::move(options), transport, std::move(sleeper)};
}

sift::JevHttpResponse http_response(long status, std::string body,
                                    std::map<std::string, std::string> headers = {}) {
    sift::JevHttpResponse response;
    response.status_code = status;
    response.body = std::move(body);
    response.headers = std::move(headers);
    return response;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}
}

int main() {
    const auto [question, state] = example_question();

    FakeTransport transport;
    std::vector<std::chrono::milliseconds> sleeps;
    auto client = make_client(transport, &sleeps);
    transport.handlers.push_back([&] {
        return http_response(200, kValidBody, {{"x-typesafe-request-id", "req-1"}});
    });

    const auto answer = client.ask_choice(state, kQuestionId, question);
    CHECK(answer.ok());
    if (answer.ok()) {
        CHECK_EQ(answer->selected_value,
                 std::string{"clear_departure_from_prior_observations"});
        CHECK_EQ(answer->probabilities.at("clear_departure_from_prior_observations"),
                 0.85);
        CHECK_EQ(answer->confidence, 0.78);
        CHECK_EQ(answer->model, std::string{"jev-1.13.0"});
        CHECK_EQ(answer->request_id, std::string{"req-1"});
        CHECK_EQ(answer->usage.input_tokens, std::int64_t{312});
        CHECK_EQ(answer->usage.output_tokens, std::int64_t{48});
    }

    CHECK_EQ(transport.requests.size(), std::size_t{1});
    if (!transport.requests.empty()) {
        const auto& request = transport.requests.front();
        CHECK_EQ(request.method, std::string{"POST"});
        CHECK_EQ(request.url, std::string{"https://example.test/v1/systemone"});
        CHECK(request.headers.size() == 2);
        CHECK_EQ(request.headers[0], std::string{"Authorization: Bearer test-key"});
        const auto body = nlohmann::json::parse(request.body);
        CHECK_EQ(body["model"].get<std::string>(), std::string{"jev-1.13.0"});
        CHECK_EQ(body["state"]["entity_id"].get<std::string>(),
                 std::string{"user-142"});
        CHECK_EQ(body["questions"][kQuestionId]["type"].get<std::string>(),
                 std::string{"choice"});
        CHECK_EQ(body["questions"][kQuestionId]["criteria"].size(), std::size_t{3});
        CHECK(contains(body["questions"][kQuestionId]["instructions"].dump(),
                       "current export activity"));
    }

    // Malformed and unexpected responses.
    FakeTransport bad;
    std::vector<std::chrono::milliseconds> no_sleeps;
    auto bad_client = make_client(bad, &no_sleeps);
    bad.handlers.push_back([] { return http_response(200, "not json"); });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back(
        [] { return http_response(200, "{\"model\":\"m\"}"); });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back([] {
        return http_response(
            200,
            "{\"model\":\"m\",\"answers\":{\"other\":{\"type\":\"choice\","
            "\"choice\":\"a\",\"probabilities\":{\"a\":1.0}}}}");
    });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back([] {
        return http_response(
            200,
            "{\"model\":\"m\",\"answers\":{\"change_characterization\":{"
            "\"type\":\"noul\",\"noul\":0.5}}}");
    });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back([] {
        return http_response(
            200,
            "{\"model\":\"m\",\"answers\":{\"change_characterization\":{"
            "\"type\":\"choice\",\"choice\":\"a\",\"probabilities\":{\"a\":1.5,"
            "\"b\":-0.5}}}}");
    });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back([] {
        return http_response(
            200,
            "{\"model\":\"m\",\"answers\":{\"change_characterization\":{"
            "\"type\":\"choice\",\"choice\":\"a\",\"probabilities\":{\"a\":0.2,"
            "\"b\":0.2}}}}");
    });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    bad.handlers.clear();
    bad.calls = 0;
    bad.handlers.push_back([] {
        return http_response(
            200,
            "{\"model\":\"m\",\"answers\":{\"change_characterization\":{"
            "\"type\":\"choice\",\"choice\":\"a\",\"probabilities\":{\"a\":1.0},"
            "\"confidence\":1.5}}}");
    });
    CHECK(!bad_client.ask_choice(state, kQuestionId, question).ok());

    // Non-retryable errors.
    FakeTransport unauth;
    std::vector<std::chrono::milliseconds> unauth_sleeps;
    auto unauth_client = make_client(unauth, &unauth_sleeps);
    unauth.handlers.push_back([&] {
        return http_response(401, "{\"error\":\"unauthorized\"}");
    });
    CHECK(!unauth_client.ask_choice(state, kQuestionId, question).ok());
    CHECK_EQ(unauth.calls, std::size_t{1});
    CHECK(unauth_sleeps.empty());

    FakeTransport unprocessable;
    std::vector<std::chrono::milliseconds> unprocessable_sleeps;
    auto unprocessable_client = make_client(unprocessable, &unprocessable_sleeps);
    unprocessable.handlers.push_back([] {
        return http_response(422, "{\"error\":\"bad request\"}");
    });
    CHECK(!unprocessable_client.ask_choice(state, kQuestionId, question).ok());
    CHECK_EQ(unprocessable.calls, std::size_t{1});
    CHECK(unprocessable_sleeps.empty());

    // Retryable 429 then success, honoring retry-after-ms.
    FakeTransport limited;
    std::vector<std::chrono::milliseconds> limited_sleeps;
    auto limited_client = make_client(limited, &limited_sleeps);
    limited.handlers.push_back([&] {
        return http_response(429, "{}", {{"retry-after-ms", "1234"}});
    });
    limited.handlers.push_back(
        [&] { return http_response(200, kValidBody); });
    const auto limited_answer = limited_client.ask_choice(state, kQuestionId, question);
    CHECK(limited_answer.ok());
    CHECK_EQ(limited.calls, std::size_t{2});
    CHECK_EQ(limited_sleeps.size(), std::size_t{1});
    if (!limited_sleeps.empty()) {
        CHECK_EQ(limited_sleeps.front().count(), std::int64_t{1234});
    }

    // Transport failure then success.
    FakeTransport flaky;
    std::vector<std::chrono::milliseconds> flaky_sleeps;
    auto flaky_client = make_client(flaky, &flaky_sleeps);
    flaky.handlers.push_back(
        [] { return arrow::Status::Invalid("connection reset"); });
    flaky.handlers.push_back([&] { return http_response(200, kValidBody); });
    CHECK(flaky_client.ask_choice(state, kQuestionId, question).ok());
    CHECK_EQ(flaky.calls, std::size_t{2});
    CHECK_EQ(flaky_sleeps.size(), std::size_t{1});

    // Retry exhaustion.
    FakeTransport dead;
    std::vector<std::chrono::milliseconds> dead_sleeps;
    auto dead_client = make_client(dead, &dead_sleeps);
    for (int i = 0; i < 3; ++i) {
        dead.handlers.push_back([&] { return http_response(529, "{}"); });
    }
    CHECK(!dead_client.ask_choice(state, kQuestionId, question).ok());
    CHECK_EQ(dead.calls, std::size_t{3});
    CHECK_EQ(dead_sleeps.size(), std::size_t{2});

    // Missing credential fails before any request.
    FakeTransport unused;
    std::vector<std::chrono::milliseconds> unused_sleeps;
    sift::JevClientOptions empty_options;
    empty_options.api_key.clear();
    sift::JevClient empty_client{empty_options, unused,
                                 [&](std::chrono::milliseconds d) {
                                     unused_sleeps.push_back(d);
                                 }};
    CHECK(!empty_client.ask_choice(state, kQuestionId, question).ok());
    CHECK_EQ(unused.calls, std::size_t{0});

    unsetenv("TYPESAFE_API_KEY");
    CHECK(!sift::read_typesafe_api_key().ok());
    setenv("TYPESAFE_API_KEY", "secret", 1);
    const auto key = sift::read_typesafe_api_key();
    unsetenv("TYPESAFE_API_KEY");
    CHECK(key.ok());
    if (key.ok()) {
        CHECK_EQ(*key, std::string{"secret"});
    }

    CHECK_EQ(sift::retry_backoff(0, std::nullopt).count(), std::int64_t{500});
    CHECK_EQ(sift::retry_backoff(3, std::nullopt).count(), std::int64_t{4000});
    CHECK_EQ(sift::retry_backoff(10, std::nullopt).count(), std::int64_t{5000});
    CHECK_EQ(sift::retry_backoff(0, std::chrono::milliseconds{70000}).count(),
             std::int64_t{60000});

    return sift_test::failures == 0 ? 0 : 1;
}
