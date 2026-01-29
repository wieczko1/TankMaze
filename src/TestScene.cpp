#include "TestScene.h"
#include "GameEngine.h"
#include <iostream>
#include <optional>

SceneTest::SceneTest(GameEngine* engine) : Scene(engine) {
    init();
}

void SceneTest::init() {

    auto& cfg = m_engine->assets().config;

    GRID_WIDTH = cfg.getInt("GridWidth");
    GRID_HEIGHT = cfg.getInt("GridHeight");


    if (GRID_WIDTH % 2 == 0) {
        GRID_WIDTH += 1;
        std::cout << "[INFO] Skorygowano szerokosc mapy na nieparzysta: " << GRID_WIDTH << std::endl;
    }
    if (GRID_HEIGHT % 2 == 0) {
        GRID_HEIGHT += 1;
        std::cout << "[INFO] Skorygowano wysokosc mapy na nieparzysta: " << GRID_HEIGHT << std::endl;
    }

    TILE_SIZE = cfg.getFloat("TileSize");
    speed = cfg.getFloat("PlayerSpeed");

    std::cout << "--- TEST CONFIGU ---" << std::endl;
    std::cout << "Szerokosc: " << GRID_WIDTH << std::endl;
    std::cout << "Rozmiar kafelka: " << TILE_SIZE << std::endl;
    std::cout << "Predkosc: " << speed << std::endl;
    std::cout << "--------------------" << std::endl;

    std::cout << "Inicjalizacja sceny z predkoscia gracza: " << speed << std::endl;

    m_engine->assets().textures.loadTexture("tileset", "assets/graphics/tileset.png");
    m_engine->assets().textures.loadTexture("ball", "assets/graphics/ball.png");
    m_tilesetTexture = m_engine->assets().textures.getTexture("tileset");

    setupScoreText();

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

            m_gridMap = mapData;

            m_entityManager = EntityManager();
            createEntitiesFromData(mapData);
            m_entityManager.update();
            assembleMap();
            spawnPlayers();
            m_isGenerating = false;
        }
        m_loadingRotation += 360.0f * dt;
    }

    sMovement(dt);
    sCollision(dt);
    sWeapon(dt);

    m_entityManager.update();
}

void SceneTest::sRender() {
    auto& window = m_engine->window();

    if (m_masterVertexArray.getVertexCount() > 0) {
        window.draw(m_masterVertexArray, &m_tilesetTexture);
    }

    if (!m_isGenerating) {
        if (m_textScoreP1.has_value()) window.draw(*m_textScoreP1);
        if (m_textScoreP2.has_value()) window.draw(*m_textScoreP2);
        if (m_textW.has_value()) window.draw(*m_textW);
    }

    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) window.draw(*comp.sprite);
        }
    }

    for (auto& e : m_entityManager.getEntitiesByType("Bullet")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) window.draw(*comp.sprite);
        }
    }

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

    int roomWidth = width / 3;
    int roomHeight = height / 3;

    int startX = (width - roomWidth) / 2;
    int startY = (height - roomHeight) / 2;

    for (int x = startX; x < startX + roomWidth; ++x) {
        for (int y = startY; y < startY + roomHeight; ++y) {
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                mapData[x + y * width] = CTile::Type::WALKABLE;
            }
        }
    }

    this->m_scoreP1 = 0; m_textScoreP1->setString("0");
    this->m_scoreP2 = 0; m_textScoreP2->setString("0");

    return mapData;
}

void SceneTest::createEntitiesFromData(const std::vector<CTile::Type>& mapData) {
    const float TEXTURE_TILE_SIZE = 64.0f;

    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            CTile::Type type = mapData[x + y * GRID_WIDTH];
            auto entity = m_entityManager.createEntity("Tile");


            float tx = (type == CTile::Type::MAP_BORDER) ? 0.0f :
                (type == CTile::Type::WALL) ? 64.0f : 128.0f;

            entity->addComponent("CTile", std::make_shared<CTile>(x + y * GRID_WIDTH, type));


            entity->addComponent("CVertexArray", std::make_shared<CVertexArray>(
                "CVertexArray",
                x * TILE_SIZE,
                y * TILE_SIZE,
                tx,
                0.0f,
                TEXTURE_TILE_SIZE,
                TEXTURE_TILE_SIZE,
                "tileset"
            ));
        }
    }
}

