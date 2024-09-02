#pragma once
#include "Component.h"

class GameObject {
private:
    GameObject* parent = NULL;
    std::string name;
    unsigned int id;

public:
    std::vector<GameObject*> children = {};
    std::vector<Component*> components = {};

public:
    GameObject() {};
    GameObject(std::string name):name(name) {};

    std::string GetName() {
        return name;
    }

    GameObject* GetParent() {
        return parent;
    }

    void AddChild(GameObject* child) {
        children.push_back(child);
        child->parent = this;
    };

    void AddComponent(Component* component) {
        components.push_back(component);
    }
    
    virtual void Draw(glm::mat4 lookat) {
        for (int i = 0; i < children.size(); i++) {
            children[i]->Draw(lookat);
        }
    };
};
