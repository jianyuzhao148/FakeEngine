#pragma once

#include "Component.h"
#include <string>

namespace FakeEngine {
    class Hierarchy :public Component {
    public:
        std::string name;
        EntityID parent;
    };
};
