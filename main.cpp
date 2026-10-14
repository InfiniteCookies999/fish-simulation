
#include "simulation.h"

#include <chrono>
#include <random>

#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

static const int RES_WIDTH = 200;
static const int WIDTH = 800 + 150;
static const int HEIGHT = 600;

static const int NUM_PATHS = 4;
const int PATH_START_INDEXES[NUM_PATHS] = { 0, 6, 12, 14 };
const int NUM_FISHES[NUM_PATHS] = { 64, 40, 64, 32 };

static void load_textures(Simulation& sim);

static void create_fishes(Simulation& sim, int swim_path_index, int num_fish);

static void update(Simulation& sim);

static void render(const Simulation& sim);
static void render_path(const SwimPath& swim_path);

static void render_and_update_gui(Simulation& sim);

int main() {

    InitWindow(WIDTH, HEIGHT, "fish simulation");

    // TODO (maddie): can we vsync this? if we are doing this in webgl eventually does it make any
    //                sense to vsync... what even is vsync in a browser.
    SetTargetFPS(60);

    rlImGuiSetup(true);

    // Prevent ImGui from making an .ini file.
    auto& imGuiIO{ ImGui::GetIO() };
    imGuiIO.IniFilename = nullptr;


    Simulation sim(RES_WIDTH, WIDTH, HEIGHT);

    load_textures(sim);

    for (int i = 0; i < NUM_PATHS; i++) {
        sim.swim_paths.push_back(SwimPath{});
    }

    for (int i = 0; i < NUM_PATHS; i++) {
        SwimPath& swim_path = sim.swim_paths[i];
        generate_swim_path(sim, swim_path, PATH_START_INDEXES[i]);
    }

    for (int i = 0; i < NUM_PATHS; i++) {
        create_fishes(sim, i, NUM_FISHES[i]);
    }

    sim.last_path_change_time = current_time_milliseconds();
    sim.start_time = current_time_milliseconds();
    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsKeyPressed(KEY_F1)) {
            for (int i = 0; i < NUM_PATHS; i++) {
                SwimPath& swim_path = sim.swim_paths[i];
                generate_swim_path(sim, swim_path, PATH_START_INDEXES[i]);
            }
        }

        update(sim);
        render(sim);
        render_and_update_gui(sim);

        EndDrawing();
    }

    UnloadTexture(sim.fish_texture);
    UnloadTexture(sim.fish_food_texture);
    UnloadTexture(sim.fish_food_muched1_texture);
    UnloadTexture(sim.fish_food_muched2_texture);
    UnloadTexture(sim.fish_food_box_texture);
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}

static void load_textures(Simulation& sim) {
    Image fish_image = LoadImage("assets/fish.png");
    Image fish_food_image = LoadImage("assets/fish_food.png");
    Image fish_food_muched1_image = LoadImage("assets/fish_food_muched1.png");
    Image fish_food_muched2_image = LoadImage("assets/fish_food_muched2.png");
    Image fish_food_box_image = LoadImage("assets/fish_food_box.png");
    sim.fish_texture = LoadTextureFromImage(fish_image);
    sim.fish_food_texture = LoadTextureFromImage(fish_food_image);
    sim.fish_food_muched1_texture = LoadTextureFromImage(fish_food_muched1_image);
    sim.fish_food_muched2_texture = LoadTextureFromImage(fish_food_muched2_image);
    sim.fish_food_box_texture = LoadTextureFromImage(fish_food_box_image);
    UnloadImage(fish_image);
    UnloadImage(fish_food_image);
    UnloadImage(fish_food_muched1_image);
    UnloadImage(fish_food_muched2_image);
    UnloadImage(fish_food_box_image);
}

static void create_fishes(Simulation& sim, int swim_path_index, int num_fish) {
    std::uniform_real_distribution<float> pos_dist(-150.f, +150.f);
    std::uniform_real_distribution<float> scale_dist(0.25f, 0.8f);
    std::uniform_real_distribution<float> max_speed_dist(1.5f, 3.f);
    for (int i = 0; i < num_fish; i++) {
        const SwimPath& swim_path = sim.swim_paths[swim_path_index];
        float x = sim.res_width/2.f + pos_dist(sim.gen);
        float y = sim.res_height/2.f + pos_dist(sim.gen);
        float scale = scale_dist(sim.gen);
        Fish fish{};
        fish.swim_path_index = swim_path_index;
        fish.position = Vector2{ x, y };
        fish.scale = scale;
        fish.max_speed = max_speed_dist(sim.gen);
        sim.fishes.push_back(fish);
    }
}

