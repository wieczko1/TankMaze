#include "TestScene.h"
#include "GameEngine.h"
#include <iostream>

SceneTest::SceneTest(GameEngine* engine) : Scene(engine) {
    init();
}

void SceneTest::init() {
    // Poprawione: assets() zamiast getAssets()
    m_engine->assets().textures.loadTexture("tileset", "assets/graphics/tileset.png");
    m_tilesetTexture = m_engine->assets().textures.getTexture("tileset");

    startAsyncGeneration();
}

void SceneTest::startAsyncGeneration() {
    if (m_isGenerating) return;

    m_isGenerating = true;
    std::cout << "[ASYNC] Start generowania..." << std::endl;

    m_futureMapData = std::async(std::launch::async, [this]() {
        return generateMazeData(GRID_WIDTH, GRID_HEIGHT);
        });
}

void SceneTest::sUpdate(float dt) {
    if (m_isGenerating) {
        if (m_futureMapData.valid() && m_futureMapData.wait_for(0s) == std::future_status::ready) {
            auto mapData = m_futureMapData.get();

            m_entityManager = EntityManager(); // 1. Czyœcimy stare
            createEntitiesFromData(mapData);  
            m_entityManager.update();
            assembleMap();                    // 3. Budujemy VertexArray

            m_isGenerating = false;
        }
        m_loadingRotation += 360.0f * dt;
    }

    m_entityManager.update();
}

void SceneTest::sProcessInput() {
    auto& window = m_engine->window();

    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) window.close();

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            if (keyPressed->scancode == sf::Keyboard::Scancode::R) startAsyncGeneration();
        }
    }
}

void SceneTest::sRender() {
    auto& window = m_engine->window();

   
    if (m_masterVertexArray.getVertexCount() > 0) {
        window.draw(m_masterVertexArray, &m_tilesetTexture);
    } 
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) {
          window.draw(*comp.sprite);
            }
        }
    }

    // 3. Rysowanie loadera (na samym wierzchu)
    if (m_isGenerating) {
        sf::RectangleShape loader({ 60.f, 60.f });
        loader.setOrigin({ 30.f, 30.f });
        loader.setPosition({ window.getSize().x / 2.f, window.getSize().y / 2.f });
        loader.setFillColor(sf::Color::Cyan);
        loader.setRotation(sf::degrees(m_loadingRotation));
        window.draw(loader);
    }
}

std::vector<CTile::Type> SceneTest::generateMazeData(int width, int height) {
    std::vector<CTile::Type> mapData(width * height, CTile::Type::WALL);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
                mapData[x + y * width] = CTile::Type::MAP_BORDER;
        }
    }

    struct Point { int x, y; };
    std::stack<Point> stack;
    stack.push({ 1, 1 });
    mapData[1 + 1 * width] = CTile::Type::WALKABLE;

    std::random_device rd;
    std::mt19937 rng(rd());
    const Point dirs[] = { {0,-2}, {0,2}, {-2,0}, {2,0} };

    while (!stack.empty()) {
        Point curr = stack.top();
        std::vector<int> neighbors;

        for (int i = 0; i < 4; ++i) {
            int nx = curr.x + dirs[i].x, ny = curr.y + dirs[i].y;
            if (nx > 0 && nx < width - 1 && ny > 0 && ny < height - 1) {
                if (mapData[nx + ny * width] == CTile::Type::WALL) neighbors.push_back(i);
            }
        }

        if (!neighbors.empty()) {
            std::shuffle(neighbors.begin(), neighbors.end(), rng);
            int d = neighbors[0];
            Point next = { curr.x + dirs[d].x, curr.y + dirs[d].y };
            Point wall = { curr.x + dirs[d].x / 2, curr.y + dirs[d].y / 2 };

            mapData[next.x + next.y * width] = CTile::Type::WALKABLE;
            mapData[wall.x + wall.y * width] = CTile::Type::WALKABLE;
            stack.push(next);
        }
        else {
            stack.pop();
        }
    }
    return mapData;
}

void SceneTest::createEntitiesFromData(const std::vector<CTile::Type>& mapData) {
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            CTile::Type type = mapData[x + y * GRID_WIDTH];
            auto entity = m_entityManager.createEntity("Tile");
            std::cout << "Tworze encje z danych o rozmiarze: " << mapData.size() << std::endl;
            float tx = (type == CTile::Type::MAP_BORDER) ? 0.0f :
                (type == CTile::Type::WALL) ? 32.0f : 64.0f;

            entity->addComponent("CTile", std::make_shared<CTile>(x + y * GRID_WIDTH, type));

            // POPRAWIONE WYWO£ANIE (8 argumentów):
            entity->addComponent("CVertexArray", std::make_shared<CVertexArray>(
                "CVertexArray",      // const std::string& name
                x * TILE_SIZE,       // float x
                y * TILE_SIZE,       // float y
                tx,                  // float tx
                0.0f,                // float ty
                TILE_SIZE,           // float width
                TILE_SIZE,           // float height
                "tileset"            // std::string textureID
            ));
        }
    }
}

void SceneTest::assembleMap() {
    m_masterVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
    m_masterVertexArray.resize(GRID_WIDTH * GRID_HEIGHT * 6);

    auto& tiles = m_entityManager.getEntitiesByType("Tile");
    std::cout << "Skladam mape z " << tiles.size() << " kafelkow." << std::endl; // Dodaj to!
    size_t vIdx = 0;

    for (auto& e : tiles) {
        auto va = std::dynamic_pointer_cast<CVertexArray>(e->getComponent("CVertexArray"));
        if (va) {
            for (size_t i = 0; i < 6; ++i) {
                m_masterVertexArray[vIdx + i] = va->getVertexArray()[i];
            }
            vIdx += 6;
        }
    }
}