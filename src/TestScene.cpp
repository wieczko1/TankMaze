#include "TestScene.h"
#include "GameEngine.h"
#include <iostream>

SceneTest::SceneTest(GameEngine* engine) : Scene(engine) {
    init();
}

void SceneTest::init() {

    auto& cfg = m_engine->assets().config;

    GRID_WIDTH = cfg.getInt("GridWidth");
    GRID_HEIGHT = cfg.getInt("GridHeight");

    // --- POPRAWKA: WYMUSZENIE LICZB NIEPARZYSTYCH ---
    // Algorytm labiryntu wymaga nieparzystych wymiarów, aby dojœæ do samej krawêdzi.
    // Jeœli w configu jest liczba parzysta (np. 20), zmieniamy j¹ na 21.
    if (GRID_WIDTH % 2 == 0) {
        GRID_WIDTH += 1;
        std::cout << "[INFO] Skorygowano szerokosc mapy na nieparzysta: " << GRID_WIDTH << std::endl;
    }
    if (GRID_HEIGHT % 2 == 0) {
        GRID_HEIGHT += 1;
        std::cout << "[INFO] Skorygowano wysokosc mapy na nieparzysta: " << GRID_HEIGHT << std::endl;
    }
    // ------------------------------------------------

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
            // Pobieramy dane
            auto mapData = m_futureMapData.get();

            // --- NOWOŒÆ: Zapisujemy mapê do zmiennej klasy dla kolizji ---
            m_gridMap = mapData;
            // -------------------------------------------------------------

            m_entityManager = EntityManager();
            createEntitiesFromData(mapData);
            m_entityManager.update();
            assembleMap();
            spawnPlayers();
            m_isGenerating = false;
        }
        m_loadingRotation += 360.0f * dt;
    }

    sMovement(dt); // Tylko ustawia prêdkoœæ
    sCollision(dt); // NOWOŒÆ: Sprawdza kolizje i przesuwa
    sWeapon(dt);

    m_entityManager.update();
}

void SceneTest::sRender() {
    auto& window = m_engine->window();

    // 1. Rysowanie mapy (t³o)
    if (m_masterVertexArray.getVertexCount() > 0) {
        window.draw(m_masterVertexArray, &m_tilesetTexture);
    }

    // 2. Rysowanie Graczy
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) {
                window.draw(*comp.sprite);
            }
        }
    }

    // --- NOWOŒÆ: Rysowanie Kulek ---
    for (auto& e : m_entityManager.getEntitiesByType("Bullet")) {
        if (e->hasComponent("CSprite")) {
            // Pamiêtaj o kropce zamiast strza³ki przy getComponent!
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) {
                window.draw(*comp.sprite);
            }
        }
    }
    // -------------------------------

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
    // 1. Wype³nij wszystko œcianami
    std::vector<CTile::Type> mapData(width * height, CTile::Type::WALL);

    // Ustaw granice mapy (nieruszalne)
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
                mapData[x + y * width] = CTile::Type::MAP_BORDER;
        }
    }

    // 2. ALGORYTM GENEROWANIA (DFS) - to zostaje bez zmian
    struct Point { int x, y; };
    std::stack<Point> stack;

    // Startujemy standardowo
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
            // Sprawdzamy zakres (z marginesem na œciany graniczne)
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

    // 3. NOWOŒÆ: WYCINANIE PUSTEGO PLACU NA ŒRODKU (1/3 wielkoœci mapy)
    int roomWidth = width / 3;
    int roomHeight = height / 3;

    // Oblicz lewy górny róg placu, ¿eby by³ wyœrodkowany
    int startX = (width - roomWidth) / 2;
    int startY = (height - roomHeight) / 2;

    for (int x = startX; x < startX + roomWidth; ++x) {
        for (int y = startY; y < startY + roomHeight; ++y) {
            // Zabezpieczenie, ¿eby nie nadpisaæ MAP_BORDER (granic mapy)
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                mapData[x + y * width] = CTile::Type::WALKABLE;
            }
        }
    }

    return mapData;
}

