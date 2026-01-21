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
    bool isSolid(int x, int y);
    void sCollision(float dt);
    void sUpdate(float dt) override;
    void sRender() override;
    void sWeapon(float dt);
    bool spawnBullet(std::shared_ptr<Entity> shooter);

private:
    // Podstawowe systemy ECS i renderowania
    EntityManager m_entityManager;
    sf::VertexArray m_masterVertexArray;
    sf::Texture m_tilesetTexture;

    // Asynchroniczne generowanie labiryntu
    std::future<std::vector<CTile::Type>> m_futureMapData;
    bool m_isGenerating = false;
    float m_loadingRotation = 0.0f;
    std::vector<CTile::Type> m_gridMap;

    // --- NOWE ZMIENNE DO WYNIKÓW ---
    int m_scoreP1 = 0;
    int m_scoreP2 = 0;

    std::optional<sf::Text> m_textScoreP1;
    std::optional<sf::Text> m_textScoreP2;
    std::optional<sf::Text> m_textW;

    // WskaŸniki na graczy (¿eby identyfikowaæ ich przy trafieniu)
    std::shared_ptr<Entity> m_player1;
    std::shared_ptr<Entity> m_player2;

    void setupScoreText(); // Funkcja konfiguruj¹ca wygl¹d tekstu

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