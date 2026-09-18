//
// Created by rain on 17/09/26.
//
#include "src/jev/jev_client.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <string_view>

namespace sift {
namespace {

std::once_flag g_curl_once;

void ensure_curl_global() {
    std::call_once(g_curl_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

std::size_t write_body(char* data, std::size_t size, std::size_t count,
                       void* userdata) {
    const auto total = size * count;
    static_cast<std::string*>(userdata)->append(data, total);
    return total;
}

std::size_t write_header(char* data, std::size_t size, std::size_t count,
                         void* userdata) {
    const auto total = size * count;
    auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);
    const std::string_view line{data, total};
    const auto colon = line.find(':');
    if (colon != std::string_view::npos) {
        std::string name{line.substr(0, colon)};
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
        auto value = std::string{line.substr(colon + 1)};
        const auto first = value.find_first_not_of(" \t");
        const auto last = value.find_last_not_of(" \t\r\n");
        value = first == std::string::npos ? std::string{}
                                           : value.substr(first, last - first + 1);
        (*headers)[std::move(name)] = std::move(value);
    }
    return total;
}

}  // namespace

arrow::Result<JevHttpResponse> CurlJevTransport::send(
    const JevHttpRequest& request) const {
    ensure_curl_global();
    CURL* handle = curl_easy_init();
    if (handle == nullptr) {
        return arrow::Status::Invalid(
            "failed to initialize the Jev HTTP transport");
    }

    JevHttpResponse response;
    curl_slist* header_list = nullptr;
    for (const auto& header : request.headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }

    curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(handle, CURLOPT_POST, 1L);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, request.body.c_str());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(request.body.size()));
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header_list);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, request.timeout_ms);
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_body);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, write_header);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &response.headers);

    const auto result = curl_easy_perform(handle);
    long status_code = 0;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code);
    curl_slist_free_all(header_list);
    curl_easy_cleanup(handle);

    if (result != CURLE_OK) {
        return arrow::Status::Invalid("Jev HTTP request failed: ",
                                      curl_easy_strerror(result));
    }
    response.status_code = status_code;
    return response;
}

}  // namespace sift
