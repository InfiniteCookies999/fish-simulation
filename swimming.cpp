#include "simulation.h"

#include <algorithm>
#include <random>
#include <raymath.h>

static Vector2 get_wrapped_point(const SwimPath& path, int index);

/*
 * Occassionally randomly has the fish decide to change to follow the next
 * path point.
 */
static void fish_change_paths(Simulation& sim);

/*
 * For each fish checks if there is food nearby and sets the id of the food
 * to chase after.
 *
 * If close enough sets the food to eaten and stops chasing.
 */
static void fish_find_food_to_chase_and_eat(Simulation& sim);

static void fish_update_velocity(Simulation& sim, float dt);
static void fish_update_position_and_angle(Simulation& sim);

static void remove_eaten_fish_food(Simulation& sim);
static void remove_tank_taps(Simulation& sim);

void generate_swim_path(Simulation& sim, SwimPath& swim_path, int start_index) {

    swim_path.points.clear();

    // traversing along the arc path by randomized steps so that the points never get
    // too close to each other.
    const int AVG_POINTS = 16;
    const float STEP = (2 * PI) / AVG_POINTS;

    std::uniform_real_distribution<float> step_dist(STEP - STEP/2.f, STEP);
    std::uniform_real_distribution<float> radius_dist(-100.f, +5.f);

    float w = static_cast<float>(sim.res_width);
    float h = static_cast<float>(sim.res_height);

    // produce a circle it looks better.
    float rr = std::min(w - sim.gui_width, h);

    // TODO (maddie): feels as if the last point cuts off a bit early.
    float angle = step_dist(sim.gen);
    while (angle < (2 * PI - STEP/2.f)) {
        angle += step_dist(sim.gen);

        float rx = ((rr - 200 + radius_dist(sim.gen)) / 2);
        float ry = ((rr - 200 + radius_dist(sim.gen)) / 2);

        float ax = sinf(angle) * rx;
        float ay = cosf(angle) * ry;

        float x = (w - sim.gui_width) / 2 + ax + sim.gui_width;
        float y = h / 2 + ay;

        swim_path.points.push_back(Vector2{ x, y });
    }

    start_index %= swim_path.points.size();
    swim_path.cur_point = swim_path.points[start_index];
    swim_path.start_index = start_index;
    swim_path.t = 0;
}

void update_swim_point(SwimPath& swim_path, float dt) {

    // Account for the path length when traversing so that swimming moves at a steady
    // state.
    Vector2 path_start_point = get_wrapped_point(swim_path, swim_path.start_index);
    Vector2 path_end_point = get_wrapped_point(swim_path, swim_path.start_index + 1);
    float path_length = Vector2Distance(path_start_point, path_end_point);

    swim_path.t += dt * (100.f/path_length);
    if (swim_path.t > 1) {
        ++swim_path.start_index;
        swim_path.start_index %= swim_path.points.size();
    }
    while (swim_path.t > 1) {
        swim_path.t -= 1;
    }
    path_start_point = get_wrapped_point(swim_path, swim_path.start_index);
    path_end_point = get_wrapped_point(swim_path, swim_path.start_index + 1);

    Vector2 p0 = path_start_point;
    Vector2 p1 = { 0 };
    Vector2 p2 = { 0 };
    Vector2 p3 = path_end_point;
    // Raylib always uses the midpoint on the x axis so I guess we will follow that.
    float mid_x = (path_start_point.x + path_end_point.x) / 2.f;
    p1.x = mid_x;
    p2.x = mid_x;
    p1.y = path_start_point.y;
    p2.y = path_end_point.y;

    swim_path.cur_point = lerp_cubic_bezier(p0, p1, p2, p3, swim_path.t);
}


void update_fish_world(Simulation& sim, float dt) {

    fish_change_paths(sim);

    fish_find_food_to_chase_and_eat(sim);

    fish_update_velocity(sim, dt);

    fish_update_position_and_angle(sim);

    remove_eaten_fish_food(sim);

    remove_tank_taps(sim);

}

static void fish_change_paths(Simulation& sim) {
    std::uniform_real_distribution<float> path_change_dist(0, 1);

    long long now = current_time_milliseconds();
    if (now - sim.last_path_change_time > 250) {
        sim.last_path_change_time = now;
        for (Fish& fish : sim.fishes) {
            float p = path_change_dist(sim.gen);
            if (!(p < 0.005f)) {
                continue;
            }
            fish.swim_path_index = (fish.swim_path_index + 1) % sim.swim_paths.size();
        }
    }
}