void SceneTest::createEntitiesFromData(const std::vector<CTile::Type>& mapData) {
    // 1. Definiujemy, ¿e kafelek W PLIKU GRAFICZNYM ma 64 piksele
    const float TEXTURE_TILE_SIZE = 64.0f;

    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            CTile::Type type = mapData[x + y * GRID_WIDTH];
            auto entity = m_entityManager.createEntity("Tile");

            // 2. Obliczamy pozycjê X na teksturze (tx)
            // 0 * 64 = 0
            // 1 * 64 = 64
            // 2 * 64 = 128
            float tx = (type == CTile::Type::MAP_BORDER) ? 0.0f :
                (type == CTile::Type::WALL) ? 64.0f : 128.0f;

            entity->addComponent("CTile", std::make_shared<CTile>(x + y * GRID_WIDTH, type));

            // 3. Tworzymy VertexArray
            // Kluczowe s¹ 4 ostatnie argumenty: 
            // tx, 0.0f -> sk¹d zacz¹æ wycinaæ z png
            // TEXTURE_TILE_SIZE, TEXTURE_TILE_SIZE -> jak du¿y kawa³ek wyci¹æ (64x64)
            entity->addComponent("CVertexArray", std::make_shared<CVertexArray>(
                "CVertexArray",
                x * TILE_SIZE,       // Gdzie na ekranie (X)
                y * TILE_SIZE,       // Gdzie na ekranie (Y)
                tx,                  // Pozycja X na obrazku (0, 64 lub 128)
                0.0f,                // Pozycja Y na obrazku (zawsze 0, bo masz jeden rz¹d)
                TEXTURE_TILE_SIZE,   // Szerokoœæ wycinka (64)
                TEXTURE_TILE_SIZE,   // Wysokoœæ wycinka (64)
                "tileset"
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

void SceneTest::sMovement(float dt) {
    // Prêdkoœæ poruszania (piksele na sekundê)
    float moveSpeed = (speed > 0.f) ? speed : 200.0f;
    // Prêdkoœæ obrotu (stopnie na sekundê)
    float rotationSpeed = 180.0f;

    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& transform = e->getComponent<CTransform>("CTransform");
        auto& input = e->getComponent<CInput>("CInput");

        // 1. Resetujemy prêdkoœæ (ale k¹t zostaje!)
        transform.velocity = { 0.f, 0.f };

        // 2. Obs³uga obrotu (Lewo/Prawo)
        if (input.left) {
            transform.angle -= rotationSpeed * dt;
        }
        if (input.right) {
            transform.angle += rotationSpeed * dt;
        }

        // 3. Obs³uga jazdy (Góra/Dó³)
        float direction = 0.0f;
        if (input.up)   direction = 1.0f;  // Do przodu
        if (input.down) direction = -1.0f; // Do ty³u

        if (direction != 0.0f) {
            // Musimy zamieniæ k¹t (w stopniach) na radiany dla funkcji matematycznych
            // W SFML i matematyce: 
            // 0 stopni to zazwyczaj "Prawo" (oœ X). 
            // Jeœli Twój sprite czo³gu w pliku PNG jest skierowany w GÓRÊ, musimy odj¹æ 90 stopni do obliczeñ.
            // Zak³adam tutaj standard: 0 stopni = wektor w prawo.

            // Konwersja stopni na radiany: rad = deg * PI / 180
            float radians = transform.angle * (3.14159265f / 180.0f);

            // Obliczamy wektor przesuniêcia
            // cos(k¹t) daje sk³adow¹ X, sin(k¹t) daje sk³adow¹ Y
            transform.velocity.x = std::cos(radians) * moveSpeed * direction;
            transform.velocity.y = std::sin(radians) * moveSpeed * direction;
        }
    }
}

void SceneTest::spawnPlayers() {
    auto& tex = m_engine->assets().textures.getTexture("tank");
    sf::Vector2u texSize = tex.getSize();

    // --- GRACZ 1 ---
    auto p1 = m_entityManager.createEntity("Player");
    // ... (Twoja konfiguracja Transform, BoundingBox, Input, Sprite bez zmian) ...

    // START KOPIOWANIA (Reszta bez zmian)
    float startX1 = 1 * TILE_SIZE + TILE_SIZE / 2.f;
    float startY1 = 1 * TILE_SIZE + TILE_SIZE / 2.f;

    p1->addComponent("CTransform", std::make_shared<CTransform>(sf::Vector2f(startX1, startY1), sf::Vector2f(0.f, 0.f), 0.f));
    p1->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(TILE_SIZE / 2.0f, TILE_SIZE / 2.0f)));
    p1->addComponent("CInput", std::make_shared<CInput>(
        sf::Keyboard::Scancode::W, sf::Keyboard::Scancode::S,
        sf::Keyboard::Scancode::A, sf::Keyboard::Scancode::D,
        sf::Keyboard::Scancode::Space
    ));

    // --- NOWOŒÆ: Dodajemy broñ ---
    p1->addComponent("CBurstWeapon", std::make_shared<CBurstWeapon>());
    // -----------------------------

    auto spriteComp1 = std::make_shared<CSprite>(tex);
    spriteComp1->sprite->setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
    spriteComp1->sprite->setScale(sf::Vector2f(0.5f, 0.5f));
    spriteComp1->sprite->setColor(sf::Color(255, 0, 0));
    p1->addComponent("CSprite", spriteComp1);


    // --- GRACZ 2 ---
    auto p2 = m_entityManager.createEntity("Player");
    // ... (Transform, BoundingBox, Input bez zmian) ...

    float startX2 = (GRID_WIDTH - 2) * TILE_SIZE + TILE_SIZE / 2.f;
    float startY2 = (GRID_HEIGHT - 2) * TILE_SIZE + TILE_SIZE / 2.f;

    p2->addComponent("CTransform", std::make_shared<CTransform>(sf::Vector2f(startX2, startY2), sf::Vector2f(0.f, 0.f), 0.f));
    p2->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(TILE_SIZE / 2.0f, TILE_SIZE / 2.0f)));
    p2->addComponent("CInput", std::make_shared<CInput>(
        sf::Keyboard::Scancode::Up, sf::Keyboard::Scancode::Down,
        sf::Keyboard::Scancode::Left, sf::Keyboard::Scancode::Right,
        sf::Keyboard::Scancode::RControl
    ));

    // --- NOWOŒÆ: Dodajemy broñ ---
    p2->addComponent("CBurstWeapon", std::make_shared<CBurstWeapon>());
    // -----------------------------

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

        // Obs³uga klawiszy systemowych (ESC, R)
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) window.close();
            if (keyPressed->scancode == sf::Keyboard::Scancode::R) startAsyncGeneration();
        }
    }

    // Ci¹g³e sprawdzanie stanu klawiatury dla ka¿dego gracza
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& input = e->getComponent<CInput>("CInput");

        // Resetujemy stan
        input.up = false; input.down = false; input.left = false; input.right = false; input.shoot = false;

        // Sprawdzamy czy klawisze zdefiniowane w komponencie s¹ wciœniête
        if (sf::Keyboard::isKeyPressed(input.kUp))    input.up = true;
        if (sf::Keyboard::isKeyPressed(input.kDown))  input.down = true;
        if (sf::Keyboard::isKeyPressed(input.kLeft))  input.left = true;
        if (sf::Keyboard::isKeyPressed(input.kRight)) input.right = true;
        if (sf::Keyboard::isKeyPressed(input.kShoot)) input.shoot = true;
    }
}

