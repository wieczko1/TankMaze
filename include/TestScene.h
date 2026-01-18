#pragma once

#include "Scene.h"
#include "EntityManager.h"
#include "GameEngine.h"
#include "Components.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <stack>
#include <random>
#include <algorithm>
#include <future>
#include <chrono>
#include <optional>
#include <iostream>

using namespace std::chrono_literals;

class SceneTest : public Scene
{
public:
    SceneTest(GameEngine* engine);

    void sProcessInput() override;
    void sUpdate(float dt) override;
    void sRender() override;

private:
    // Podstawowe systemy ECS i renderowania
    EntityManager m_entityManager;
    sf::VertexArray m_masterVertexArray;
    sf::Texture m_tilesetTexture;

    // Asynchroniczne generowanie labiryntu
    std::future<std::vector<CTile::Type>> m_futureMapData;
    bool m_isGenerating = false;
    float m_loadingRotation = 0.0f;



    // Konfiguracja mapy
    int GRID_WIDTH;
    int GRID_HEIGHT;
    float TILE_SIZE;
    float speed;

    // Metody wewnêtrzne
    void init();
    void spawnPlayers();
    void startAsyncGeneration();
    std::vector<CTile::Type> generateMazeData(int width, int height);
    void createEntitiesFromData(const std::vector<CTile::Type>& mapData);
    void assembleMap();
    void sMovement(float dt);
    // Tu mo¿esz dopisaæ systemy w przysz³oœci
    void sCollision();
    
};