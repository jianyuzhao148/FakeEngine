#pragma once

#include <memory>
#include "Component.h"
#include "EntityManager.h"

namespace FakeEngine {
    typedef size_t ComponentTypeID;

    class ComponentManager {

    public:
        std::unordered_map<ComponentTypeID, std::unordered_map<EntityID, std::unique_ptr<Component>>> components;

    public:
        static ComponentManager& GetInstance() {
            static ComponentManager instance;
            return instance;
        }

        template <typename T>
        void AddComponent(EntityID entityID, std::unique_ptr<T> component) {
            auto& componentMap = components[GetComponentTypeID<T>()];// unordered_map ������Խ�磬�Զ�������Ԫ��
            componentMap[entityID] = std::move(component);// ��ʹ��move���벻��
            UpdateEntityMask(entityID);
        }

        template <typename T>
        T* GetComponent(EntityID entityID) {
            auto& componentMap = components[GetComponentTypeID<T>()];
            auto it = componentMap.find(entityID);
            if (it != componentMap.end()) {
                return (T*)it->second.get();// ����ת����
            }
            return nullptr;
        }

        template <typename T>
        void removeComponent(EntityID entityID) {
            auto& componentMap = components[GetComponentTypeID<T>()];
            componentMap.erase(entityID);
            UpdateEntityMask(entityID);
        }

        // ���ɵ㣺ģ�庯������ÿ��ģ��ʵ������ͬ��T��������һ�������ľ�̬�ֲ�����typeID��
        // ����һ�ε��ú���ʱtypeID�ᱻ��ʼ��������ͬ����T���۵��ö��ٴζ����ֳ�ʼ����ֵ
        template <typename T>
        static ComponentTypeID GetComponentTypeID() {
            static ComponentTypeID typeID = nextTypeID++;
            return typeID;
        }

    private:
        static ComponentTypeID nextTypeID;

    private:
         ComponentManager() {};
         ComponentManager(const ComponentManager&) = delete;
         ComponentManager& operator=(const ComponentManager&) = delete;

        void UpdateEntityMask(EntityID entityID) {
            Entity* entity = EntityManager::GetInstance().GetEntity(entityID);
            if (entity) {
                std::bitset<MAX_COMPONENTS> mask = {};
                for (const auto& item : components) {
                    if (item.second.find(entityID) != item.second.end()) {
                        mask.set(item.first);
                    }
                }
                entity->GetComponentMask() = mask;
            }
        }
    };
    ComponentTypeID ComponentManager::nextTypeID = 0;
};
