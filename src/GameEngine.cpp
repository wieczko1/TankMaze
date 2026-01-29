#include "GameEngine.h"
#include <iostream>

GameEngine::GameEngine()
{
    init();
} 

void GameEngine::run()
{
    while (m_running && m_window.isOpen())
    {
        processInput();
        update();
        render();
    }
}

void GameEngine::init()
{
    m_assets.loadAll();
    int width = m_assets.config.getInt("WindowWidth");
    int height = m_assets.config.getInt("WindowHeight");
    m_window.create(sf::VideoMode({ (unsigned int)width, (unsigned int)height }), "Tank Maze");
    m_window.setFramerateLimit(60);
    m_assets.loadAll();
}

void GameEngine::processInput()
{
    if (m_currentScene)
    {
        m_currentScene->sProcessInput();
    }
}

void GameEngine::update()
{
    if (m_currentScene)
    {
        m_currentScene->sUpdate(1.f/60.f); 
    }
}

void GameEngine::render()
{
    m_window.clear();

    if (m_currentScene)
    {
        m_currentScene->sRender();
    }

    m_window.display();
}

void GameEngine::changeScene(const std::string& name, std::shared_ptr<Scene> scene, bool endCurrent)
{
    m_scenes[name] = scene;
    m_currentScene = scene;
}
