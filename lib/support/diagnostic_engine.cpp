#include "tame/support/diagnostic_engine.h"

using namespace tame::diagnostics;

void DiagnosticEngine::init(const std::string &source) {
    source_str = source;
}

void DiagnosticEngine::report(
    DiagnosticReport::DiagnosticReportType report_type,
    const std::string &report_message,
    SourceLocation source_location
) {
    reports.emplace_back(report_type, report_message, source_location);

    switch (report_type) {
        case DiagnosticReport::DiagnosticReportType::ERROR:
        case DiagnosticReport::DiagnosticReportType::FATAL:
            encountered_error = true;
            break;
        default: break;
    }
}

void DiagnosticEngine::raise_errors() const {

}