//
// Created by rain on 07/08/26.
//

#ifndef SIFT_SCHEMA_VALIDATOR_HPP
#define SIFT_SCHEMA_VALIDATOR_HPP
#include <arrow/status.h>
#include <arrow/type.h>

#include "schema_mapping.hpp"

namespace sift {
arrow::Status validate_schema(const arrow::Schema& schema, const SchemaMapping& schema_map);
}

#endif //SIFT_SCHEMA_VALIDATOR_HPP
