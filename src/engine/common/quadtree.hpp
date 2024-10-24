#pragma once
#include <vector>

#include "engine/common/vec.hpp"

struct QuadObject {
    Vec2 position;  // coordinates of the object
    int next;       // next object in the node, -1 if last
    int id;         // can be used for external data

    QuadObject() = default;
    QuadObject(const Vec2& pos, int _id) : position{pos}, next{-1}, id{_id} {}
};

struct QuadCell {
    Vec2 position;   // center of the rectangle
    Vec2 half_size;  // distance from bounds to center for each axis

    QuadCell(const Vec2& pos, float width, float height) : position{pos}, half_size{width / 2, height / 2} {}

    bool contains(const Vec2& point) const {
        float left = position.x - half_size.x;
        float right = position.x + half_size.x;
        float top = position.y - half_size.y;
        float bottom = position.y + half_size.y;
        return left < point.x && right >= point.x && top < point.y && bottom >= point.y;
    }
    bool contains(const QuadObject& obj) const {
        auto point = obj.position;
        float left = position.x - half_size.x;
        float right = position.x + half_size.x;
        float top = position.y - half_size.y;
        float bottom = position.y + half_size.y;
        return left < point.x && right >= point.x && top < point.y && bottom >= point.y;
    }
    bool intersects(QuadCell& a) const {
        return true;
    }
};

struct QuadNode {
  public:
    QuadCell area;    // QuadNode's AABB
    int first_child;  // if node is branch, index to the first child node, else if node is leaf, index to the first object in the node, -1 if empty
    int count;        // number of objects in the cell, -1 if node is branch
    int depth;        // depth of QuadNode in the quadtree

    QuadNode(const QuadCell& _area, int _depth) : area{_area}, first_child{-1}, count{0}, depth{_depth} {}
};

struct QuadTree {
    std::vector<QuadObject> objects;  // stores all object of the quadtree
    std::vector<QuadNode> nodes;      // childs of the quadtree (root is always at index 0)
    QuadCell root_bounds;             // root's AABB
    const int max_obj = 8;            // max number of objects allowed in each cell before subdividing
    const int max_depth = 4;          // how deep the tree can grow
    int free_node = -1;

    QuadTree(const QuadCell& bounds) : root_bounds{bounds} {
        nodes.reserve(1 + max_depth * max_depth * max_depth * max_depth);
        nodes.emplace_back(QuadNode{root_bounds, 0});  // root node
    }

    QuadTree(const Vec2& size) : root_bounds{size / 2.0f, size.x, size.y} {
        nodes.reserve(1 + max_depth * max_depth * max_depth * max_depth);
        nodes.emplace_back(QuadNode{root_bounds, 0});  // root node
    }

    int getQuadrant(const QuadCell& area, const QuadObject& obj) const {
        const float cx = area.position.x;
        const float cy = area.position.y;

        if (obj.position.x < cx) {
            if (obj.position.y < cy) {
                return 0;  // Upper left
            } else {
                return 2;  // Lower left
            }
        } else {
            if (obj.position.y < cy) {
                return 1;  // Upper right
            } else {
                return 3;  // Lower right
            }
        }
    }

    void splitNode(QuadNode& node) {
        const QuadCell& area = node.area;
        const Vec2& current_pos = area.position;
        const Vec2& new_half_size = area.half_size / 2.0f;
        int childs_index = -1;

        QuadNode new_nodes[4] = {// Upper left
                                 QuadNode{QuadCell{{current_pos.x - new_half_size.x, current_pos.y - new_half_size.y}, area.half_size.x, area.half_size.y}, node.depth + 1},
                                 // Lower left
                                 QuadNode{QuadCell{{current_pos.x + new_half_size.x, current_pos.y - new_half_size.y}, area.half_size.x, area.half_size.y}, node.depth + 1},
                                 // Upper right
                                 QuadNode{QuadCell{{current_pos.x - new_half_size.x, current_pos.y + new_half_size.y}, area.half_size.x, area.half_size.y}, node.depth + 1},
                                 // Lower right
                                 QuadNode{QuadCell{{current_pos.x + new_half_size.x, current_pos.y + new_half_size.y}, area.half_size.x, area.half_size.y}, node.depth + 1}};

        if (free_node != -1) {
            childs_index = free_node;
            free_node = nodes[free_node].first_child;
            nodes[childs_index] = new_nodes[0];
            nodes[childs_index + 1] = new_nodes[1];
            nodes[childs_index + 2] = new_nodes[2];
            nodes[childs_index + 3] = new_nodes[3];
        } else {
            childs_index = nodes.size();
            nodes.emplace_back(new_nodes[0]);
            nodes.emplace_back(new_nodes[1]);
            nodes.emplace_back(new_nodes[2]);
            nodes.emplace_back(new_nodes[3]);
        }

        int object_index = node.first_child;
        while (object_index != -1) {
            QuadObject& obj = objects[object_index];
            int next = obj.next;
            int quadrant = getQuadrant(area, obj);
            QuadNode& child = nodes[childs_index + quadrant];
            if (child.count > 0) {
                obj.next = child.first_child;
                child.first_child = object_index;
            } else {
                child.first_child = object_index;
                obj.next = -1;
            }
            object_index = next;
            child.count++;
        }

        node.first_child = childs_index;
        node.count = -1;
    }

