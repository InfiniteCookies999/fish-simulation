
#include "simulation.h"

#include <random>
#include <raylib.h>

typedef std::mt19937 rng_t;

static const int WIDTH = 800;
static const int HEIGHT = 600;

static void create_fishes(Simulation& sim, const SwimPath& swim_path, int num_fish);

static void update(Simulation& sim);

static void render(const Simulation& sim);
static void render_path(const SwimPath& swim_path);

static float degrees(float radians);

int main() {

    InitWindow(WIDTH, HEIGHT, "fish simulation");

    // TODO (maddie): can we vsync this? if we are doing this in webgl eventually does it make any
    //                sense to vsync... what even is vsync in a browser.
    SetTargetFPS(60);

    Simulation sim(WIDTH, HEIGHT);

    Image fish_image = LoadImage("fish.png");
    sim.fish_texture = LoadTextureFromImage(fish_image);
    UnloadImage(fish_image);

    const int NUM_PATHS = 4;
    const int path_start_indexes[NUM_PATHS] = { 0, 6, 12, 14 };
    const int num_fishes[NUM_PATHS] = { 64, 40, 64, 32 };

    for (int i = 0; i < NUM_PATHS; i++) {
        sim.swim_paths.push_back(SwimPath{});
    }

    for (int i = 0; i < NUM_PATHS; i++) {
        SwimPath& swim_path = sim.swim_paths[i];
        generate_swim_path(sim, swim_path, path_start_indexes[i]);
    }

    for (int i = 0; i < NUM_PATHS; i++) {
        SwimPath& swim_path = sim.swim_paths[i];
        create_fishes(sim, swim_path, num_fishes[i]);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_F1)) {
            for (int i = 0; i < NUM_PATHS; i++) {
                SwimPath& swim_path = sim.swim_paths[i];
                generate_swim_path(sim, swim_path, path_start_indexes[i]);
            }
        }

        // TODO (maddie): we should measure performance
        update(sim);
        render(sim);

        EndDrawing();
    }

    UnloadTexture(sim.fish_texture);
    CloseWindow();

    return 0;
}

static void create_fishes(Simulation& sim, const SwimPath& swim_path, int num_fish) {
    std::uniform_real_distribution<float> pos_dist(-20.f, +20.f);
    std::uniform_real_distribution<float> scale_dist(0.25f, 0.8f);
    std::uniform_real_distribution<float> max_speed_dist(1.5f, 3.f);
    for (int i = 0; i < num_fish; i++) {
        float x = swim_path.cur_point.x + pos_dist(sim.gen);
        float y = swim_path.cur_point.y + pos_dist(sim.gen);
        float scale = scale_dist(sim.gen);
        Fish fish = Fish(swim_path);
        fish.position = Vector2{ x, y };
        fish.scale = scale;
        fish.max_speed = max_speed_dist(sim.gen);
        fish.special = i == 0;
        sim.fishes.push_back(fish);
    }
}

static void update(Simulation& sim) {

    float dt = GetFrameTime();

    for (SwimPath& path : sim.swim_paths) {
        update_swim_point(path, dt);
    }

    update_fishes(sim, dt);

}

static void render(const Simulation& sim) {

    ClearBackground(Color{ 47, 130, 204 });

    // for (const SwimPath& path : sim.swim_paths) {
    //     render_path(path);
    // }

    const Color fish_color = Color{ 255, 255, 255, 190 };
    for (const Fish& fish : sim.fishes) {
        // if (fish.special) {
        //     DrawCircleV(fish.position, 30.f, Color{ 200, 200, 200, 30 });
        // }

        // the part of the texture to use for drawing.
        Rectangle source_rect = { 0, 0, 32, 32 };

        // the destination rectangle defines where to draw the texture.
        Rectangle dest_rect = { fish.position.x, fish.position.y, 32*fish.scale, 32*fish.scale };
        // we want to specify the origin to be the center of the texture so it rotates around the center.
        Vector2 origin = { 16*fish.scale, 16*fish.scale };
        // std::cout << "angle: " << fish.angle << "\n";
        DrawTexturePro(sim.fish_texture, source_rect, dest_rect, origin, degrees(fish.angle) + 270, fish_color);
    }
}

static void render_path(const SwimPath& swim_path) {
    for (size_t i = 0 ; i < swim_path.points.size() - 1; i++) {
        Vector2 point0 = swim_path.points[i  ];
        Vector2 point1 = swim_path.points[i+1];
        DrawLineBezier(point0, point1, 1.f, RAYWHITE);
    }
    DrawLineBezier(swim_path.points[swim_path.points.size() - 1], swim_path.points[0], 1.f, RAYWHITE);


    for (Vector2 point : swim_path.points) {
        DrawCircleV(point, 4.f, BLACK);
    }

    DrawCircleV(swim_path.cur_point, 8.f, Color{ 248, 3, 252, 255 });
}

static float degrees(float radians) {
    return radians * (180.f / PI);
}
