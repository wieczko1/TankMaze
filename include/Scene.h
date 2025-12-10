#pragma once
#include <string>

class GameEngine;

class Scene
{
public:
    Scene(GameEngine* engine) : m_engine(engine) {}
    virtual ~Scene() = default;

    virtual void onActivate() {}      // wywo³ywane przy wejœciu do sceny
    virtual void onDeactivate() {}    // przy wyjœciu
    virtual void update(float dt) = 0;
    virtual void sRender() = 0;
    virtual void sDoAction(const std::string& actionName) {}

protected:
    GameEngine* m_engine;  // pozwala scenie korzystaæ z zasobów, zmieniaæ sceny itd.
};
