#include "tame/support/driver.h"

using namespace tame::diagnostics;

int tame::DriverEngine::execute() {
    backend::VirtualMachine virtual_machine;

    while (true) {
        DiagnosticEngine diagnostic_engine;

        std::cout << "> " << std::flush;

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            return 1;
        }

        diagnostic_engine.init(user_input);

        frontend::Lexer lexer(diagnostic_engine);
        auto tokens = lexer.tokenize(user_input);

        frontend::Parser parser(diagnostic_engine);
        auto statements = parser.run(tokens);

        backend::Compiler compiler;
        auto code_buffer = compiler.run(statements);

        virtual_machine.execute(std::move(code_buffer));

        diagnostic_engine.raise_errors();
    }

    return 0;
}