bool SceneTest::isSolid(int x, int y) {
    // 1. Zabezpieczenie przed wyjœciem poza tablicê
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return true;

    // 2. Pobierz typ kafelka z zapamiêtanej mapy
    CTile::Type type = m_gridMap[x + y * GRID_WIDTH];

    // 3. Zwróæ true jeœli to œciana lub granica
    return (type == CTile::Type::WALL || type == CTile::Type::MAP_BORDER);
}

void SceneTest::sCollision(float dt) {
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        auto& transform = e->getComponent<CTransform>("CTransform");
        auto& box = e->getComponent<CBoundingBox>("CBoundingBox");

        // --- CZÊŒÆ FIZYCZNA (Tylko jeœli siê rusza) ---
        if (transform.velocity.x != 0 || transform.velocity.y != 0) {

            // Obliczamy przysz³¹ pozycjê
            sf::Vector2f nextPos = transform.pos + transform.velocity * dt;

            // --- ETAP 1: KOLIZJA PO OSI X ---
            float nextX = transform.pos.x + transform.velocity.x * dt;
            float left = nextX - box.halfSize.x;
            float right = nextX + box.halfSize.x;
            float top = transform.pos.y - box.halfSize.y;
            float bottom = transform.pos.y + box.halfSize.y;

            int gridLeft = static_cast<int>(left / TILE_SIZE);
            int gridRight = static_cast<int>(right / TILE_SIZE);
            int gridTop = static_cast<int>(top / TILE_SIZE);
            int gridBottom = static_cast<int>(bottom / TILE_SIZE);

            if (transform.velocity.x > 0) { // PRAWO
                if (isSolid(gridRight, gridTop) || isSolid(gridRight, gridBottom)) {
                    nextX = gridRight * TILE_SIZE - box.halfSize.x - 0.1f;
                }
            }
            else if (transform.velocity.x < 0) { // LEWO
                if (isSolid(gridLeft, gridTop) || isSolid(gridLeft, gridBottom)) {
                    nextX = (gridLeft + 1) * TILE_SIZE + box.halfSize.x + 0.1f;
                }
            }
            transform.pos.x = nextX; // Aplikujemy X

            // --- ETAP 2: KOLIZJA PO OSI Y ---
            float nextY = transform.pos.y + transform.velocity.y * dt;
            // Odœwie¿amy X (bo ju¿ siê zmieni³)
            left = transform.pos.x - box.halfSize.x;
            right = transform.pos.x + box.halfSize.x;
            top = nextY - box.halfSize.y;
            bottom = nextY + box.halfSize.y;

            gridLeft = static_cast<int>(left / TILE_SIZE);
            gridRight = static_cast<int>(right / TILE_SIZE);
            gridTop = static_cast<int>(top / TILE_SIZE);
            gridBottom = static_cast<int>(bottom / TILE_SIZE);

            if (transform.velocity.y > 0) { // DÓ£
                if (isSolid(gridLeft, gridBottom) || isSolid(gridRight, gridBottom)) {
                    nextY = gridBottom * TILE_SIZE - box.halfSize.y - 0.1f;
                }
            }
            else if (transform.velocity.y < 0) { // GÓRA
                if (isSolid(gridLeft, gridTop) || isSolid(gridRight, gridTop)) {
                    nextY = (gridTop + 1) * TILE_SIZE + box.halfSize.y + 0.1f;
                }
            }
            transform.pos.y = nextY; // Aplikujemy Y
        }

        // --- CZÊŒÆ GRAFICZNA (Zawsze!) ---
        if (e->hasComponent("CSprite")) {
            auto& spriteComp = e->getComponent<CSprite>("CSprite");
            if (spriteComp.sprite) {
                // Ustawiamy pozycjê
                spriteComp.sprite->setPosition(transform.pos);

                // NOWOŒÆ: Ustawiamy rotacjê
                // Jeœli Twój obrazek w pliku .png jest skierowany w GÓRÊ, a matematyka zak³ada 0 stopni = PRAWO,
                // to musisz dodaæ +90 stopni do rotacji sprite'a, ¿eby siê zgadza³o.
                // Spróbuj: transform.angle LUB transform.angle + 90.0f
                spriteComp.sprite->setRotation(sf::degrees(transform.angle + 90.0f));
            }
        }
    }
}

