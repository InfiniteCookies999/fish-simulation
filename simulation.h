#pragma once

#include <random>
#include <unordered_map>
#include <vector>
#include <raylib.h>

struct SwimPath {
    std::vector<Vector2> points;
    int start_index;
    float t;
    Vector2 cur_point;
};

struct Fish {
    int swim_path_index = 0;
    Vector2 position = { 0 };
    Vector2 velocity = { 0 };
    float angle = 0;
    float scale = 1;
    float max_speed = 1;
    int chasing_food_id = -1;
};

struct FishFood {
    Vector2 position = { 0 };
    bool eaten = false;
    int total_fish_chasing = 0;
    long long time_eaten_start = 0;
};

struct TankTap {
    Vector2 position = { 0 };
    long long start_time = 0;
};

struct Simulation {

    const int gui_width;
    const int res_width;
    const int res_height;

    // random
    std::random_device rd;
    std::mt19937 gen;

    // state
    int start_time;

    bool resetting = false;
    bool feeding_fish = false;
    bool tapping_tank = false;
    bool action_in_bounds = false;

    std::vector<SwimPath> swim_paths;
    std::vector<Fish> fishes;

    int fish_food_id_counter = 0;
    std::unordered_map<int, FishFood> fish_foods;
    std::vector<TankTap> tank_taps;

    // time in milliseconds
    long long last_path_change_time;

    // rendering
    Texture2D fish_texture;
    Texture2D fish_food_texture;
    Texture2D fish_food_muched1_texture;
    Texture2D fish_food_muched2_texture;
    Texture2D fish_food_box_texture;

    Simulation(int _gui_width, int _res_width, int _res_height)
        : gui_width(_gui_width)
        , res_width(_res_width)
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
 *
 * If there is food nearby the fish will swim to the food to eat it instead.
 * If the tank has been tapped the fish will avoid the tapped area.
 */
void update_fish_world(Simulation& sim, float dt);

long long current_time_nanoseconds();
long long current_time_milliseconds();


// ------------------------
// math

float degrees(float radians);

Vector2 lerp_cubic_bezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t);
