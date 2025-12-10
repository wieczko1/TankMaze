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
    // Create a window (1280x720)
    m_window.create(sf::VideoMode({ 1280, 720 }), "Tank Maze");
    m_window.setFramerateLimit(60);
}

// 4. Input Processing
void GameEngine::processInput()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }

        // Example: check for Escape key to close
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
            {
                m_window.close();
            }
        }
    }
}

// 5. Update Game Logic
void GameEngine::update()
{
    // TODO: Update current scene
    if (m_currentScene)
    {
        // m_currentScene->update(); 
    }
}

// 6. Render
void GameEngine::render()
{
    m_window.clear();

    // TODO: Draw current scene
    if (m_currentScene)
    {
        // m_window.draw(*m_currentScene);
    }

    m_window.display();
}

// 7. Change Scene
void GameEngine::changeScene(const std::string& name, std::shared_ptr<Scene> scene, bool endCurrent)
{
    m_scenes[name] = scene;
    m_currentScene = scene;
}