void SceneTest::spawnBullet(std::shared_ptr<Entity> shooter) {
    auto& shooterTransform = shooter->getComponent<CTransform>("CTransform");

    // Tworzymy kulê
    auto bullet = m_entityManager.createEntity("Bullet");
    bullet->addComponent("CBullet", std::make_shared<CBullet>());

    // 1. Obliczamy pozycjê i prêdkoœæ
    float bulletSpeed = 600.0f; // Szybkoœæ kuli

    // Konwersja k¹ta gracza na radiany
    float radians = shooterTransform.angle * (3.14159265f / 180.0f);

    // Wektor prêdkoœci (taki sam jak kierunek patrzenia czo³gu)
    sf::Vector2f velocity(std::cos(radians) * bulletSpeed, std::sin(radians) * bulletSpeed);

    // Transform: Pozycja startowa to pozycja czo³gu, K¹t ten sam co czo³g
    bullet->addComponent("CTransform", std::make_shared<CTransform>(shooterTransform.pos, velocity, shooterTransform.angle));

    // BoundingBox dla kuli (ma³a, np. 10x10)
    bullet->addComponent("CBoundingBox", std::make_shared<CBoundingBox>(sf::Vector2f(10.f, 10.f)));

    // Sprite
    auto& tex = m_engine->assets().textures.getTexture("ball");
    auto spriteComp = std::make_shared<CSprite>(tex);

    // Ustawiamy origin na œrodek kulki
    sf::Vector2u texSize = tex.getSize();
    spriteComp->sprite->setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
    // Skalujemy, jeœli kulka jest za du¿a
    spriteComp->sprite->setScale(sf::Vector2f(0.5f, 0.5f));

    bullet->addComponent("CSprite", spriteComp);
}

