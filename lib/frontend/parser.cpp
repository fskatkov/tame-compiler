#include "tame/frontend/parser.h"

using namespace tame::frontend;
using namespace tame::diagnostics;
using namespace tame::ast;

Parser::Parser(std::string source, DiagnosticEngine &diagnostic_engine) : diagnostic_engine(diagnostic_engine) {
    Lexer lexer(std::move(source), diagnostic_engine);
    tokens = lexer.tokenize();
}
