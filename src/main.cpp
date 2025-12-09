#include <SFML/Graphics.hpp>
#include "EntityManager.h"

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode({1920u, 1080u}), "CMake SFML Project");
    window.setFramerateLimit(144);

    EntityManager manger;
	manger.createEntity("Player");

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