void SceneTest::sWeapon(float dt) {
    // 1. Logika strzelania (BEZ ZMIAN - skopiowane z poprzedniej wersji)
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (!e->hasComponent("CBurstWeapon") || !e->hasComponent("CInput")) continue;

        auto& weapon = e->getComponent<CBurstWeapon>("CBurstWeapon");
        auto& input = e->getComponent<CInput>("CInput");

        weapon.timeSinceLastShot += dt;

        if (weapon.burstCooldown > 0) {
            weapon.burstCooldown -= dt;
            if (weapon.burstCooldown <= 0) {
                weapon.shotsFired = 0;
                weapon.burstCooldown = 0;
                std::cout << "PRZELADOWANO! Gotowy do strzalu." << std::endl;
            }
        }
        else if (input.shoot && weapon.shotsFired < weapon.maxShots && weapon.timeSinceLastShot >= weapon.fireRate) {
            spawnBullet(e);
            weapon.shotsFired++;
            weapon.timeSinceLastShot = 0.f;

            // Opcjonalnie: Logowanie
            // int bulletsLeft = weapon.maxShots - weapon.shotsFired;
            // std::cout << "Strzal! Zostalo kul: " << bulletsLeft << std::endl;

            if (weapon.shotsFired >= weapon.maxShots) {
                weapon.burstCooldown = weapon.burstDelay;
                std::cout << "Pusty magazynek! Przeladowywanie (5s)..." << std::endl;
            }
        }
    }

    // 2. NOWA LOGIKA RUCHU I ODBIJANIA POCISKÓW
    for (auto& e : m_entityManager.getEntitiesByType("Bullet")) {
        auto& transform = e->getComponent<CTransform>("CTransform");
        auto& bullet = e->getComponent<CBullet>("CBullet");

        // A. Odliczanie czasu ¿ycia
        bullet.lifetime -= dt;
        if (bullet.lifetime <= 0) {
            std::cout << "Kula zniknela ze starosci." << std::endl;
            e->destroy();
            continue; // Nie ma sensu liczyæ fizyki dla martwej kuli
        }

        // B. Fizyka Odbiæ (Rykoszet)
        // Musimy sprawdziæ oœ X i oœ Y osobno, ¿eby wiedzieæ, od której œciany siê odbiliœmy.

        // --- Sprawdzanie osi X ---
        float nextX = transform.pos.x + transform.velocity.x * dt;
        int gx = static_cast<int>(nextX / TILE_SIZE);
        int gy = static_cast<int>(transform.pos.y / TILE_SIZE); // Y siê jeszcze nie zmieni³

        // Jeœli nastêpny krok w X wchodzi w œcianê...
        if (isSolid(gx, gy)) {
            // ...to ODBIJAMY SIÊ (odwracamy prêdkoœæ X)
            transform.velocity.x *= -1.0f;
        }
        else {
            // ...w przeciwnym razie przesuwamy siê normalnie
            transform.pos.x = nextX;
        }

        // --- Sprawdzanie osi Y ---
        float nextY = transform.pos.y + transform.velocity.y * dt;
        gx = static_cast<int>(transform.pos.x / TILE_SIZE); // X jest ju¿ zaktualizowany
        gy = static_cast<int>(nextY / TILE_SIZE);

        // Jeœli nastêpny krok w Y wchodzi w œcianê...
        if (isSolid(gx, gy)) {
            // ...to ODBIJAMY SIÊ (odwracamy prêdkoœæ Y)
            transform.velocity.y *= -1.0f;
        }
        else {
            transform.pos.y = nextY;
        }

        // C. Aktualizacja k¹ta obrotu (Grafika)
        // Skoro kula mog³a zmieniæ kierunek lotu, musimy zaktualizowaæ jej k¹t (transform.angle),
        // ¿eby Sprite obróci³ siê w stronê nowego lotu.

        // atan2 zwraca k¹t w radianach na podstawie wektora (y, x)
        float angleRad = std::atan2(transform.velocity.y, transform.velocity.x);
        transform.angle = angleRad * (180.0f / 3.14159265f); // Zamiana na stopnie


        // D. Synchronizacja Sprite'a
        if (e->hasComponent("CSprite")) {
            auto& spriteComp = e->getComponent<CSprite>("CSprite");
            if (spriteComp.sprite) {
                spriteComp.sprite->setPosition(transform.pos);
                // +90 stopni korekty, bo Twoja tekstura pewnie patrzy w górê, a matematyka zak³ada w prawo
                spriteComp.sprite->setRotation(sf::degrees(transform.angle + 90.0f));
            }
        }
    }
}