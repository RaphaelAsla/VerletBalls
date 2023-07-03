#pragma once
#include <SFML/Graphics.hpp>
#include <future>

#include "engine/physics/physics.hpp"

struct Renderer {
    PhysicsSolver& solver;
    sf::VertexArray world_va;
    sf::VertexArray objects_va;
    sf::Texture object_texture;

    Renderer(PhysicsSolver& solver) : solver{solver}, world_va{sf::Quads, 4}, objects_va{sf::Quads} {
        initializeWorldVA();

        object_texture.loadFromFile("res/circle.png");
        object_texture.generateMipmap();
        object_texture.setSmooth(true);
    }

    void render(sf::RenderWindow& window) {
        window.draw(world_va);

        sf::RenderStates states;
        states.texture = &object_texture;
        window.draw(world_va, states);

        updateObjectsVA();
        window.draw(objects_va, states);
    }

    void initializeWorldVA() {
        world_va[0].position = {0.0f, 0.0f};
        world_va[1].position = {solver.world_size.x, 0.0f};
        world_va[2].position = {solver.world_size.x, solver.world_size.y};
        world_va[3].position = {0.0f, solver.world_size.y};

        const char level = 50;
        const sf::Color background_color{level, level, level};
        world_va[0].color = background_color;
        world_va[1].color = background_color;
        world_va[2].color = background_color;
        world_va[3].color = background_color;
    }

    void updateObjectsVA() {
        std::vector<std::future<void>> futures(std::thread::hardware_concurrency());

        objects_va.resize(solver.objects.size() * 4);

        const float texture_size = 1024.0f;
        const float radius = 0.5f;
        const auto make_pixels = [&](size_t start, size_t end) {
            for (int i = start; i < end; ++i) {
                const PhysicsObject& object = solver.objects[i];
                const int idx = i << 2;
                objects_va[idx + 0].position = object.position + Vec2{-radius, -radius};
                objects_va[idx + 1].position = object.position + Vec2{radius, -radius};
                objects_va[idx + 2].position = object.position + Vec2{radius, radius};
                objects_va[idx + 3].position = object.position + Vec2{-radius, radius};
                objects_va[idx + 0].texCoords = {0.0f, 0.0f};
                objects_va[idx + 1].texCoords = {texture_size, 0.0f};
                objects_va[idx + 2].texCoords = {texture_size, texture_size};
                objects_va[idx + 3].texCoords = {0.0f, texture_size};

                const sf::Color color = object.color;
                objects_va[idx + 0].color = color;
                objects_va[idx + 1].color = color;
                objects_va[idx + 2].color = color;
                objects_va[idx + 3].color = color;
            }
        };

        int batch_size = solver.objects.size() / futures.size();
        for (size_t i = 0; i < futures.size(); i++) {
            const size_t start = i * batch_size;
            const size_t end = start + batch_size;
            futures[i] = std::async(std::launch::async, make_pixels, start, end);
        }

        for (auto& future : futures) {
            future.wait();
        }

        if (batch_size * futures.size() < solver.objects.size()) {
            const size_t start = batch_size * futures.size();
            make_pixels(start, solver.objects.size());
        }
    }
};
