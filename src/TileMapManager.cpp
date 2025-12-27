#include "TileMapManager.h"

TileMapManager::TileMapManager(TextureManager& tm, float size)
    : m_texMgr(tm), m_tileSize(size)
{
    m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
}

void TileMapManager::loadMap(const std::vector<std::string>& levelData) {
    m_vertices.clear();
    m_wallBounds.clear();

    if (levelData.empty()) return;

    sf::Texture& tex = m_texMgr.getTexture("wall");
    sf::Vector2u texSize = tex.getSize();

    // W SFML 3.0 wspó³rzêdne tekstury mog¹ byæ znormalizowane lub w pikselach.
    // Domyœlnie s¹ to piksele.
    float tw = static_cast<float>(texSize.x);
    float th = static_cast<float>(texSize.y);

    for (size_t y = 0; y < levelData.size(); ++y) {
        for (size_t x = 0; x < levelData[y].size(); ++x) {
            if (levelData[y][x] == '#') {
                float xPos = static_cast<float>(x) * m_tileSize;
                float yPos = static_cast<float>(y) * m_tileSize;

                m_wallBounds.push_back(sf::FloatRect({ xPos, yPos }, { m_tileSize, m_tileSize }));

                // Tworzymy pomocniczy wierzcho³ek
                sf::Vertex v;
                v.color = sf::Color::White; // Dobra praktyka w SFML 3.0

                // TRÓJK¥T 1
                v.position = { xPos, yPos }; v.texCoords = { 0.f, 0.f }; m_vertices.append(v);
                v.position = { xPos + m_tileSize, yPos }; v.texCoords = { tw, 0.f }; m_vertices.append(v);
                v.position = { xPos, yPos + m_tileSize }; v.texCoords = { 0.f, th }; m_vertices.append(v);

                // TRÓJK¥T 2
                v.position = { xPos + m_tileSize, yPos }; v.texCoords = { tw, 0.f }; m_vertices.append(v);
                v.position = { xPos + m_tileSize, yPos + m_tileSize }; v.texCoords = { tw, th }; m_vertices.append(v);
                v.position = { xPos, yPos + m_tileSize }; v.texCoords = { 0.f, th }; m_vertices.append(v);
            }
        }
    }
}

void TileMapManager::draw(sf::RenderWindow& window) {
    // Przy rysowaniu VertexArray MUSISZ podaæ teksturê
    window.draw(m_vertices, &m_texMgr.getTexture("wall"));
}

const std::vector<sf::FloatRect>& TileMapManager::getWallBounds() const {
    return m_wallBounds;
}