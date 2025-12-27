#pragma once
#include <string>
#include "TextureManager.h"
//#include "FontManager.h"
#include "TileMapManager.h"
//#include "ConfigManager.h"
// i inne

class AssetManager
{
public:
    TextureManager textures;
    TileMapManager tileMaps;

    AssetManager() :tileMaps(textures, 32.0f) {}
    //ConfigManager config;
    //FontManager fonts;
    // i inne

    void loadAll()
    {   
        //config.loadFromFile(SCENE_CONFIG_FILE);
        textures.loadTexture("wall", "assets/graphics/wall.png");
        textures.loadTexture("tank", "assets/graphics/tank.png");
        textures.loadTexture("tileset", "assets/graphics/tileset.png");
        //fonts.loadFromFile("assets/sounds.txt");
        // i inne
    }
};
