#pragma once

#include "diagnostic_engine.h"
#include "tame/frontend/parser.h"
#include "tame/backend/compiler.h"

namespace tame {
    class DriverEngine {
    public:
        static int execute();
    };
}
