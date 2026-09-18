add_executable(app_config_test app_config_test.cpp)
target_link_libraries(app_config_test PRIVATE sift_core)
add_test(NAME app_config
        COMMAND app_config_test ${CMAKE_CURRENT_BINARY_DIR}/app_config_tmp)

add_executable(cli_options_test cli_options_test.cpp)
target_link_libraries(cli_options_test PRIVATE sift_core)
add_test(NAME cli_options COMMAND cli_options_test)

add_executable(column_mapping_test column_mapping_test.cpp)
target_link_libraries(column_mapping_test PRIVATE sift_core)
add_test(NAME column_mapping
        COMMAND column_mapping_test ${CMAKE_CURRENT_BINARY_DIR}/column_mapping_tmp)

add_executable(event_store_test event_store_test.cpp)
target_link_libraries(event_store_test PRIVATE sift_core)
add_test(NAME event_store COMMAND event_store_test)

add_executable(feature_extraction_test feature_extraction_test.cpp)
target_link_libraries(feature_extraction_test PRIVATE sift_core)
add_test(NAME feature_extraction COMMAND feature_extraction_test)

add_executable(analysis_runner_test analysis_runner_test.cpp)
target_link_libraries(analysis_runner_test PRIVATE sift_core)
add_test(NAME analysis_runner COMMAND analysis_runner_test)

add_executable(text_reporter_test text_reporter_test.cpp)
target_link_libraries(text_reporter_test PRIVATE sift_core)
add_test(NAME text_reporter COMMAND text_reporter_test)

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

if(SIFT_ENABLE_JEV)
    add_executable(jev_client_test jev_client_test.cpp)
    target_link_libraries(jev_client_test PRIVATE sift_jev)
    add_test(NAME jev_client COMMAND jev_client_test)

    add_executable(jev_analysis_test jev_analysis_test.cpp)
    target_link_libraries(jev_analysis_test PRIVATE sift_jev)
    add_test(NAME jev_analysis COMMAND jev_analysis_test)
endif()

add_test(NAME generate_fixture
        COMMAND sift_fixture ${CMAKE_CURRENT_BINARY_DIR}/test_events.parquet)
set_tests_properties(generate_fixture PROPERTIES FIXTURES_SETUP events_fixture)
set_tests_properties(parquet_event_reader PROPERTIES FIXTURES_REQUIRED events_fixture)
set_tests_properties(cli_output PROPERTIES FIXTURES_REQUIRED events_fixture)
