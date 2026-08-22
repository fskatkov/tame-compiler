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
    }

    return 0;
}
