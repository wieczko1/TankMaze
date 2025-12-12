#pragma once
#include <string>

#include "SFML/Graphics.hpp"

class Component
{
public:
    virtual ~Component() = default;
    std::string name;

protected:
    Component(const std::string& n) : name(n) {}
};

class CTile : public Component
{
public:
    enum class Type
    {
        NONE,
        MAP_BORDER,
        WALL,
        WALKABLE
    };

    int id;
    Type type;

    CTile(int id, Type type)
        : Component("Tile"), id(id), type(type)
    {
    }
};

class CVertexArray : public Component
{
public:
    CVertexArray(const std::string& name, float x, float y,
        float tx, float ty, float width, float height, std::string textureID)
        : Component(name)
    {
        setupQuad(x, y, tx, ty, width, height, textureID);
    }

    sf::VertexArray& getVertexArray() { return m_vertices; }
    std::string getTextureID() const { return textureID; }

    void setupQuad(float x, float y, float tx, float ty, float width, float height, std::string textureID)
    {
        setPositionCoords(x, y, width, height);
        setTextureCoords(tx, ty, width, height);

        this->textureID = textureID;
    }

    void setPositionCoords(float x, float y, float width, float height)
    {
        m_vertices[0].position = sf::Vector2f(x, y);                    // Top-Left
        m_vertices[1].position = sf::Vector2f(x + width, y);            // Top-Right
        m_vertices[2].position = sf::Vector2f(x, y + height);           // Bottom-Left

        m_vertices[3].position = sf::Vector2f(x + width, y);            // Top-Right (kopia)
        m_vertices[4].position = sf::Vector2f(x + width, y + height);   // Bottom-Right
        m_vertices[5].position = sf::Vector2f(x, y + height);           // Bottom-Left (kopia)
    }

    void setTextureCoords(float tx, float ty, float width, float height)
    {
        m_vertices[0].texCoords = sf::Vector2f(tx, ty);
        m_vertices[1].texCoords = sf::Vector2f(tx + width, ty);
        m_vertices[2].texCoords = sf::Vector2f(tx, ty + height);

        m_vertices[3].texCoords = sf::Vector2f(tx + width, ty);
        m_vertices[4].texCoords = sf::Vector2f(tx + width, ty + height);
        m_vertices[5].texCoords = sf::Vector2f(tx, ty + height);
    }

private:
    sf::VertexArray m_vertices{ sf::PrimitiveType::Triangles, 6 };
    std::string textureID;
};