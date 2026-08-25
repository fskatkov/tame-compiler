#pragma once

#include "diagnostic_engine.h"
#include "tame/frontend/lexer.h"
#include "tame/frontend/parser.h"
#include "tame/backend/compiler.h"
#include "tame/backend/virtual_machine.h"

namespace tame {
    class DriverEngine {
    public:
        static int execute();
    };
}
