#pragma once
#include "Scene.h"
#include "EntityManager.h"
#include "GameEngine.h"
#include "Components.h"
#include <vector>
#include <stack>
#include <random>
#include <algorithm>
#include <iostream>
#include <future> // Wymagane do std::async i std::future
#include <chrono>

using namespace std::chrono_literals;

class SceneTest : public Scene
{
public:
    SceneTest(GameEngine* engine) : Scene(engine)
    {
        init();
    }

    void sProcessInput() override
    {
        auto& window = m_engine->window();

        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();

                // R: Uruchom asynchroniczne generowanie (tylko jeœli nie trwa obecnie)
                if (keyPressed->scancode == sf::Keyboard::Scancode::R)
                {
                    if (!m_isGenerating)
                    {
                        std::cout << "[ASYNC] Rozpoczynanie generowania w tle..." << std::endl;
                        startAsyncGeneration();
                    }
                }
            }
        }
    }

    void sUpdate(float dt) override
    {
        // Sprawdzamy stan zadania w tle
        if (m_isGenerating)
        {
            // wait_for(0s) sprawdza status bez blokowania gry!
            if (m_futureMapData.valid() && m_futureMapData.wait_for(0s) == std::future_status::ready)
            {
                std::cout << "[ASYNC] Generowanie zakonczone. Tworzenie swiata..." << std::endl;

                // 1. Pobierz wynik z w¹tku
                auto mapData = m_futureMapData.get();

                // 2. Wyczyœæ stare dane (na g³ównym w¹tku)
                m_entityManager = EntityManager();
                m_masterVertexArray.clear();

                // 3. Utwórz encje na podstawie danych (na g³ównym w¹tku)
                createEntitiesFromData(mapData);

                // 4. Zaktualizuj i zbuduj grafikê
                m_entityManager.update();
                assembleMap();

                m_isGenerating = false;
                std::cout << "[ASYNC] Gotowe!" << std::endl;
            }
        }

        m_entityManager.update();

        // Opcjonalnie: animacja ³adowania
        if (m_isGenerating)
        {
            m_loadingRotation += 360.0f * dt;
        }
    }

    void sRender() override
    {
        // Rysuj mapê
        m_engine->window().draw(m_masterVertexArray, &m_tilesetTexture);

        // Jeœli generujemy, wyœwietl prosty symbol ³adowania (np. obracaj¹cy siê kwadrat)
        if (m_isGenerating)
        {
            sf::RectangleShape loader({ 50.f, 50.f });
            loader.setOrigin({ 25.f, 25.f });
            loader.setPosition({ 1280 / 2.f, 720 / 2.f });
            loader.setFillColor(sf::Color::Blue);
            loader.setRotation(sf::degrees(m_loadingRotation));
            m_engine->window().draw(loader);
        }
    }

