//
// Created by rain on 07/08/26.
//

#ifndef SIFT_SCHEMA_MAPPING_HPP
#define SIFT_SCHEMA_MAPPING_HPP
#include <string>


namespace sift {
    struct SchemaMapping {
        std::string timestamp_column;
        std::string event_type_column;
        std::string primary_entity_id_column;
    };
}

#endif //SIFT_SCHEMA_MAPPING_HPP
