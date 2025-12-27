#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <SFML/Graphics.hpp>
#include <map>
#include <string>

class TextureManager {
private:
    std::map<std::string, sf::Texture> m_textures;

public:
    TextureManager() = default;

    
    bool loadTexture(const std::string& name, const std::string& filename);


    sf::Texture& getTexture(const std::string& name);
};

#endif