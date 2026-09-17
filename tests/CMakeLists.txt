add_executable(column_mapping_test column_mapping_test.cpp)
target_link_libraries(column_mapping_test PRIVATE sift_core)
add_test(NAME column_mapping
        COMMAND column_mapping_test ${CMAKE_CURRENT_BINARY_DIR}/column_mapping_tmp)

add_executable(event_store_test event_store_test.cpp)
target_link_libraries(event_store_test PRIVATE sift_core)
add_test(NAME event_store COMMAND event_store_test)

add_executable(daily_exports_test daily_exports_test.cpp)
target_link_libraries(daily_exports_test PRIVATE sift_core)
add_test(NAME daily_exports COMMAND daily_exports_test)

add_executable(parquet_event_reader_test parquet_event_reader_test.cpp)
target_link_libraries(parquet_event_reader_test PRIVATE sift_core)
add_test(NAME parquet_event_reader
        COMMAND parquet_event_reader_test
                ${CMAKE_CURRENT_BINARY_DIR}/test_events.parquet
                ${CMAKE_CURRENT_BINARY_DIR}/parquet_event_reader_tmp)

add_executable(cli_output_test cli_output_test.cpp)
target_link_libraries(cli_output_test PRIVATE sift_core)
add_test(NAME cli_output
        COMMAND cli_output_test
                $<TARGET_FILE:Sift>
                ${CMAKE_CURRENT_BINARY_DIR}/test_events.parquet
                ${CMAKE_SOURCE_DIR}/sift.yaml)

add_test(NAME generate_fixture
        COMMAND sift_fixture ${CMAKE_CURRENT_BINARY_DIR}/test_events.parquet)
set_tests_properties(generate_fixture PROPERTIES FIXTURES_SETUP events_fixture)
set_tests_properties(parquet_event_reader PROPERTIES FIXTURES_REQUIRED events_fixture)
set_tests_properties(cli_output PROPERTIES FIXTURES_REQUIRED events_fixture)