void SceneTest::assembleMap() {
    m_masterVertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
    m_masterVertexArray.resize(GRID_WIDTH * GRID_HEIGHT * 6);

    auto& tiles = m_entityManager.getEntitiesByType("Tile");
    std::cout << "Skladam mape z " << tiles.size() << " kafelkow." << std::endl;
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

void SceneTest::sMovement(float dt) {
    float moveSpeed = (speed > 0.f) ? speed : 200.0f;
    float rotationSpeed = 180.0f;

    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& transform = e->getComponent<CTransform>("CTransform");
        auto& input = e->getComponent<CInput>("CInput");

        transform.velocity = { 0.f, 0.f };

        if (input.left) {
            transform.angle -= rotationSpeed * dt;
        }
        if (input.right) {
            transform.angle += rotationSpeed * dt;
        }

        float direction = 0.0f;
        if (input.up)   direction = 1.0f;
        if (input.down) direction = -1.0f;

        if (direction != 0.0f) {


            float radians = transform.angle * (3.14159265f / 180.0f);


            transform.velocity.x = std::cos(radians) * moveSpeed * direction;
            transform.velocity.y = std::sin(radians) * moveSpeed * direction;
        }
    }
}

void SceneTest::spawnPlayers() {
    auto& tex = m_engine->assets().textures.getTexture("tank");
    sf::Vector2u texSize = tex.getSize();

    auto p1 = m_entityManager.createEntity("Player");
    m_player1 = p1;

    float startX1 = 1 * TILE_SIZE + TILE_SIZE / 2.f;
    float startY1 = 1 * TILE_SIZE + TILE_SIZE / 2.f;

    p1->addComponent("CTransform", std::make_shared<CTransform>(sf::Vector2f(startX1, startY1), sf::Vector2f(0.f, 0.f), 0.f));
    p1->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(TILE_SIZE / 2.0f, TILE_SIZE / 2.0f)));
    p1->addComponent("CInput", std::make_shared<CInput>(
        sf::Keyboard::Scancode::W, sf::Keyboard::Scancode::S,
        sf::Keyboard::Scancode::A, sf::Keyboard::Scancode::D,
        sf::Keyboard::Scancode::Space
    ));

    p1->addComponent("CBurstWeapon", std::make_shared<CBurstWeapon>());
    // -----------------------------

    auto spriteComp1 = std::make_shared<CSprite>(tex);
    spriteComp1->sprite->setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
    spriteComp1->sprite->setScale(sf::Vector2f(0.5f, 0.5f));
    spriteComp1->sprite->setColor(sf::Color(255, 0, 0));
    p1->addComponent("CSprite", spriteComp1);


    auto p2 = m_entityManager.createEntity("Player");
    m_player2 = p2;

    float startX2 = (GRID_WIDTH - 2) * TILE_SIZE + TILE_SIZE / 2.f;
    float startY2 = (GRID_HEIGHT - 2) * TILE_SIZE + TILE_SIZE / 2.f;

    p2->addComponent("CTransform", std::make_shared<CTransform>(sf::Vector2f(startX2, startY2), sf::Vector2f(0.f, 0.f), 0.f));
    p2->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(TILE_SIZE / 2.0f, TILE_SIZE / 2.0f)));
    p2->addComponent("CInput", std::make_shared<CInput>(
        sf::Keyboard::Scancode::Up, sf::Keyboard::Scancode::Down,
        sf::Keyboard::Scancode::Left, sf::Keyboard::Scancode::Right,
        sf::Keyboard::Scancode::RControl
    ));

    p2->addComponent("CBurstWeapon", std::make_shared<CBurstWeapon>());

    auto spriteComp2 = std::make_shared<CSprite>(tex);
    spriteComp2->sprite->setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
    spriteComp2->sprite->setScale(sf::Vector2f(0.5f, 0.5f));
    spriteComp2->sprite->setColor(sf::Color(65, 105, 225));
    p2->addComponent("CSprite", spriteComp2);

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

    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& input = e->getComponent<CInput>("CInput");

        input.up = false; input.down = false; input.left = false; input.right = false; input.shoot = false;

        if (sf::Keyboard::isKeyPressed(input.kUp))    input.up = true;
        if (sf::Keyboard::isKeyPressed(input.kDown))  input.down = true;
        if (sf::Keyboard::isKeyPressed(input.kLeft))  input.left = true;
        if (sf::Keyboard::isKeyPressed(input.kRight)) input.right = true;
        if (sf::Keyboard::isKeyPressed(input.kShoot)) input.shoot = true;
    }
}

