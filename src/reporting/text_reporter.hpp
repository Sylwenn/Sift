//
// Created by rain on 17/09/26.
//

#ifndef SIFT_TEXT_REPORTER_HPP
#define SIFT_TEXT_REPORTER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "src/analysis/analysis_types.hpp"

namespace sift {

// Renders the current native output. The format is preserved for compatibility.
std::string render_native_report(const std::string& schema_text,
                                 const std::vector<EntityFeatureSnapshot>& snapshots,
                                 std::size_t event_count);

// Renders measurement and judgment output for the Jev mode. It never labels an
// entity as malicious, compromised, guilty, or high risk.
std::string render_jev_report(const std::string& schema_text,
                              const AnalysisResult& result,
                              std::size_t event_count);

}  // namespace sift

#endif  // SIFT_TEXT_REPORTER_HPP
