#pragma once
#include <string>

class GameEngine;

class Scene
{
public:
    Scene(GameEngine* engine) : m_engine(engine) {}
    virtual ~Scene() = default;

    virtual void onActivate() {}
    virtual void onDeactivate() {}
    virtual void sProcessInput() = 0;
    virtual void sUpdate(float dt) = 0;
    virtual void sRender() = 0;

protected:
    GameEngine* m_engine;
};
