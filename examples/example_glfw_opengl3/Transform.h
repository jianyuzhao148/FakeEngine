#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace FakeEngine {
    class Transform :public Component {
    public:
        glm::vec3 position{0.0f};
        glm::vec3 rotation{0.0f};
        glm::vec3 scale{1.0f};

    //public:
    //    glm::mat4 GetTransformMatx() {
    //        glm::mat4 transformMatx = glm::mat4(1.0f);
    //        transformMatx = glm::scale(transformMatx, scale);
    //        transformMatx = glm::rotate(transformMatx, glm::radians(rotation.x), {1.0f,0.0f,0.0f});
    //        transformMatx = glm::rotate(transformMatx, glm::radians(rotation.y), {0.0f,1.0f,0.0f});
    //        transformMatx = glm::rotate(transformMatx, glm::radians(rotation.z), {1.0f,0.0f,1.0f});
    //        transformMatx = glm::translate(transformMatx, position);
    //        return transformMatx;
    //    }
    };
};
