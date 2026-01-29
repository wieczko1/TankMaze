#pragma once
#include <string>
#include <memory>
#include "SFML/Graphics.hpp"

class Component
{
public:
    virtual ~Component() = default;
    std::string name;
    Component(const std::string& n) : name(n) {}
protected:
    
};



class CBoundingBox : public Component {
public:
    sf::Vector2f size;
    sf::Vector2f halfSize;

    CBoundingBox(const sf::Vector2f& s) : Component("BoundingBox"), size(s), halfSize(s / 2.f) {}
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
        for (size_t i = 0; i < 6; ++i) {
            m_vertices[i].color = sf::Color::White;
        }

        setPositionCoords(x, y, width, height);
        setTextureCoords(tx, ty, width, height);
        this->textureID = textureID;
    }

    void setPositionCoords(float x, float y, float width, float height)
    {
        m_vertices[0].position = sf::Vector2f(x, y);                    
        m_vertices[1].position = sf::Vector2f(x + width, y);            
        m_vertices[2].position = sf::Vector2f(x, y + height);           

        m_vertices[3].position = sf::Vector2f(x + width, y);            
        m_vertices[4].position = sf::Vector2f(x + width, y + height);   
        m_vertices[5].position = sf::Vector2f(x, y + height);           
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

class CTransform : public Component {
public:
    sf::Vector2f pos = { 0.f, 0.f };
    sf::Vector2f velocity = { 0.f, 0.f };
    float angle = 0.f;

    sf::Vector2f homePos = { 0.f, 0.f };

    CTransform(const sf::Vector2f& p, const sf::Vector2f& v, float a)
        : Component("Transform"), pos(p), velocity(v), angle(a), homePos(p) {
    } 
};

class CSprite : public Component {
public:
    std::unique_ptr<sf::Sprite> sprite;

    CSprite() : Component("Sprite"), sprite(nullptr) {}

    CSprite(const sf::Texture& tex)
        : Component("Sprite") {

       
        sprite = std::make_unique<sf::Sprite>(tex);

        sf::Vector2f size = sf::Vector2f(tex.getSize());
        sprite->setOrigin(size / 2.f);
    }
};

class CInput : public Component {
public:
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;


    sf::Keyboard::Scancode kUp;
    sf::Keyboard::Scancode kDown;
    sf::Keyboard::Scancode kLeft;
    sf::Keyboard::Scancode kRight;
    sf::Keyboard::Scancode kShoot;

    CInput() : Component("Input") {}

    CInput(sf::Keyboard::Scancode u, sf::Keyboard::Scancode d,
        sf::Keyboard::Scancode l, sf::Keyboard::Scancode r,
        sf::Keyboard::Scancode s)
        : Component("Input"), kUp(u), kDown(d), kLeft(l), kRight(r), kShoot(s)
    {
    }
};

class CBullet : public Component {
public:
    float lifetime = 10.0f;

    CBullet() : Component("Bullet") {}
};

class CBurstWeapon : public Component {
public:
    int shotsFired = 0;          
    float timeSinceLastShot = 0.f; 
    float burstCooldown = 0.f;   
    bool isBursting = false;     

    const int maxShots = 5;
    const float fireRate = 0.1f;
    const float burstDelay = 3.0f;

    CBurstWeapon() : Component("BurstWeapon") {}
};
