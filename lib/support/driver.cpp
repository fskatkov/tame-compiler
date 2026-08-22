#include "tame/support/driver.h"

using namespace tame::diagnostics;

int tame::DriverEngine::execute() {
    while (true) {
        DiagnosticEngine diagnostic_engine;
        std::cout << "> " << std::flush;

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            return 1;
        }

        diagnostic_engine.init(user_input);
        frontend::Lexer lexer(user_input, diagnostic_engine);
        const auto tokens = lexer.tokenize();
        diagnostic_engine.raise_errors();
    }

    return 0;
}
