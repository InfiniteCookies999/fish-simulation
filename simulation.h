#pragma once

#include <random>
#include <vector>
#include <raylib.h>

struct SwimPath {
    std::vector<Vector2> points;
    int start_index;
    float t;
    Vector2 cur_point;
};

struct Fish {
    const SwimPath& swim_path;
    Vector2 position = { 0 };
    Vector2 velocity = { 0 };
    float angle = 0;
    float scale = 1;
    float max_speed = 1;
    bool special = false;

    Fish(const SwimPath& _swim_path)
        : swim_path(_swim_path) {}
};

struct Simulation {
    const int res_width;
    const int res_height;

    // random
    std::random_device rd;
    std::mt19937 gen;

    // state
    std::vector<SwimPath> swim_paths;
    std::vector<Fish> fishes;

    // rendering
    Texture2D fish_texture;

    Simulation(int _res_width, int _res_height)
        : res_width(_res_width)
        , res_height(_res_height)
        , gen(rd()) {}
};

/*
 * Creates a path that "circles" the window be randomly generating points which can be
 * interpolated over with bezier curves.
 */
void generate_swim_path(Simulation& sim, SwimPath& swim_path, int start_index);

/*
 * Walks the points of the `swim_path` updating `swim_path.cur_point` which walks from
 * point to point using bezier curves.
 */
void update_swim_point(SwimPath& swim_path, float dt);

/*
 * Updates the positions/velocity of the fishes using a combination of following the point
 * defined by the assigned `swim_path` of each fish and boids algorithm.
 */
void update_fishes(Simulation& sim, float dt);