static void update(Simulation& sim) {

    if (sim.resetting) {
        for (int i = 0; i < NUM_PATHS; i++) {
            SwimPath& swim_path = sim.swim_paths[i];
            generate_swim_path(sim, swim_path, PATH_START_INDEXES[i]);
        }
        sim.fishes.clear();
        for (int i = 0; i < NUM_PATHS; i++) {
            create_fishes(sim, i, NUM_FISHES[i]);
        }
        sim.fish_foods.clear();
        sim.fish_food_id_counter = 0;
        sim.last_path_change_time = current_time_milliseconds();
        sim.start_time = current_time_milliseconds();
        sim.resetting = false;
    }

    float dt = GetFrameTime();

    for (SwimPath& path : sim.swim_paths) {
        update_swim_point(path, dt);
    }

    update_fish_world(sim, dt);


    const int BOUNDS_PAD = 20;
    Vector2 mouse_pos = GetMousePosition();
    sim.action_in_bounds = true;
    if (mouse_pos.x < sim.gui_width + BOUNDS_PAD) {
        sim.action_in_bounds = false;
    }
    if (mouse_pos.x > sim.res_width - BOUNDS_PAD) {
        sim.action_in_bounds = false;
    }
    if (mouse_pos.y < BOUNDS_PAD) {
        sim.action_in_bounds = false;
    }
    if (mouse_pos.y > sim.res_height - 20) {
        sim.action_in_bounds = false;
    }

    if (sim.feeding_fish) {
        if (sim.action_in_bounds && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            std::uniform_real_distribution<float> pos_dist(-5.f, +5.f);
            FishFood fish_food{
                .position = {
                    mouse_pos.x + pos_dist(sim.gen),
                    mouse_pos.y + pos_dist(sim.gen)
                }
            };
            sim.fish_foods.insert({ sim.fish_food_id_counter, fish_food });
            ++sim.fish_food_id_counter;
        }
    }
    if (sim.tapping_tank) {
        if (sim.action_in_bounds && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            sim.tank_taps.push_back(TankTap{
                .position = mouse_pos,
                .start_time = current_time_milliseconds()
            });
        }
    }
}

static void render(const Simulation& sim) {

    ClearBackground(Color{ 47, 130, 204 });

    // for (const SwimPath& swim_path : sim.swim_paths) {
    //     render_path(swim_path);
    // }

    const Color fish_color = Color{ 255, 255, 255, 190 };
    for (const Fish& fish : sim.fishes) {

        // the part of the texture to use for drawing.
        Rectangle source_rect = { 0, 0, 32, 32 };

        // the destination rectangle defines where to draw the texture.
        Rectangle dest_rect = { fish.position.x, fish.position.y, 32*fish.scale, 32*fish.scale };
        // we want to specify the origin to be the center of the texture so it rotates around the center.
        Vector2 origin = { 16*fish.scale, 16*fish.scale };
        DrawTexturePro(sim.fish_texture, source_rect, dest_rect, origin, degrees(fish.angle) + 270, fish_color);

        // DrawCircleV(fish.head_position, 2.f, RED);
    }

    long long time_now = current_time_milliseconds();

    for (const TankTap& tap : sim.tank_taps) {
        long long passed = time_now - tap.start_time;
        float radius = (passed / 15.f);
        int alpha_t = passed/200;
        DrawCircleLinesV(tap.position, radius, Color{
            255, 255, 255,
            static_cast<unsigned char>(alpha_t == 0 ? 200 : 200/alpha_t)
        });
    }

    for (const auto& [_, food] : sim.fish_foods) {
        Texture texture = sim.fish_food_texture;
        if (food.eaten) {
            long long passed = time_now - food.time_eaten_start;
            if (passed < 150) {
                texture = sim.fish_food_muched1_texture;
            } else {
                texture = sim.fish_food_muched2_texture;
            }
        }

        DrawTextureV(texture, food.position, RAYWHITE);
    }

    if (sim.feeding_fish && sim.action_in_bounds) {
        Vector2 mouse_pos = GetMousePosition();
        DrawTexture(sim.fish_food_box_texture, mouse_pos.x - 10, mouse_pos.y - 14, RAYWHITE);
    }
}

static void render_and_update_gui(Simulation& sim) {
    rlImGuiBegin();

    static bool window_open = false;
    ImGui::Begin("My First Tool", &window_open, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowPos(ImVec2{ 0, 0 });
    ImGui::SetWindowSize(ImVec2{
        static_cast<float>(sim.gui_width),
        static_cast<float>(sim.res_height)
    });
    ImGui::SetWindowFontScale(2.f);

    if (ImGui::Button("Reset", ImVec2{ 100, 30 })) {
        sim.resetting = true;
    }
    if (ImGui::Checkbox("Fish Food", &sim.feeding_fish)) {
        sim.tapping_tank = false;
    }
    if (ImGui::Checkbox("Tap tank", &sim.tapping_tank)) {
        sim.feeding_fish = false;
    }

    ImGui::End();

    rlImGuiEnd();
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

long long current_time_nanoseconds() {
    using namespace std::chrono;
    auto timepoint = steady_clock::now();
    return duration_cast<nanoseconds>(timepoint.time_since_epoch()).count();
}

long long current_time_milliseconds() {
    return (long long) (current_time_nanoseconds() * 1e-6);
}