    void insert(const Vec2& position, const int id) {
        int node_index = 0;
        QuadObject object{position, id};
        int object_index = objects.size();
        objects.push_back(object);
        while (nodes[node_index].count == -1) {
            int quadrant = getQuadrant(nodes[node_index].area, object);
            node_index = nodes[node_index].first_child + quadrant;
        }
        if (nodes[node_index].count >= max_obj && nodes[node_index].depth < max_depth) {
            splitNode(nodes[node_index]);
            int quadrant = getQuadrant(nodes[node_index].area, object);
            node_index = nodes[node_index].first_child + quadrant;
        }
        QuadNode& node = nodes[node_index];
        if (node.count > 0) {
            objects[object_index].next = node.first_child;
            node.first_child = object_index;
        } else {
            node.first_child = object_index;
            objects[object_index].next = -1;
        }
        node.count++;
    }

    void updateLeafs() {
        std::vector<int> to_reinsert;

        for (auto& node : nodes) {
            if (node.count <= 0) {
                continue;
            }
            int current_index = node.first_child;
            int prev_index = -1;
            while (current_index != -1) {
                int next_index = objects[current_index].next;
                if (node.area.contains(objects[current_index].position)) {
                    prev_index = current_index;
                } else {
                    if (node.first_child == current_index) {
                        node.first_child = next_index;
                    } else {
                        objects[prev_index].next = next_index;
                    }
                    to_reinsert.emplace_back(current_index);
                    node.count--;
                }
                current_index = next_index;
            }
        }

        for (const auto i : to_reinsert) {
            const Vec2 position = objects[i].position;
            const int id = objects[i].id;
            // objects.erase(i);
            insert(position, id);
        }
    }

    void cleanup() {
        std::vector<int> to_process;

        if (nodes[0].count == -1) {
            to_process.emplace_back(0);
        }

        while (to_process.size() > 0) {
            const int node_index = to_process.back();
            QuadNode& node = nodes[node_index];
            to_process.pop_back();

            int empty_leaves = 0;

            for (int i = 0; i < 4; i++) {
                const int child_index = node.first_child + i;
                const QuadNode& child = nodes[child_index];

                if (child.count == 0) {
                    empty_leaves++;
                } else if (child.count == -1) {
                    to_process.emplace_back(child_index);
                }
            }

            if (empty_leaves == 4) {
                nodes[node.first_child].first_child = free_node;
                free_node = node.first_child;
                node.first_child = -1;
                node.count = 0;
            }
        }
    }

    void clear() {
        objects.clear();
        nodes.clear();
        nodes.emplace_back(QuadNode{root_bounds, 0});
    }

    template <typename Bound>
    std::vector<int> query(const Bound& bounds) {
        std::vector<int> result;
        query(bounds, result, 0);
        return result;
    }

  private:
    template <typename Bound>
    void query(const Bound& bounds, std::vector<int>& result, int idx) {
        if (!bounds.intersects(nodes[idx].area)) {
            return;
        }

        if (nodes[idx].count == -1) {
            for (int i = 0; i < 4; i++) {
                const int index = nodes[idx].first_child + i;
                query(bounds, result, index);
            }
        } else if (nodes[idx].count > 0) {
            int next = nodes[idx].first_child;
            while (next != -1) {
                const QuadObject& obj = objects[next];
                if (bounds.contains(obj)) {
                    result.emplace_back(next);
                }
                next = obj.next;
            }
        }
    }
};
