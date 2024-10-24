#pragma once
#include <chrono>
#include <cmath>
#include <format>
#include <future>
#include <vector>

#include "engine/common/quadtree.hpp"
#include "engine/common/vec.hpp"
#include "physics_object.hpp"

struct PhysicsSolver {
    QuadTree qtree;
    std::vector<PhysicsObject> objects;
    Vec2 world_size;
    Vec2 gravity = {0.0f, 20.0f};
    int sub_steps;

    PhysicsSolver(const Vec2& size) : qtree{size}, world_size{size}, sub_steps{8} {}

    int addObject(const PhysicsObject& object) {
        objects.emplace_back(object);
        return objects.size() - 1;
    }

    int createObject(const Vec2& pos) {
        objects.emplace_back(PhysicsObject{pos});
        return objects.size() - 1;
    }

    void addObjectsToTree() {
        qtree.clear();
        for (int i = qtree.objects.size(); i < objects.size(); i++) {
            const PhysicsObject& obj = objects[i];
            if (obj.position.x > 0.0f && obj.position.x < world_size.x && obj.position.y > 0.0f && obj.position.y < world_size.y) {
                qtree.insert(obj.position, i);
            }
        }
    }

    void solveContact(int obj_1_idx, int obj_2_idx) {
        PhysicsObject& obj_1 = objects[obj_1_idx];
        PhysicsObject& obj_2 = objects[obj_2_idx];
        const Vec2 diff = obj_1.position - obj_2.position;
        const float dist_2 = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        // Radius are all equal to 1.0f
        const float delta = 0.5f * (1.0f - dist_2);
        const Vec2& col_vec = (diff / dist_2) * delta;
        obj_1.position += col_vec;
        obj_2.position -= col_vec;
    }

    std::vector<QuadCell> generateQuadCells(int n, float screenWidth, float screenHeight) {
        std::vector<QuadCell> cells;

        float cellWidth = screenWidth / n;

        for (int i = 0; i < n; ++i) {
            float cellX = i * cellWidth + cellWidth / 2.0f;
            float cellY = screenHeight / 2.0f;

            Vec2 cellPosition(cellX, cellY);
            QuadCell cell(cellPosition, cellWidth, screenHeight);

            cells.push_back(cell);
        }

        return cells;
    }

    std::vector<QuadCell> thread_cells = generateQuadCells(8, 150, 150);

    std::vector<std::future<void>> cell_futures = std::vector<std::future<void>>(thread_cells.size());

    void solveCollisions() {
        const auto solve_objects = [&](const std::vector<int>& indicies) {
            for (const int i : indicies) {
                const std::vector<int>& found_ids = qtree.query(objects[i]);
                for (const int found_id : found_ids) {
                    if (found_id != i) {
                        solveContact(found_id, i);
                    }
                }
            }
        };

        std::vector<std::vector<int>> vec_of_vecint;

        for (const auto& cell : thread_cells) {
            vec_of_vecint.emplace_back(qtree.query(cell));
        }

        for (int i = 0; i < cell_futures.size(); i += 2) {
            cell_futures[i] = std::async(std::launch::async, solve_objects, vec_of_vecint[i]);
        }

        for (int i = 0; i < cell_futures.size(); i += 2) {
            cell_futures[i].wait();
        }

        for (int i = 1; i < cell_futures.size(); i += 2) {
            cell_futures[i] = std::async(std::launch::async, solve_objects, vec_of_vecint[i]);
        }

        for (int i = 1; i < cell_futures.size(); i += 2) {
            cell_futures[i].wait();
        }

        // for (const auto& obj : qtree.objects) {
        //     const QuadObject& elm = obj;
        //     const std::vector<int>& found_ids = qtree.query(objects[elm.id]);
        //     for (const auto& found_id : found_ids) {
        //         if (found_id != elm.id) {
        //             solveContact(found_id, elm.id);
        //         }
        //     }
        // }
    }

    void updateObjects(float dt) {
        const auto update_objects = [&](size_t start, size_t end) {
            for (size_t i = start; i < end; i++) {
                PhysicsObject& obj = objects[i];
                obj.acceleration += gravity;
                obj.update(dt);
                const float margin = 1.0f;
                if (obj.position.x > world_size.x - margin) {
                    obj.position.x = world_size.x - margin;
                } else if (obj.position.x < margin) {
                    obj.position.x = margin;
                }
                if (obj.position.y > world_size.y - margin) {
                    obj.position.y = world_size.y - margin;
                } else if (obj.position.y < margin) {
                    obj.position.y = margin;
                }
            }
        };

        // int batch_size = objects.size() / futures.size();
        // for (size_t i = 0; i < futures.size(); i++) {
        //     const size_t start = i * batch_size;
        //     const size_t end = start + batch_size;
        //     futures[i] = std::async(std::launch::async, update_objects, start, end);
        // }

        // for (const auto& future : futures) {
        //     future.wait();
        // }

        // if (batch_size * futures.size() < objects.size()) {
        //     const size_t start = batch_size * futures.size();
        update_objects(0, objects.size());
        //}
    }

    void update(float dt) {
        const float sub_dt = dt / static_cast<float>(sub_steps);
        for (int i = sub_steps; i--;) {
            addObjectsToTree();
            solveCollisions();
            updateObjects(sub_dt);
            // for (auto& obj : qtree.objects) {
            //     obj.element.position = objects[obj.element.id].position;
            // }
            // qtree.updateLeafs();
        }
        // qtree.cleanup();
    }
};
