cmake_minimum_required(VERSION 4.2)
project(Sift)
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(Sift main.cpp
        src/domain/event.hpp
        src/ingestion/schema_mapping.hpp
        src/ingestion/schema_validator.hpp
        src/ingestion/schema_validator.cpp)
target_link_libraries(Sift PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
        yaml-cpp::yaml-cpp
)

add_executable(sift_fixture tests/fixtures/generate_events.cpp)
target_link_libraries(sift_fixture PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
)
