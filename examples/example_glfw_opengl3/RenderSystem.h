#pragma once
#include "Mesh.h"
namespace FakeEngine{
    class RenderSystem {
    public:
        void Update(glm::mat4& lookat) {
            EntityManager& entityManager=EntityManager::GetInstance();
            ComponentManager& componentManager = ComponentManager::GetInstance();
            // 遍历Entities
            for (auto& item : entityManager.GetEntities()) {
                // 快速检测出这个对象是否有指定组件
                if (item.second->GetComponentMask().test(ComponentManager::GetComponentTypeID<Transform>()) &&
                    item.second->GetComponentMask().test(ComponentManager::GetComponentTypeID<Mesh>())) {
                    RenderMesh(*(componentManager.GetComponent<Transform>(item.first)),*(componentManager.GetComponent<Mesh>(item.first)),lookat);
                }
            }
        }

        // Only Care Mesh and Tranform
        void RenderMesh(Transform& transform,Mesh& mesh,glm::mat4& lookat) {
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 projection= glm::mat4(1.0f);
            projection = glm::perspective(glm::radians(45.0f), 1280 / 720.0f, 0.1f, 100.0f);

            mesh.GetShader().use();
            //mesh.GetShader().setMat4("mvp", model * projection * lookat * transform.GetTransformMatx());

            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

        }
    };
};