static void fish_find_food_to_chase_and_eat(Simulation& sim) {
    for (Fish& fish : sim.fishes) {
        if (fish.chasing_food_id == -1) {
            // TODO (maddie): may want to randomize this in some way.
            for (auto& [food_id, food] : sim.fish_foods) {
                if (food.eaten) continue;
                if (food.total_fish_chasing > 3) continue;
                float dist = Vector2Distance(fish.position, food.position);
                if (dist < 100.f) {
                    fish.chasing_food_id = food_id;
                    ++food.total_fish_chasing;
                    break;
                }
            }
        }

        if (fish.chasing_food_id != -1) {
            auto it = sim.fish_foods.find(fish.chasing_food_id);
            if (it != sim.fish_foods.end()) {
                FishFood& food = it->second;
                if (food.eaten) {
                    continue;
                }

                float dist = Vector2Distance(fish.position, food.position);
                if (dist < 5.f) {
                    fish.chasing_food_id = -1;
                    food.eaten = true;
                    food.time_eaten_start = current_time_milliseconds();
                }
            } else {
                fish.chasing_food_id = -1;
            }
        }
    }
}

static void fish_update_velocity(Simulation& sim, float dt) {
    // Useful information on boids algorithm:
    // https://vanhunteradams.com/Pico/Animal_Movement/Boids-algorithm.html

    long long time_now = current_time_milliseconds();

    const float avoid_factor = 0.2f;//0.1f; //0.06f;
    const float alignment_factor = 0.02f;//0.015f;
    const float avg_vel_factor = 0.01f;
    const float centering_factor = 0.005f;
    const float turn_factor = 1.5f;

    for (Fish& fish : sim.fishes) {

        float dx = 0, dy = 0;

        if (fish.chasing_food_id != -1) {
            auto it = sim.fish_foods.find(fish.chasing_food_id);
            if (it != sim.fish_foods.end()) {
                // Set the velocity to be fixed since treating moving towards the food as
                // a force means the fish may continuously overshoot the food.
                const FishFood& food = it->second;
                fish.velocity.x = (food.position.x - fish.position.x) * 5.f * dt;
                fish.velocity.y = (food.position.y - fish.position.y) * 5.f * dt;
            }
        }

        const SwimPath& swim_path = sim.swim_paths[fish.swim_path_index];

        if (fish.chasing_food_id == -1) {
            dx += (swim_path.cur_point.x - fish.position.x) * alignment_factor;
            dy += (swim_path.cur_point.y - fish.position.y) * alignment_factor;
        }

        for (const TankTap& tap : sim.tank_taps) {
            float dist = Vector2Distance(fish.position, tap.position);
            if (dist < 100.f) {
                dx += (fish.position.x - tap.position.x) * 0.25f;
                dy += (fish.position.y - tap.position.y) * 0.25f;
            }
        }

        // accumulated velocity away from other fishes so the fishes remain spaced out.
        float avoid_dx = 0;
        float avoid_dy = 0;

        float avg_vel_x = 0;
        float avg_vel_y = 0;
        float avg_pos_x = 0;
        float avg_pos_y = 0;
        int avg_vel_count = 0;
        int avg_pos_count = 0;
        for (const Fish& o_fish : sim.fishes) {
            if (&fish == &o_fish) continue;
            float dist = Vector2Distance(fish.position, o_fish.position);
            if (dist < 20.f) {
                float diff_x = fish.position.x - o_fish.position.x;
                float diff_y = fish.position.y - o_fish.position.y;
                if (dist < 4.f) {
                    avoid_dx += diff_x*16.f;
                    avoid_dy += diff_y*16.f;
                } else {
                    avoid_dx += diff_x;
                    avoid_dy += diff_y;
                }
            }
            if (dist < 50.f) {
                avg_vel_x += fish.velocity.x;
                avg_vel_y += fish.velocity.y;
                ++avg_vel_count;
            }
            if (dist < 5.f) {
                avg_pos_x += fish.velocity.x;
                avg_pos_y += fish.velocity.y;
                ++avg_pos_count;
            }
        }

        if (time_now - sim.start_time > 2500) {
            // apply velocity to ensure the fishes do not get too close
            // to each other.
            dx += avoid_dx*avoid_factor;
            dy += avoid_dy*avoid_factor;
        }

        // apply velocity so that fishes maintain a similar velocity to the
        // other fishes around them.
        if (avg_vel_count != 0 && fish.chasing_food_id == -1) {
            avg_vel_x /= avg_vel_count;
            avg_vel_y /= avg_vel_count;
            dx += (avg_vel_x - fish.velocity.x) * avg_vel_factor;
            dy += (avg_vel_y - fish.velocity.y) * avg_vel_factor;
        }

        // apply velocity so that fishes gravitate towards the center of
        // of fishes near them.
        if (avg_pos_count != 0 && fish.chasing_food_id == -1) {
            avg_pos_x /= avg_pos_count;
            avg_pos_y /= avg_pos_count;
            dx += (avg_pos_x - fish.position.x) * centering_factor;
            dy += (avg_pos_y - fish.position.y) * centering_factor;
        }

        // if too close to the border of the window apply a turn factor to
        // avoid it.
        bool turning = false;
        if (fish.chasing_food_id == -1) {
            if (fish.position.x < 100 + sim.gui_width) {
                dx += turn_factor;
                turning = true;
            }
            if (fish.position.x > sim.res_width - 100) {
                dx -= turn_factor;
                turning = true;
            }
            if (fish.position.y < 100) {
                dy += turn_factor;
                turning = true;
            }
            if (fish.position.y > sim.res_height - 100) {
                dy -= turn_factor;
                turning = true;
            }
        }


        if (turning) {
            dx *= 1.3f;
            dy *= 1.3f;
        }

        fish.velocity.x += dx * dt;
        fish.velocity.y += dy * dt;
    }

    for (Fish& fish : sim.fishes) {
        // limit the speed of the velocity.
        float min_speed = 0.05f;//0.2f;
        if (fish.chasing_food_id != -1) {
            min_speed = 0.7f;
        }
        float speed = Vector2Length(fish.velocity);
        if (speed < 1e-4) {
            fish.velocity.x = min_speed;
            fish.velocity.y = min_speed;
        } else if (speed < min_speed) {
            Vector2 norm = Vector2Normalize(fish.velocity);
            fish.velocity.x = norm.x * min_speed;
            fish.velocity.y = norm.y * min_speed;
        } else if (speed > fish.max_speed) {
            Vector2 norm = Vector2Normalize(fish.velocity);
            fish.velocity.x = norm.x * fish.max_speed;
            fish.velocity.y = norm.y * fish.max_speed;
        }
    }
}

