//
// Created by rain on 07/08/26.
//
#include "src/ingestion/column_mapping.hpp"

#include <yaml-cpp/yaml.h>

namespace sift {
arrow::Result<SchemaMapping> mapping_from_yaml(const YAML::Node& config) {
    if (!config.IsMap() || !config["timestamp"] ||
        !config["event_type"] || !config["entity_id"]) {
        return arrow::Status::Invalid(
            "config must define timestamp, event_type, and entity_id");
    }
    return SchemaMapping{
        config["timestamp"].as<std::string>(),
        config["event_type"].as<std::string>(),
        config["entity_id"].as<std::string>()};
}

arrow::Result<SchemaMapping> load_mapping(const std::string& path) {
    try {
        return mapping_from_yaml(YAML::LoadFile(path));
    } catch (const YAML::Exception& error) {
        return arrow::Status::Invalid("failed to read config: ", error.what());
    }
}

arrow::Status validate_schema(const arrow::Schema& schema, const SchemaMapping& schema_map) {
    if (schema_map.timestamp_column.empty() ||
        schema_map.event_type_column.empty() ||
        schema_map.primary_entity_id_column.empty()) {
        return arrow::Status::Invalid("schema mapping contains an empty column name");
    }

    if (schema_map.timestamp_column == schema_map.event_type_column ||
        schema_map.timestamp_column == schema_map.primary_entity_id_column ||
        schema_map.event_type_column == schema_map.primary_entity_id_column) {
        return arrow::Status::Invalid("schema mapping assigns one column to multiple fields");
    }

    const auto check_column = [&schema](const std::string& name, arrow::Type::type expected) {
        const auto indices = schema.GetAllFieldIndices(name);
        if (indices.size() != 1) {
            return arrow::Status::Invalid("expected exactly one column named '", name, "'");
        }
        if (schema.field(indices.front())->type()->id() != expected) {
            return arrow::Status::Invalid("column '", name, "' has an unexpected type");
        }
        return arrow::Status::OK();
    };

    if (auto status = check_column(schema_map.timestamp_column, arrow::Type::TIMESTAMP);
        !status.ok()) {
        return status;
    }
    const auto timestamp_field =
        schema.field(schema.GetFieldIndex(schema_map.timestamp_column));
    const auto timestamp_type =
        std::static_pointer_cast<arrow::TimestampType>(timestamp_field->type());
    if (timestamp_type->unit() != arrow::TimeUnit::MILLI) {
        return arrow::Status::Invalid("timestamp column must use millisecond precision");
    }
    if (auto status = check_column(schema_map.event_type_column, arrow::Type::STRING);
        !status.ok()) {
        return status;
    }
    return check_column(schema_map.primary_entity_id_column, arrow::Type::STRING);
}
}