private:
    EntityManager m_entityManager;
    sf::VertexArray m_masterVertexArray;
    sf::Texture m_tilesetTexture;

    // Zmienne do obs³ugi asynchronicznoœci
    std::future<std::vector<CTile::Type>> m_futureMapData;
    bool m_isGenerating = false;
    float m_loadingRotation = 0.0f;

    const int GRID_WIDTH = 21;
    const int GRID_HEIGHT = 21;
    const float TILE_SIZE = 32.0f;

    void init()
    {
        createTestTexture();
        startAsyncGeneration(); // Pierwsze generowanie te¿ robimy async, a co!
    }

    void startAsyncGeneration()
    {
        m_isGenerating = true;

        // std::async uruchamia lambdê w nowym w¹tku.
        // Przekazujemy this->GRID_WIDTH itd. przez kopie lub referencje.
        // WA¯NE: W¹tek nie mo¿e dotykaæ m_entityManager ani m_masterVertexArray!
        m_futureMapData = std::async(std::launch::async, [this]()
            {
                // Symulacja ciê¿kiej pracy (opcjonalne, ¿ebyœ zobaczy³ efekt ³adowania)
                // std::this_thread::sleep_for(1s); 

                return generateMazeData(GRID_WIDTH, GRID_HEIGHT);
            });
    }

    // Ta funkcja jest "czysta" - nie u¿ywa stanu klasy (poza argumentami), 
    // wiêc jest bezpieczna dla w¹tków.
    std::vector<CTile::Type> generateMazeData(int width, int height)
    {
        std::vector<CTile::Type> mapData(width * height, CTile::Type::WALL);

        // Ustawianie granic
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                    mapData[x + y * width] = CTile::Type::MAP_BORDER;
                }
            }
        }

        struct Point { int x, y; };
        std::stack<Point> stack;
        Point start = { 1, 1 };
        mapData[start.x + start.y * width] = CTile::Type::WALKABLE;
        stack.push(start);

        // Ka¿dy w¹tek musi mieæ w³asny generator losowy
        std::random_device rd;
        std::mt19937 rng(rd());
        const Point directions[] = { {0, -2}, {0, 2}, {-2, 0}, {2, 0} };

        while (!stack.empty())
        {
            Point current = stack.top();
            std::vector<int> neighbors;

            for (int i = 0; i < 4; ++i)
            {
                int nx = current.x + directions[i].x;
                int ny = current.y + directions[i].y;

                if (nx > 0 && nx < width - 1 && ny > 0 && ny < height - 1)
                {
                    if (mapData[nx + ny * width] == CTile::Type::WALL)
                    {
                        neighbors.push_back(i);
                    }
                }
            }

            if (!neighbors.empty())
            {
                std::shuffle(neighbors.begin(), neighbors.end(), rng);
                int dirIdx = neighbors[0];

                Point next = { current.x + directions[dirIdx].x, current.y + directions[dirIdx].y };

                mapData[next.x + next.y * width] = CTile::Type::WALKABLE;

                Point wall = { current.x + directions[dirIdx].x / 2, current.y + directions[dirIdx].y / 2 };
                mapData[wall.x + wall.y * width] = CTile::Type::WALKABLE;

                stack.push(next);
            }
            else
            {
                stack.pop();
            }
        }
        return mapData;
    }

    // Ta funkcja musi byæ wywo³ana na G£ÓWNYM w¹tku
    void createEntitiesFromData(const std::vector<CTile::Type>& mapData)
    {
        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            for (int y = 0; y < GRID_HEIGHT; ++y)
            {
                CTile::Type type = mapData[x + y * GRID_WIDTH];

                auto entity = m_entityManager.createEntity("Tile");
                entity->addComponent("CTile", std::make_shared<CTile>(x + y * GRID_WIDTH, type));

                float tx = 0;
                if (type == CTile::Type::MAP_BORDER) tx = 0.0f;
                else if (type == CTile::Type::WALL)  tx = 32.0f;
                else if (type == CTile::Type::WALKABLE) tx = 64.0f;

                entity->addComponent("CVertexArray", std::make_shared<CVertexArray>(
                    "CVertexArray",
                    x * TILE_SIZE, y * TILE_SIZE,
                    tx, 0.0f,
                    TILE_SIZE, TILE_SIZE,
                    "tileset"
                ));

                auto vaComp = std::dynamic_pointer_cast<CVertexArray>(entity->getComponent("CVertexArray"));
                if (vaComp) vaComp->setTextureCoords(tx, 0.0f, 32.0f, 32.0f);
            }
        }
    }

    void assembleMap()
    {
        m_masterVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
        m_masterVertexArray.resize(GRID_WIDTH * GRID_HEIGHT * 6);

        auto& tiles = m_entityManager.getEntitiesByType("Tile");
        int currentVertexIndex = 0;

        for (auto& e : tiles)
        {
            auto vaComp = std::dynamic_pointer_cast<CVertexArray>(e->getComponent("CVertexArray"));
            if (vaComp)
            {
                sf::VertexArray& localVA = vaComp->getVertexArray();
                for (size_t i = 0; i < localVA.getVertexCount(); ++i)
                {
                    m_masterVertexArray[currentVertexIndex + i] = localVA[i];
                }
                currentVertexIndex += 6;
            }
        }
    }

    void createTestTexture()
    {
        // Bez zmian (kod z poprzedniej odpowiedzi)
        sf::Image img;
        img.resize({ 96, 32 });
        for (int x = 0; x < 32; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color::Red);
        for (int x = 32; x < 64; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color::Black);
        for (int x = 64; x < 96; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color(200, 200, 200));
        m_tilesetTexture.loadFromImage(img);
        m_tilesetTexture.setSmooth(false);
    }
};