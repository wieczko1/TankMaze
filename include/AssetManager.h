#pragma once
#include <string>
#include "TextureManager.h"
#include "FontManager.h"
#include "TileMapManager.h"
#include "ConfigManager.h"
// i inne

class AssetManager
{
public:
    TextureManager textures;
    TileMapManager tileMaps;
    ConfigManager config;
    AssetManager() :tileMaps(textures, 32.0f) {}
    FontManager fonts;
    // i inne

    void loadAll()
    {   
        config.loadFromFile("assets/config.txt");
        textures.loadTexture("tank", "assets/graphics/tank.png");
        textures.loadTexture("tileset", "assets/graphics/tileset.png");
        fonts.loadFont("main", "assets/fonts/font.ttf");
        // i inne
    }
};
