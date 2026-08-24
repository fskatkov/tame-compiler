#pragma once

#include "common.h"

namespace tame::diagnostics {
    struct DiagnosticReport {
        enum class DiagnosticReportType { ERROR, FATAL };

        DiagnosticReportType report_type;
        std::string report_message;
        SourceLocation source_location;
    };

    class DiagnosticEngine {
    public:
        explicit DiagnosticEngine() = default;

        bool encountered_error{false};
        std::vector<DiagnosticReport> reports;

        void init(std::string_view source);
        void report(
            DiagnosticReport::DiagnosticReportType report_type,
            const std::string &report_message,
            SourceLocation source_location
        );
        void raise_errors() const;
    private:
        std::string_view source_str;
    };
}