bool SceneTest::isSolid(int x, int y) {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return true;

    CTile::Type type = m_gridMap[x + y * GRID_WIDTH];

    return (type == CTile::Type::WALL || type == CTile::Type::MAP_BORDER);
}

void SceneTest::sCollision(float dt) {
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& transform = e->getComponent<CTransform>("CTransform");
        auto& box = e->getComponent<CBoundingBox>("CBoundingBox");

        if (transform.velocity.x != 0 || transform.velocity.y != 0) {

            sf::Vector2f nextPos = transform.pos + transform.velocity * dt;

            float nextX = transform.pos.x + transform.velocity.x * dt;
            float left = nextX - box.halfSize.x;
            float right = nextX + box.halfSize.x;
            float top = transform.pos.y - box.halfSize.y;
            float bottom = transform.pos.y + box.halfSize.y;

            int gridLeft = static_cast<int>(left / TILE_SIZE);
            int gridRight = static_cast<int>(right / TILE_SIZE);
            int gridTop = static_cast<int>(top / TILE_SIZE);
            int gridBottom = static_cast<int>(bottom / TILE_SIZE);

            if (transform.velocity.x > 0) {
                if (isSolid(gridRight, gridTop) || isSolid(gridRight, gridBottom)) {
                    nextX = gridRight * TILE_SIZE - box.halfSize.x - 0.1f;
                }
            }
            else if (transform.velocity.x < 0) {
                if (isSolid(gridLeft, gridTop) || isSolid(gridLeft, gridBottom)) {
                    nextX = (gridLeft + 1) * TILE_SIZE + box.halfSize.x + 0.1f;
                }
            }
            transform.pos.x = nextX;

            float nextY = transform.pos.y + transform.velocity.y * dt;
            left = transform.pos.x - box.halfSize.x;
            right = transform.pos.x + box.halfSize.x;
            top = nextY - box.halfSize.y;
            bottom = nextY + box.halfSize.y;

            gridLeft = static_cast<int>(left / TILE_SIZE);
            gridRight = static_cast<int>(right / TILE_SIZE);
            gridTop = static_cast<int>(top / TILE_SIZE);
            gridBottom = static_cast<int>(bottom / TILE_SIZE);

            if (transform.velocity.y > 0) {
                if (isSolid(gridLeft, gridBottom) || isSolid(gridRight, gridBottom)) {
                    nextY = gridBottom * TILE_SIZE - box.halfSize.y - 0.1f;
                }
            }
            else if (transform.velocity.y < 0) {
                if (isSolid(gridLeft, gridTop) || isSolid(gridRight, gridTop)) {
                    nextY = (gridTop + 1) * TILE_SIZE + box.halfSize.y + 0.1f;
                }
            }
            transform.pos.y = nextY;
        }

        if (e->hasComponent("CSprite")) {
            auto& spriteComp = e->getComponent<CSprite>("CSprite");
            if (spriteComp.sprite) {
                spriteComp.sprite->setPosition(transform.pos);


                spriteComp.sprite->setRotation(sf::degrees(transform.angle + 90.0f));
            }
        }
    }
}

