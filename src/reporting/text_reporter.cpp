//
// Created by rain on 17/09/26.
//
#include "src/reporting/text_reporter.hpp"

#include <sstream>

namespace sift {

std::string render_native_report(
    const std::string& schema_text,
    const std::vector<EntityFeatureSnapshot>& snapshots,
    std::size_t event_count) {
    std::ostringstream output;
    output << schema_text << '\n';
    output << "events: " << event_count << '\n';
    if (event_count != 0) {
        for (const auto& snapshot : snapshots) {
            output << snapshot.entity_id << " daily_exports:";
            for (const auto& dated : snapshot.daily_export_counts) {
                output << ' ' << dated.count;
            }
            output << '\n';
        }
    }
    return output.str();
}

std::string render_jev_report(const std::string& schema_text,
                              const AnalysisResult& result,
                              std::size_t event_count) {
    std::ostringstream output;
    output << schema_text << '\n';
    output << "events: " << event_count << '\n';
    for (const auto& item : result.items) {
        for (const auto& judgment : item.judgments) {
            output << item.entity_id << ' ' << judgment.judgment_id << ": "
                   << judgment.selected_value;
            for (const auto& [option, probability] : judgment.probabilities) {
                output << " p(" << option << ")=" << probability;
            }
            if (judgment.confidence) {
                output << " confidence=" << *judgment.confidence;
            }
            output << '\n';
        }
    }
    return output.str();
}

}  // namespace sift
