#include <chrono>
#include <iostream>

#include "engine/physics/physics.hpp"
#include "renderer/renderer.hpp"

using namespace std::chrono;

int main() {
    const int window_width = 1000;
    const int window_height = 1000;
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "VerletBalls", sf::Style::Close);

    Vec2 world_size = {150, 150};
    PhysicsSolver solver{world_size};
    Renderer renderer{solver};

    float zoom_level = 6.5f;
    sf::View view(window.getDefaultView());
    view.setCenter({75, 75});
    view.setSize(window.getSize().x / zoom_level, window.getSize().y / zoom_level);
    window.setView(view);

    sf::Clock clock;
    sf::Time elapsed;
    auto last_second = steady_clock::now();

    int fps_cap = 60;
    window.setFramerateLimit(fps_cap);
    const float dt = 1.0f / static_cast<float>(fps_cap);

    while (window.isOpen()) {
        elapsed = clock.restart();

        if (solver.objects.size() < 15000) {
            for (int i = 10; i--;) {
                const auto id = solver.createObject({2.0f, 1.0f + 1.1f * i});
                solver.objects[id].last_position.x -= 0.14f;
                solver.objects[id].color = sf::Color::White;
            }
        }

        auto solver_start = steady_clock::now();
        solver.update(dt);
        auto solver_done = steady_clock::now() - solver_start;

        window.clear();
        auto render_start = steady_clock::now();
        renderer.render(window);

        // Show nodes
        // for (auto& child : solver.qtree.nodes) {
        //    if (child.count <= 0) {
        //        continue;
        //    }
        //    sf::RectangleShape rect(sf::Vector2f(child.area.half_size.x * 2, child.area.half_size.y * 2));
        //    rect.setOrigin(rect.getSize() / 2.f);
        //    rect.setPosition(child.area.position.x, child.area.position.y);
        //    rect.setFillColor(sf::Color::Transparent);
        //    rect.setOutlineThickness(0.2);
        //    rect.setOutlineColor(sf::Color::Green);
        //    window.draw(rect);
        //}

        window.display();
        auto render_done = steady_clock::now() - render_start;

        if (steady_clock::now() - last_second >= 1s) {
            std::cout << "-------------------\n";
            std::cout << "Running on " << solver.sub_steps << " substeps  \n";
            std::cout << "Frames per second: " << static_cast<int>(1.0f / elapsed.asSeconds()) << "\n";
            std::cout << "Physics took: " << duration_cast<milliseconds>(solver_done) << "\n";
            std::cout << "Rendering took: " << duration_cast<milliseconds>(render_done) << "\n";
            std::cout << "-------------------\n";
            last_second = steady_clock::now();
        }
    }
}
