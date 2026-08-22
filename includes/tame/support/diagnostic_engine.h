#pragma once

#include "common.h"

namespace tame::diagnostics {
    struct DiagnosticReport {
        enum class DiagnosticReportType {
            WARNING, ERROR, FATAL
        };

        DiagnosticReportType report_type;
        std::string report_message;
        SourceLocation source_location;
    };

    class DiagnosticEngine {
    public:
        explicit DiagnosticEngine() = default;

        bool encountered_error{false};

        void init(const std::string &source);
        void report(
            DiagnosticReport::DiagnosticReportType report_type,
            const std::string &report_message,
            SourceLocation source_location
        );
        void raise_errors() const;
    private:
        std::vector<DiagnosticReport> reports;
        std::string source_str;
    };
}