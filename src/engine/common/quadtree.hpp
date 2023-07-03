#pragma once
#include <iostream>
#include <vector>

#include "engine/common/freelist.hpp"
#include "engine/common/vec.hpp"

struct QuadCell {
    Vec2 position;   // center of the rectangle
    Vec2 half_size;  // distance from bounds to center for each axis

    QuadCell(const Vec2 pos, float width, float height) : position{pos}, half_size{width / 2, height / 2} {}

    bool intersects(const QuadCell& rect) const {
        return (position.x - half_size.x < rect.position.x + rect.half_size.x) && (position.x + half_size.x > rect.position.x - rect.half_size.x) &&
               (position.y - half_size.y < rect.position.y + rect.half_size.y) && (position.y + half_size.y > rect.position.y - rect.half_size.y);
    }

    bool contains(const Vec2& point) const {
        float left = position.x - half_size.x;
        float right = position.x + half_size.x;
        float top = position.y - half_size.y;
        float bottom = position.y + half_size.y;

        float nearest_x = std::max(left, std::min(point.x, right));
        float nearest_y = std::max(top, std::min(point.y, bottom));

        float dx = nearest_x - position.x;
        float dy = nearest_y - position.y;
        float dist_sq = dx * dx + dy * dy;

        // Radius is always equal to 1
        return dist_sq <= 1.0f;
    }
};

struct QuadObject {
    Vec2 position;  // coordinates of the object
    int next;       // next object in the node, -1 if last
    int id;         // can be used for external data

    QuadObject() = default;
    QuadObject(const Vec2 pos, int _id) : position{pos}, id{_id} {}
};

struct QuadNode {
  public:
    QuadCell area;    // QuadNode's AABB
    int first_child;  // if node is branch, index to the first child node, else if node is leaf, index to the first object in the node, -1 if empty
    int count;        // number of objects in the cell, -1 if node is branch
    int depth;        // depth of QuadNode in the quadtree

    QuadNode(const QuadCell& _area, int _depth = 0) : area{_area}, first_child{-1}, count{0}, depth{_depth} {}
};

struct QuadTree {
    FreeList<QuadObject> objects;  // stores all object of the quadtree
    std::vector<QuadNode> nodes;   // childs of the quadtree (root is always at index 0)
    QuadCell root_bounds;          // root's AABB
    const int max_obj = 8;         // max number of objects allowed in each cell before subdividing
    const int max_depth = 4;       // how deep the tree can grow
    int free_node = -1;

    QuadTree(const QuadCell& bounds) : root_bounds{bounds} {
        nodes.reserve(1 + (max_depth * max_depth * max_depth * max_depth));
        nodes.emplace_back(QuadNode{root_bounds});  // root node
    }

    QuadTree(const Vec2& size) : root_bounds{size / 2.0f, size.x, size.y} {
        nodes.reserve(1 + (max_depth * max_depth * max_depth * max_depth));
        nodes.emplace_back(QuadNode{root_bounds});  // root node
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
        const Vec2 current_pos = area.position;
        const Vec2 new_half_size = area.half_size / 2.0f;
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
            if (child.count >= 0) {
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

    void insert(const Vec2 position, const int id) {
        int node_index = 0;
        QuadObject object{position, id};
        int object_index = objects.insert(object);
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
        if (node.count >= 0) {
            objects[object_index].next = node.first_child;
            node.first_child = object_index;
        } else {
            node.first_child = object_index;
        }
        node.count++;
    }

    template <typename Bound>
        requires requires(const Bound& bounds) {
                     { bounds.contains(QuadObject{Vec2{0.0f, 0.0f}, 0}) } -> std::same_as<bool>;
                     { bounds.intersects(QuadCell{Vec2{0.0f, 0.0f}, 0, 0}) } -> std::same_as<bool>;
                 }
    std::vector<int> query(const Bound& bounds) {
        std::vector<int> result;
        query(bounds, result, 0);
        return result;
    }

    void clear() {
        objects.clear();
        nodes.clear();
        nodes.emplace_back(QuadNode{root_bounds});
    }

  private:
    template <typename Bound>
    void query(const Bound& bounds, std::vector<int>& result, int idx = 0) {
        if (!bounds.intersects(nodes[idx].area)) {
            return;
        }

        if (nodes[idx].count == -1) {
            for (int i = 0; i < 4; i++) {
                const int index = nodes[idx].first_child + i;
                query(bounds, result, index);
            }
        } else if (nodes[idx].count >= 0) {
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
