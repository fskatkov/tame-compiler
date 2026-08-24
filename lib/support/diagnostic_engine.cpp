#include "tame/support/diagnostic_engine.h"

using namespace tame::diagnostics;

void DiagnosticEngine::init(std::string_view source) {
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
    const std::string_view source_str_view{source_str};

    for (const auto &[report_type, report_message, source_location] : reports) {
        std::string_view report_type_str;
        switch (report_type) {
            case DiagnosticReport::DiagnosticReportType::ERROR:
                report_type_str = "compile-time error: "; break;
            case DiagnosticReport::DiagnosticReportType::FATAL:
                report_type_str = "runtime error: "; break;
        }

        std::println(
            stdout,
            "\033[31m<stdin>:{}:{}: {}{}",
            source_location.line,
            source_location.column,
            report_type_str,
            report_message
        );

        std::string_view code_block = source_str_view;

        for (std::size_t i = 1; i < source_location.line; ++i) {
            const auto new_line_position = code_block.find('\n');

            if (new_line_position == std::string_view::npos) {
                code_block = {};
                break;
            }

            code_block.remove_prefix(new_line_position + 1);
        }

        code_block = code_block.substr(0, code_block.find('\n'));

        const std::string stringified_line = std::to_string(source_location.line);
        std::println(stdout, " {} | {}", stringified_line, code_block);

        const std::string padding(stringified_line.length(), ' ');
        const std::string spaces(source_location.column > 1 ? source_location.column - 1 : 0, ' ');

        const std::size_t tilde_position = source_location.length > 0 ? source_location.length : 1;
        const std::string tildes(tilde_position > 1 ? tilde_position - 1 : 0, '~');

        std::println(stdout, " {} | {}^{}\033[0m", padding, spaces, tildes);
    }
}