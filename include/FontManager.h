#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <iostream>

class FontManager {
private:
    std::map<std::string, sf::Font> m_fonts;

public:
    void loadFont(const std::string& name, const std::string& path) {
        sf::Font font;
        if (!font.openFromFile(path)) {
            std::cerr << "Blad: Nie mozna zaladowac czcionki: " << path << std::endl;
            return;
        }
        m_fonts[name] = std::move(font);
        std::cout << "Zaladowano czcionke: " << name << " z " << path << std::endl;
    }

    const sf::Font& getFont(const std::string& name) const {
        auto it = m_fonts.find(name);
        if (it != m_fonts.end()) {
            return it->second;
        }
        std::cerr << "Blad: Brak czcionki o nazwie: " << name << std::endl;
        static sf::Font placeholder;
        return placeholder;
    }
};