bool SceneTest::spawnBullet(std::shared_ptr<Entity> shooter) {
    auto& shooterTransform = shooter->getComponent<CTransform>("CTransform");

    float bulletSpeed = 600.0f;
    float radians = shooterTransform.angle * (3.14159265f / 180.0f);

    float offsetDist = 40.0f;
    sf::Vector2f offset(std::cos(radians) * offsetDist, std::sin(radians) * offsetDist);
    sf::Vector2f spawnPos = shooterTransform.pos + offset;

    int gx = static_cast<int>(spawnPos.x / TILE_SIZE);
    int gy = static_cast<int>(spawnPos.y / TILE_SIZE);

    if (isSolid(gx, gy)) {
        std::cout << "Lufa zablokowana przez sciane! Nie mozna strzelic." << std::endl;
        return false;
    }

    auto bullet = m_entityManager.createEntity("Bullet");
    bullet->addComponent("CBullet", std::make_shared<CBullet>());

    sf::Vector2f velocity(std::cos(radians) * bulletSpeed, std::sin(radians) * bulletSpeed);

    bullet->addComponent("CTransform", std::make_shared<CTransform>(spawnPos, velocity, shooterTransform.angle));
    bullet->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(10.f, 10.f)));

    auto& tex = m_engine->assets().textures.getTexture("ball");
    auto spriteComp = std::make_shared<CSprite>(tex);
    sf::Vector2u texSize = tex.getSize();
    spriteComp->sprite->setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
    spriteComp->sprite->setScale(sf::Vector2f(0.5f, 0.5f));

    bullet->addComponent("CSprite", spriteComp);

    return true;
}

void SceneTest::sWeapon(float dt) {
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (!e->hasComponent("CBurstWeapon") || !e->hasComponent("CInput")) continue;
        auto& weapon = e->getComponent<CBurstWeapon>("CBurstWeapon");
        auto& input = e->getComponent<CInput>("CInput");

        weapon.timeSinceLastShot += dt;

        if (weapon.burstCooldown > 0) {
            weapon.burstCooldown -= dt;
            if (weapon.burstCooldown <= 0) {
                weapon.shotsFired = 0; weapon.burstCooldown = 0;
                std::cout << "PRZELADOWANO! Gotowy do strzalu." << std::endl;
            }
        }
        else if (input.shoot && weapon.shotsFired < weapon.maxShots && weapon.timeSinceLastShot >= weapon.fireRate) {

            bool shotSuccess = spawnBullet(e);

            if (shotSuccess) {
                weapon.shotsFired++;
                weapon.timeSinceLastShot = 0.f;

                if (weapon.shotsFired >= weapon.maxShots) {
                    weapon.burstCooldown = weapon.burstDelay;
                    std::cout << "Pusty magazynek! Przeladowywanie..." << std::endl;
                }
            }
        }
    }

    auto& players = m_entityManager.getEntitiesByType("Player");

    for (auto& b : m_entityManager.getEntitiesByType("Bullet")) {
        auto& bTransform = b->getComponent<CTransform>("CTransform");
        auto& bBox = b->getComponent<CBoundingBox>("CBoundingBox");
        auto& bullet = b->getComponent<CBullet>("CBullet");

        bullet.lifetime -= dt;
        if (bullet.lifetime <= 0) { b->destroy(); continue; }

        for (auto& p : players) {
            auto& pTransform = p->getComponent<CTransform>("CTransform");
            auto& pBox = p->getComponent<CBoundingBox>("CBoundingBox");

            float dx = std::abs(bTransform.pos.x - pTransform.pos.x);
            float dy = std::abs(bTransform.pos.y - pTransform.pos.y);

            float w = bBox.halfSize.x + pBox.halfSize.x;
            float h = bBox.halfSize.y + pBox.halfSize.y;

            if (dx < w && dy < h) {
                std::cout << "TRAFIENIE! Czolg zniszczony!" << std::endl;


                if (m_player1 && p->id() == m_player1->id()) {
                    m_scoreP2++;
                    m_textScoreP2->setString(std::to_string(m_scoreP2));
                }
                else if (m_player2 && p->id() == m_player2->id()) {
                    m_scoreP1++;
                    m_textScoreP1->setString(std::to_string(m_scoreP1));
                }

                sf::FloatRect b1 = m_textScoreP1->getLocalBounds();
                m_textScoreP1->setOrigin(sf::Vector2f(b1.size.x / 2.f, b1.size.y / 2.f)); // size.x

                sf::FloatRect b2 = m_textScoreP2->getLocalBounds();
                m_textScoreP2->setOrigin(sf::Vector2f(b2.size.x / 2.f, b2.size.y / 2.f)); // size.x
                // -----------------------------

                b->destroy();

                pTransform.pos = pTransform.homePos;
                pTransform.velocity = { 0.f, 0.f };
                pTransform.angle = 0.f;

                if (p->hasComponent("CBurstWeapon")) {
                    auto& wpn = p->getComponent<CBurstWeapon>("CBurstWeapon");
                    wpn.shotsFired = 0;
                    wpn.burstCooldown = 0;
                }

                break;
            }
        }

        if (!b->isAlive()) continue;


        float nextX = bTransform.pos.x + bTransform.velocity.x * dt;
        int gx = static_cast<int>(nextX / TILE_SIZE);
        int gy = static_cast<int>(bTransform.pos.y / TILE_SIZE);

        if (isSolid(gx, gy)) bTransform.velocity.x *= -1.0f;
        else bTransform.pos.x = nextX;

        float nextY = bTransform.pos.y + bTransform.velocity.y * dt;
        gx = static_cast<int>(bTransform.pos.x / TILE_SIZE);
        gy = static_cast<int>(nextY / TILE_SIZE);

        if (isSolid(gx, gy)) bTransform.velocity.y *= -1.0f;
        else bTransform.pos.y = nextY;

        float angleRad = std::atan2(bTransform.velocity.y, bTransform.velocity.x);
        bTransform.angle = angleRad * (180.0f / 3.14159265f);

        if (b->hasComponent("CSprite")) {
            auto& spriteComp = b->getComponent<CSprite>("CSprite");
            if (spriteComp.sprite) {
                spriteComp.sprite->setPosition(bTransform.pos);
                spriteComp.sprite->setRotation(sf::degrees(bTransform.angle + 90.0f));
            }
        }
    }
}

