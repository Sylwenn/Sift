cmake_minimum_required(VERSION 4.2)
project(Sift)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

option(SIFT_ENABLE_JEV "Build the TypeSafe Jev backend" OFF)
option(SIFT_RUN_LIVE_JEV_TESTS "Register opt-in live Jev tests" OFF)

add_library(sift_core STATIC
        src/domain/event.hpp
        src/config/app_config.hpp
        src/config/app_config.cpp
        src/config/cli_options.hpp
        src/config/cli_options.cpp
        src/ingestion/column_mapping.hpp
        src/ingestion/column_mapping.cpp
        src/ingestion/parquet_event_reader.hpp
        src/ingestion/parquet_event_reader.cpp
        src/storage/event_store.hpp
        src/storage/event_store.cpp
        src/analysis/daily_exports.hpp
        src/analysis/daily_exports.cpp
        src/analysis/analysis_types.hpp
        src/analysis/feature_extraction.hpp
        src/analysis/feature_extraction.cpp
        src/analysis/analysis_runner.hpp
        src/analysis/analysis_runner.cpp
        src/reporting/text_reporter.hpp
        src/reporting/text_reporter.cpp)
target_include_directories(sift_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sift_core PUBLIC
        Arrow::arrow_shared
        Parquet::parquet_shared
        yaml-cpp::yaml-cpp
)

if(SIFT_ENABLE_JEV)
    find_package(CURL REQUIRED)
    add_library(sift_jev STATIC
            src/jev/jev_client.hpp
            src/jev/jev_client.cpp
            src/jev/curl_jev_transport.cpp
            src/jev/jev_analysis.hpp
            src/jev/jev_analysis.cpp)
    target_link_libraries(sift_jev PUBLIC sift_core)
    target_link_libraries(sift_jev PRIVATE CURL::libcurl)
    target_compile_definitions(sift_jev PUBLIC SIFT_ENABLE_JEV=1)
endif()

add_executable(Sift main.cpp)
target_link_libraries(Sift PRIVATE sift_core)
if(SIFT_ENABLE_JEV)
    target_link_libraries(Sift PRIVATE sift_jev)
endif()

add_executable(sift_fixture tests/fixtures/generate_events.cpp)
target_link_libraries(sift_fixture PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
)

enable_testing()
add_subdirectory(tests)
