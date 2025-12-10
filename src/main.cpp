#include <SFML/Graphics.hpp>
#include "EntityManager.h"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "CMake SFML Project");
    window.setFramerateLimit(144);

    EntityManager manger;
	manger.createEntity("Player");

    // 1. Sprawdü, gdzie program "stoi" w momencie uruchomienia
    std::cout << "Aktualna sciezka robocza: "
        << std::filesystem::current_path() << std::endl;

    // 2. SprÛbuj otworzyÊ plik
    std::string path = "assets/dane.txt";
    std::ifstream file(path);

    if (file.is_open()) {
        std::cout << "SUKCES: Otwarto plik!" << std::endl;
        // Czytanie...
    }
    else {
        std::cerr << "BLAD: Nie udalo sie otworzyc pliku: " << path << std::endl;

        // 3. Sprawdü czy plik w ogÛle istnieje pod tπ úcieøkπ
        if (std::filesystem::exists(path)) {
            std::cout << "Diagnostyka: Plik istnieje, ale nie mozna go otworzyc (moze zajety?)" << std::endl;
        }
        else {
            std::cout << "Diagnostyka: Plik FIZYCZNIE NIE ISTNIEJE w tej lokalizacji." << std::endl;
        }
    }

    // Zatrzymaj konsolÍ øeby przeczytaÊ wynik
    std::cin.get();

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        manger.update(); // manger dziala

        window.clear();
        window.display();
    }
}
