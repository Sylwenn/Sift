//
// Created by rain on 07/08/26.
//

#ifndef SIFT_COLUMN_MAPPING_HPP
#define SIFT_COLUMN_MAPPING_HPP
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type.h>
#include <yaml-cpp/yaml.h>

#include <string>

namespace sift {
struct SchemaMapping {
    std::string timestamp_column;
    std::string event_type_column;
    std::string primary_entity_id_column;
};

arrow::Result<SchemaMapping> mapping_from_yaml(const YAML::Node& root);

arrow::Result<SchemaMapping> load_mapping(const std::string& path);

arrow::Status validate_schema(const arrow::Schema& schema, const SchemaMapping& schema_map);
}

#endif //SIFT_COLUMN_MAPPING_HPP
