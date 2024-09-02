#pragma once

#include <string>
#include <bitset>

namespace FakeEngine {
    typedef size_t EntityID;

    const size_t MAX_COMPONENTS = 32;

    typedef std::bitset<MAX_COMPONENTS> EntityMask;

    class Entity {

    private:
        EntityID ID = 0;

        EntityMask componentMask = {};

    public:
        Entity(EntityID ID) :ID(ID) {}

        size_t GetID() { return ID; };

        EntityMask& GetComponentMask() {
            return componentMask;
        };

    };
};
