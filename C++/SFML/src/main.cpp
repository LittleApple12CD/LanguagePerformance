#include "Game.h"
#include <SFML/Graphics.hpp>
#include <optional>

int main() {
    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "Perf Compare - cpp-sfml3",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(0);
    window.setVerticalSyncEnabled(false);

    sf::Image icon;
    if (icon.loadFromFile("data/icon.png")) {
        window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    Game game;
    game.init();

    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt <= 0) dt = 0.001f;

        game.handleInput(window);
        game.update(dt);
        game.render(window);
    }

    return 0;
}