void SceneTest::setupScoreText() {

    const auto& font = m_engine->assets().fonts.getFont("main");

    std::cout << "[DEBUG] Konfiguracja tekstu..." << std::endl;

    m_textScoreP1.emplace(font);
    m_textScoreP1->setString("0");
    m_textScoreP1->setCharacterSize(100);
    m_textScoreP1->setFillColor(sf::Color(255, 0, 0, 125));

    m_textScoreP2.emplace(font);
    m_textScoreP2->setString("0");
    m_textScoreP2->setCharacterSize(100);
    m_textScoreP2->setFillColor(sf::Color(65, 105, 225, 125));

    float centerX = (GRID_WIDTH * TILE_SIZE) / 2.0f;
    float centerY = (GRID_HEIGHT * TILE_SIZE) / 2.0f;

    sf::FloatRect bounds1 = m_textScoreP1->getLocalBounds();
    m_textScoreP1->setOrigin(sf::Vector2f(bounds1.size.x / 2.f, bounds1.size.y / 2.f));
    m_textScoreP1->setPosition(sf::Vector2f(centerX - 60.f, centerY - 35.f));

    sf::FloatRect bounds2 = m_textScoreP2->getLocalBounds();
    m_textScoreP2->setOrigin(sf::Vector2f(bounds2.size.x / 2.f, bounds2.size.y / 2.f));
    m_textScoreP2->setPosition(sf::Vector2f(centerX + 60.f, centerY - 35.f));

    m_textW.emplace(font);
    m_textW->setString("graj na wieczko.pl");
    m_textW->setCharacterSize(50);
    m_textW->setFillColor(sf::Color(0, 0, 0, 125));
    
    sf::FloatRect bounds3 = m_textW->getLocalBounds();
    m_textW->setOrigin(sf::Vector2f(bounds3.size.x / 2.f, bounds3.size.y / 2.f));
    m_textW->setPosition(sf::Vector2f(centerX, centerY - 115.f));

    std::cout << "[DEBUG] Tekst skonfigurowany. Pozycja srodka: " << centerX << ", " << centerY << std::endl;
}
