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

    // 1. T£O (MAPA)
    if (m_masterVertexArray.getVertexCount() > 0) {
        window.draw(m_masterVertexArray, &m_tilesetTexture);
    }

    // --- POPRAWKA: Rysowanie optionala ---
    if (!m_isGenerating) {
        // Musimy sprawdziæ czy tekst istnieje (has_value) i go wy³uskaæ (*)
        if (m_textScoreP1.has_value()) window.draw(*m_textScoreP1);
        if (m_textScoreP2.has_value()) window.draw(*m_textScoreP2);
        if (m_textW.has_value()) window.draw(*m_textW);
    }

    // 3. GRACZE (Nad wynikiem)
    for (auto& e : m_entityManager.getEntitiesByType("Player")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) window.draw(*comp.sprite);
        }
    }

    // 4. KULE
    for (auto& e : m_entityManager.getEntitiesByType("Bullet")) {
        if (e->hasComponent("CSprite")) {
            auto& comp = e->getComponent<CSprite>("CSprite");
            if (comp.sprite) window.draw(*comp.sprite);
        }
    }

    // 5. Rysowanie loadera (na samym wierzchu)
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

    this->m_scoreP1 = 0; m_textScoreP1->setString("0");
    this->m_scoreP2 = 0; m_textScoreP2->setString("0");

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
    m_player1 = p1;
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
    m_player2 = p2;
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

bool SceneTest::spawnBullet(std::shared_ptr<Entity> shooter) {
    auto& shooterTransform = shooter->getComponent<CTransform>("CTransform");

    // 1. Obliczamy pozycjê startow¹ (tak jak wczeœniej)
    float bulletSpeed = 600.0f;
    float radians = shooterTransform.angle * (3.14159265f / 180.0f);

    // Offset (wylot lufy)
    float offsetDist = 40.0f;
    sf::Vector2f offset(std::cos(radians) * offsetDist, std::sin(radians) * offsetDist);
    sf::Vector2f spawnPos = shooterTransform.pos + offset;

    // --- NOWOŒÆ: SPRAWDZENIE CZY LUFA NIE JEST W ŒCIANIE ---
    int gx = static_cast<int>(spawnPos.x / TILE_SIZE);
    int gy = static_cast<int>(spawnPos.y / TILE_SIZE);

    if (isSolid(gx, gy)) {
        std::cout << "Lufa zablokowana przez sciane! Nie mozna strzelic." << std::endl;
        return false; // PRZERYWAMY STRZA£
    }
    // -------------------------------------------------------

    // Jeœli jest wolne, tworzymy kulê normalnie...
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

    return true; // Sukces!
}

