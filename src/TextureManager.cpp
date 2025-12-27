#include "TextureManager.h"
#include <iostream>

bool TextureManager::loadTexture(const std::string& name, const std::string& filename) {
    sf::Texture tex;
    if (!tex.loadFromFile(filename)) {
        std::cerr << "Blad: Nie udalo sie wczytac " << filename << std::endl;
        return false;
    }
    m_textures[name] = tex;
    return true;
}

sf::Texture& TextureManager::getTexture(const std::string& name) {
    return m_textures.at(name);
}