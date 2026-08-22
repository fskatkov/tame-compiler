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
    const std::string_view source_str_view{source_str};

    for (const auto &[report_type, report_message, source_location] : reports) {
        std::string_view report_type_str;
        switch (report_type) {
            case DiagnosticReport::DiagnosticReportType::ERROR:
                report_type_str = "\033[32mcompile-time error:\033[0m "; break;
            case DiagnosticReport::DiagnosticReportType::FATAL:
                report_type_str = "\033[32mruntime error:\033[0m "; break;
            case DiagnosticReport::DiagnosticReportType::WARNING:
                report_type_str = "\033[35mwarning:\033[0m "; break;
        }

        std::println(
            stderr,
            "<stdin>:{}:{}: {}{}",
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

        std::println(stderr, " {} | {}", source_location.line, code_block);

        const std::size_t tilde_position = source_location.length > 0 ? source_location.length : 1;
        std::println(stderr, " {} | {}^{}",
            std::string(source_location.line, ' '),
            std::string(source_location.column > 1 ? source_location.column - 1 : 0, ' '),
            std::string(tilde_position > 1 ? tilde_position - 1 : 0, '~')
        );
    }
}