void SceneTest::sWeapon(float dt) {
    // 1. Logika strzelania (BEZ ZMIAN)
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

            // --- NOWOŒÆ: Próbujemy strzeliæ ---
            bool shotSuccess = spawnBullet(e);

            if (shotSuccess) {
                // Wykonujemy logikê TYLKO jeœli kula faktycznie powsta³a
                weapon.shotsFired++;
                weapon.timeSinceLastShot = 0.f;

                if (weapon.shotsFired >= weapon.maxShots) {
                    weapon.burstCooldown = weapon.burstDelay;
                    std::cout << "Pusty magazynek! Przeladowywanie..." << std::endl;
                }
            }
            // Jeœli shotSuccess == false, nic nie robimy (gracz musi odjechaæ od œciany)
        }
    }

    // 2. LOGIKA KULI (Rykoszety + Trafienia w czo³g)
    auto& players = m_entityManager.getEntitiesByType("Player"); // Pobieramy listê graczy raz

    for (auto& b : m_entityManager.getEntitiesByType("Bullet")) {
        auto& bTransform = b->getComponent<CTransform>("CTransform");
        auto& bBox = b->getComponent<CBoundingBox>("CBoundingBox");
        auto& bullet = b->getComponent<CBullet>("CBullet");

        // A. Czas ¿ycia
        bullet.lifetime -= dt;
        if (bullet.lifetime <= 0) { b->destroy(); continue; }

        // B. Sprawdzanie kolizji z GRACZAMI (Trafienie = Œmieræ)
        for (auto& p : players) {
            auto& pTransform = p->getComponent<CTransform>("CTransform");
            auto& pBox = p->getComponent<CBoundingBox>("CBoundingBox");

            // Prosta kolizja AABB (Prostok¹t - Prostok¹t)
            // Obliczamy ró¿nicê pozycji (bierzemy wartoœæ bezwzglêdn¹ abs)
            float dx = std::abs(bTransform.pos.x - pTransform.pos.x);
            float dy = std::abs(bTransform.pos.y - pTransform.pos.y);

            // Sprawdzamy czy odleg³oœæ jest mniejsza ni¿ suma po³ówek szerokoœci/wysokoœci
            float w = bBox.halfSize.x + pBox.halfSize.x;
            float h = bBox.halfSize.y + pBox.halfSize.y;

            if (dx < w && dy < h) {
                // KOLIZJA!
                std::cout << "TRAFIENIE! Czolg zniszczony!" << std::endl;

                // --- NOWA LOGIKA PUNKTACJI ---
                // Jeœli zgin¹³ Gracz 1 -> Punkt dla Gracza 2
                if (m_player1 && p->id() == m_player1->id()) {
                    m_scoreP2++;
                    // Zmiana na strza³kê ->
                    m_textScoreP2->setString(std::to_string(m_scoreP2));
                }
                // Jeœli zgin¹³ Gracz 2 -> Punkt dla Gracza 1
                else if (m_player2 && p->id() == m_player2->id()) {
                    m_scoreP1++;
                    // Zmiana na strza³kê ->
                    m_textScoreP1->setString(std::to_string(m_scoreP1));
                }

                // Centrujemy tekst ponownie (SFML 3.0 fix)
                sf::FloatRect b1 = m_textScoreP1->getLocalBounds();
                m_textScoreP1->setOrigin(sf::Vector2f(b1.size.x / 2.f, b1.size.y / 2.f)); // size.x

                sf::FloatRect b2 = m_textScoreP2->getLocalBounds();
                m_textScoreP2->setOrigin(sf::Vector2f(b2.size.x / 2.f, b2.size.y / 2.f)); // size.x
                // -----------------------------

                // Niszczymy kulê
                b->destroy();

                // Respawn czo³gu (ofiary)
                pTransform.pos = pTransform.homePos;
                pTransform.velocity = { 0.f, 0.f };
                pTransform.angle = 0.f;

                // Reset broni
                if (p->hasComponent("CBurstWeapon")) {
                    auto& wpn = p->getComponent<CBurstWeapon>("CBurstWeapon");
                    wpn.shotsFired = 0;
                    wpn.burstCooldown = 0;
                }

                break;
            }
        }

        // Jeœli kula zosta³a zniszczona na czo³gu, nie licz dla niej odbiæ
        if (!b->isAlive()) continue;


        // C. Fizyka Odbiæ (Œciany) - BEZ ZMIAN WZGLÊDEM POPRZEDNIEJ WERSJI
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

        // Aktualizacja k¹ta i Sprite'a
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
    // Sprawdzamy czy AssetManager ma czcionkê
    // (Zak³adam, ¿e Twoja klasa FontManager zwraca referencjê, wiêc to zadzia³a)
    const auto& font = m_engine->assets().fonts.getFont("main");

    // DIAGNOSTYKA: Wypisz info w konsoli (sf::Font::Info dostêpne w SFML)
    // Jeœli czcionka jest pusta, rodzina (family) bêdzie pusta.
    std::cout << "[DEBUG] Konfiguracja tekstu..." << std::endl;

    // Tworzymy teksty
    m_textScoreP1.emplace(font);
    m_textScoreP1->setString("0");
    m_textScoreP1->setCharacterSize(100);
    // ZMIANA: Pe³na widocznoœæ (255), ¿eby wykluczyæ, ¿e jest za blady
    m_textScoreP1->setFillColor(sf::Color(255, 0, 0, 125));

    m_textScoreP2.emplace(font);
    m_textScoreP2->setString("0");
    m_textScoreP2->setCharacterSize(100);
    // ZMIANA: Pe³na widocznoœæ (255)
    m_textScoreP2->setFillColor(sf::Color(65, 105, 225, 125));

    float centerX = (GRID_WIDTH * TILE_SIZE) / 2.0f;
    float centerY = (GRID_HEIGHT * TILE_SIZE) / 2.0f;

    // Pozycjonowanie
    sf::FloatRect bounds1 = m_textScoreP1->getLocalBounds();
    m_textScoreP1->setOrigin(sf::Vector2f(bounds1.size.x / 2.f, bounds1.size.y / 2.f));
    m_textScoreP1->setPosition(sf::Vector2f(centerX - 60.f, centerY - 35.f));

    sf::FloatRect bounds2 = m_textScoreP2->getLocalBounds();
    m_textScoreP2->setOrigin(sf::Vector2f(bounds2.size.x / 2.f, bounds2.size.y / 2.f));
    m_textScoreP2->setPosition(sf::Vector2f(centerX + 60.f, centerY - 35.f));

    // Tworzymy teksty
    m_textW.emplace(font);
    m_textW->setString("graj na wieczko.pl");
    m_textW->setCharacterSize(50);
    // ZMIANA: Pe³na widocznoœæ (255), ¿eby wykluczyæ, ¿e jest za blady
    m_textW->setFillColor(sf::Color(0, 0, 0, 125));
    
    // Pozycjonowanie
    sf::FloatRect bounds3 = m_textW->getLocalBounds();
    m_textW->setOrigin(sf::Vector2f(bounds3.size.x / 2.f, bounds3.size.y / 2.f));
    m_textW->setPosition(sf::Vector2f(centerX, centerY - 115.f));

    std::cout << "[DEBUG] Tekst skonfigurowany. Pozycja srodka: " << centerX << ", " << centerY << std::endl;
}