#include "tame/support/driver.h"

using namespace tame::frontend;
using namespace tame::backend;
using namespace tame::diagnostics;

int tame::DriverEngine::execute() {
    DiagnosticEngine diagnostic_engine;
    MetalEngine metal_engine;

    VirtualMachine virtual_machine(diagnostic_engine, metal_engine);

    while (true) {
        std::cout << "> " << std::flush;

        std::string user_input;
        if (!std::getline(std::cin, user_input)) {
            return 1;
        }

        diagnostic_engine.init(user_input);

        Lexer lexer(diagnostic_engine);
        auto tokens = lexer.tokenize(user_input);

        Parser parser(diagnostic_engine);
        auto statements = parser.run(tokens);

        LoweringEngine lowering_engine;
        lowering_engine.process(statements);

        Compiler compiler(metal_engine);
        auto code_buffer = compiler.run(statements);

        virtual_machine.execute(std::move(code_buffer));

        diagnostic_engine.raise_errors();
    }

    return 0;
}
