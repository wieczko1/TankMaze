#pragma once
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <iostream>
#include "Entity.h"

typedef std::vector<std::shared_ptr<Entity>> EntityList;
typedef std::map<std::string, EntityList> EntityMap;

class EntityManager
{
public:
    std::shared_ptr<Entity> createEntity(const std::string& type)
    {
        auto e = std::shared_ptr<Entity>(new Entity(type, m_totalEntities++));
        m_toAdd.push_back(e);
        std::cout << "[EntityManager] Created entity ID " << e->id() << " of type \"" << type << "\"\n";
        return e;
    }

    void update()
    {
        for (auto& e : m_toAdd)
        {
            m_entities.push_back(e);
            m_entityMap[e->type()].push_back(e);
            std::cout << "[EntityManager] Added entity ID " << e->id() << " of type \"" << e->type() << "\" to manager\n";
        }
        m_toAdd.clear();

        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                [](const std::shared_ptr<Entity>& e) { return !e->isAlive(); }),
            m_entities.end());

        for (auto& [type, list] : m_entityMap)
        {
            list.erase(
                std::remove_if(list.begin(), list.end(),
                    [](const std::shared_ptr<Entity>& e) { return !e->isAlive(); }),
                list.end());
        }


        for (auto it = m_entityMap.begin(); it != m_entityMap.end(); )
        {
            if (it->second.empty())
                it = m_entityMap.erase(it);
            else
                ++it;

        }
    }

    EntityList& getAllEntities() { return m_entities; }
    EntityList& getEntitiesByType(const std::string& type) { return m_entityMap[type]; }

private:
    EntityList m_entities;
    EntityList m_toAdd;
    EntityMap m_entityMap;
    size_t m_totalEntities = 0;
};

