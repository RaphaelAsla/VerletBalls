#pragma once
#include <SFML/Graphics.hpp>

#include "engine/common/quadtree.hpp"
#include "engine/common/vec.hpp"

struct PhysicsObject {
    Vec2 position = {0.0f, 0.0f};
    Vec2 last_position = {0.0f, 0.0f};
    Vec2 acceleration = {0.0f, 0.0f};
    sf::Color color;

    PhysicsObject() = default;

    PhysicsObject(Vec2 pos) : position{pos}, last_position{pos} {}

    void setPosition(Vec2 pos) {
        position = pos;
        last_position = pos;
    }

    void update(float dt) {
        const Vec2 new_position = 2.0f * position - last_position + acceleration * (dt * dt);
        last_position = position;
        position = new_position;
        acceleration = {0.0f, 0.0f};
    }

    bool contains(const QuadObject& point) const {
        float dx = point.position.x - position.x;
        float dy = point.position.y - position.y;
        float dist_sq = dx * dx + dy * dy;
        // Radius is always equal to 1
        return dist_sq < 1.0f;
    }

    bool intersects(const QuadCell& rect) const {
        float left = rect.position.x - rect.half_size.x;
        float right = rect.position.x + rect.half_size.x;
        float top = rect.position.y - rect.half_size.y;
        float bottom = rect.position.y + rect.half_size.y;

        float nearest_x = std::max(left, std::min(position.x, right));
        float nearest_y = std::max(top, std::min(position.y, bottom));

        float dx = nearest_x - position.x;
        float dy = nearest_y - position.y;
        float dist_sq = dx * dx + dy * dy;

        // Radius is always equal to 1
        return dist_sq <= 1.0f;
    }
};
