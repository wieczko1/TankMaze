#include "GameEngine.h"
#include <iostream>

// 1. Constructor
GameEngine::GameEngine()
{
    init();
} 

// 2. The Run Loop
void GameEngine::run()
{
    while (m_running && m_window.isOpen())
    {
        processInput();
        update();
        render();
    }
}

// 3. Initialize the Window
void GameEngine::init()
{
    m_window.create(sf::VideoMode({ 672, 672 }), "Tank Maze");
    m_window.setFramerateLimit(60);
}

// 4. Input Processing
void GameEngine::processInput()
{
    if (m_currentScene)
    {
        m_currentScene->sProcessInput();
    }
}

// 5. Update Game Logic
void GameEngine::update()
{
    if (m_currentScene)
    {
        m_currentScene->sUpdate(1.f/60.f); 
    }
}

// 6. Render
void GameEngine::render()
{
    m_window.clear();

    if (m_currentScene)
    {
        m_currentScene->sRender();
    }

    m_window.display();
}

// 7. Change Scene
void GameEngine::changeScene(const std::string& name, std::shared_ptr<Scene> scene, bool endCurrent)
{
    m_scenes[name] = scene;
    m_currentScene = scene;
}