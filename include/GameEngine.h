#pragma once

#include <memory>
#include <string>
#include <map>

#include <SFML/Graphics.hpp>

#include "AssetManager.h"
#include "Scene.h"

class GameEngine
{
public:
    GameEngine();
    void run();

    void changeScene(const std::string& name, std::shared_ptr<Scene> scene, bool endCurrent = false);

    std::shared_ptr<Scene> getCurrentScene() { return m_currentScene; }
    AssetManager& assets() { return m_assets; }
    sf::RenderWindow& window() { return m_window; }

private:
    sf::RenderWindow m_window;
    AssetManager m_assets;

    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_currentScene;
    bool m_running = true;

    void init();
    void processInput();
    void update();
    void render();
};