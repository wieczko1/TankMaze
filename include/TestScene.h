#pragma once
#include "Scene.h"
#include "EntityManager.h"
#include "GameEngine.h"
#include "Components.h"
#include <vector>
#include <random>

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
            {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                {
                    window.close();
                }

                if (keyPressed->scancode == sf::Keyboard::Scancode::P)
                {
                    std::cout << "Pauza (test)" << std::endl;
                }
            }
        }
    }

    void sUpdate(float dt) override
    {
        m_entityManager.update();
    }

    void sRender() override
    {
        m_engine->window().draw(m_masterVertexArray, &m_tilesetTexture);
    }

private:
    EntityManager m_entityManager;
    sf::VertexArray m_masterVertexArray; 
    sf::Texture m_tilesetTexture;        

    const int GRID_WIDTH = 10;
    const int GRID_HEIGHT = 10;
    const float TILE_SIZE = 64.0f;

    void init()
    {
        // 1. Przygotuj teksturê (normalnie wczyta³byœ to przez AssetManager)
        createTestTexture();

        // 2. Utwórz encje w systemie ECS
        createTileEntities();

        // 3. Zaktualizuj managera, aby encje trafi³y do odpowiednich kontenerów
        m_entityManager.update();

        // 4. Pobierz dane z encji i zbuduj jeden wielki VertexArray
        assembleMap();
    }

    void createTileEntities()
    {
        // Generator liczb losowych
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, 1); // 0 = Walkable, 1 = Wall

        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            for (int y = 0; y < GRID_HEIGHT; ++y)
            {
                auto entity = m_entityManager.createEntity("Tile");

                CTile::Type type = CTile::Type::NONE;

                // Logika doboru typu kafelka
                if (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1)
                {
                    type = CTile::Type::MAP_BORDER;
                }
                else
                {
                    type = (distr(gen) == 0) ? CTile::Type::WALKABLE : CTile::Type::WALL;
                }

                // Dodaj komponent logiczny
                // ID obliczamy np. jako indeks w tablicy
                entity->addComponent("CTile", std::make_shared<CTile>(x + y * GRID_WIDTH, type));

                // Obliczanie wspó³rzêdnych tekstury (UV) w zale¿noœci od typu
                // Zak³adam teksturê gdzie kafelki s¹ obok siebie po 32px
                float tx = 0;
                float ty = 0;

                // Proste mapowanie typu na pozycjê w teksturze
                if (type == CTile::Type::MAP_BORDER) tx = 0.0f;       // Czerwony
                else if (type == CTile::Type::WALL)  tx = 32.0f;      // Szary
                else if (type == CTile::Type::WALKABLE) tx = 64.0f;   // Zielony

                // Dodaj komponent graficzny (lokalny VertexArray)
                // Zak³adam, ¿e argument textureID to po prostu nazwa, tu "tileset"
                entity->addComponent("CVertexArray", std::make_shared<CVertexArray>(
                    "CVertexArray",
                    x * TILE_SIZE,      // World X
                    y * TILE_SIZE,      // World Y
                    tx, 0.0f,           // Texture X, Y
                    TILE_SIZE, TILE_SIZE, // Width, Height (w œwiecie)
                    "tileset"           // Texture ID
                ));

                // WA¯NE: W CVertexArray tex coords (width/height) powinny odpowiadaæ rozmiarowi
                // na teksturze (np. 32x32), a nie rozmiarowi w œwiecie (64x64).
                // Jeœli chcesz, aby tekstura by³a ostra (pixel art), musisz dopasowaæ te wartoœci.
                // Tutaj dla uproszczenia przyj¹³em, ¿e skalujemy UV tak samo jak World size,
                // ale w prawdziwej grze UV to zazwyczaj np. 32x32.
                // Poni¿ej poprawka dla UV w komponencie:
                auto vaComp = std::dynamic_pointer_cast<CVertexArray>(entity->getComponent("CVertexArray"));
                if (vaComp)
                {
                    // Nadpisujê UV na mniejsze (32x32), ¿eby pasowa³o do mojej generowanej tekstury
                    vaComp->setTextureCoords(tx, 0.0f, 32.0f, 32.0f);
                }
            }
        }
    }

    // KLUCZOWA FUNKCJA: £¹czenie wszystkiego w jeden draw call
    void assembleMap()
    {
        // Ustaw typ prymitywu na Triangles
        m_masterVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

        // Zarezerwuj pamiêæ: 100 kafelków * 6 wierzcho³ków
        m_masterVertexArray.resize(GRID_WIDTH * GRID_HEIGHT * 6);

        auto& tiles = m_entityManager.getEntitiesByType("Tile");

        int currentVertexIndex = 0;

        for (auto& e : tiles)
        {
            auto vaComp = std::dynamic_pointer_cast<CVertexArray>(e->getComponent("CVertexArray"));
            if (vaComp)
            {
                sf::VertexArray& localVA = vaComp->getVertexArray();

                // Kopiujemy 6 wierzcho³ków z komponentu do g³ównej tablicy
                for (size_t i = 0; i < localVA.getVertexCount(); ++i)
                {
                    m_masterVertexArray[currentVertexIndex + i] = localVA[i];
                }
                currentVertexIndex += 6;
            }
        }
    }

    // Pomocnicza funkcja tworz¹ca teksturê w pamiêci (¿eby nie ³adowaæ pliku do testu)
    void createTestTexture()
    {
        sf::Image img;
        img.resize({ 96, 32 }); // 3 kafelki po 32px

        // 1. Border (Czerwony)
        for (int x = 0; x < 32; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color::Red);
        // 2. Wall (Szary)
        for (int x = 32; x < 64; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color(100, 100, 100));
        // 3. Walkable (Zielony)
        for (int x = 64; x < 96; x++) for (int y = 0; y < 32; y++) img.setPixel({ static_cast<unsigned int>(x), static_cast<unsigned int>(y) }, sf::Color::Green);

        m_tilesetTexture.loadFromImage(img);

        // Wa¿ne dla pixel artu / ostrych krawêdzi
        m_tilesetTexture.setSmooth(false);
    }
};