static void fish_update_position_and_angle(Simulation& sim) {
    for (Fish& fish : sim.fishes) {
        float dx = fish.velocity.x;
        float dy = fish.velocity.y;

        fish.position.x += dx;
        fish.position.y += dy;

        // clamp the fish position to stay in the bounds of the window.
        fish.position.x = std::clamp<float>(fish.position.x, sim.gui_width + 16, sim.res_width - 16);
        fish.position.y = std::clamp<float>(fish.position.y, 16, sim.res_height - 16);

        // change direction to face velocity but do not change direction too quickly.
        float new_angle = atan2(dy, dx);
        if (fabs(new_angle - fish.angle) > PI/64.f) {

            float a = new_angle - fish.angle;
            a = std::fmod(a, 2 *PI);
            if (a >= PI)  {
                a -= 2 * PI;
            }
            if (a < -PI) {
                a +=  2 * PI;
            }
            fish.angle += a * (PI/64.f);

        } else {
            fish.angle = new_angle;
        }
    }
}

static void remove_eaten_fish_food(Simulation& sim) {
    long long now = current_time_milliseconds();
    std::erase_if(sim.fish_foods, [now](const auto& pair){
        const auto& [_, food] = pair;
        long long passed = now - food.time_eaten_start;
        return food.eaten && passed > 300;
    });
}

static void remove_tank_taps(Simulation& sim) {
    long long now = current_time_milliseconds();
    std::erase_if(sim.tank_taps, [now](const TankTap& tap) {
        long long passed = now - tap.start_time;
        return passed > 1000;
    });
}

static Vector2 get_wrapped_point(const SwimPath& path, int index) {
    return path.points[index % path.points.size()];
}
