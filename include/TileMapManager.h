#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "TextureManager.h"

class TileMapManager {
private:
    TextureManager& m_texMgr;
    float m_tileSize;
    sf::VertexArray m_vertices;
    std::vector<sf::FloatRect> m_wallBounds;

public:
    TileMapManager(TextureManager& tm, float size);
    void loadMap(const std::vector<std::string>& levelData);
    void draw(sf::RenderWindow& window);

    const std::vector<sf::FloatRect>& getWallBounds() const;
};
