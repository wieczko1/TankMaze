#pragma once
#include <string>
#include <map>
#include <memory>
#include "Components.h"

class EntityManager;

class Entity : public std::enable_shared_from_this<Entity>
{
    friend class EntityManager;

public:
    void destroy() { m_isAlive = false; }
    bool isAlive() const { return m_isAlive; }
    const std::string& type() const { return m_type; }
    size_t id() const { return m_id; }

    void addComponent(const std::string& name, std::shared_ptr<Component> component)
    {
        m_components[name] = component;
    }

    std::shared_ptr<Component> getComponent(const std::string& name)
    {
        if (m_components.count(name))
            return m_components[name];
        return nullptr;
    }

    bool hasComponent(const std::string& name) const
    {
        return m_components.count(name) > 0;
    }

private:
    Entity(const std::string& type, size_t id)
        : m_type(type), m_id(id)
    {
    }

    size_t m_id = 0;
    std::string m_type;
    bool m_isAlive = true;

    std::map<std::string, std::shared_ptr<Component>> m_components;
};



