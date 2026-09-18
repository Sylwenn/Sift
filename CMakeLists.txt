cmake_minimum_required(VERSION 4.2)
project(Sift)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

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
        src/analysis/feature_extraction.cpp)
target_include_directories(sift_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sift_core PUBLIC
        Arrow::arrow_shared
        Parquet::parquet_shared
        yaml-cpp::yaml-cpp
)

add_executable(Sift main.cpp)
target_link_libraries(Sift PRIVATE sift_core)

add_executable(sift_fixture tests/fixtures/generate_events.cpp)
target_link_libraries(sift_fixture PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
)

enable_testing()
add_subdirectory(tests)
