#include "simulation.h"

#include <algorithm>
#include <iostream>
#include <raymath.h>

static Vector2 lerp_cubic_bezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t);

void generate_swim_path(Simulation& sim, SwimPath& swim_path, int start_index) {

    swim_path.points.clear();

    // TODO (maddie): the radius differences cause points to not be and create sharp edges along the path.

    // traversing along the arc path by randomized steps so that the points never get
    // too close to each other.
    const int AVG_POINTS = 16;
    const float STEP = (2 * PI) / AVG_POINTS;

    std::uniform_real_distribution<float> step_dist(STEP - STEP/2.f, STEP);
    std::uniform_real_distribution<float> radius_dist(-100.f, +5.f);

    float w = static_cast<float>(sim.res_width);
    float h = static_cast<float>(sim.res_height);

    // TODO (maddie): feels as if the last point cuts off a bit early.
    float angle = step_dist(sim.gen);
    while (angle < (2 * PI - STEP/2.f)) {
        angle += step_dist(sim.gen);

        float rx = ((w - 200 + radius_dist(sim.gen)) / 2);
        float ry = ((h - 200 + radius_dist(sim.gen)) / 2);

        float ax = sinf(angle) * rx;
        float ay = cosf(angle) * ry;

        float x = w / 2 + ax;
        float y = h / 2 + ay;

        swim_path.points.push_back(Vector2{ x, y });
    }

    swim_path.start_index = start_index;
    swim_path.end_index = start_index + 1;
    swim_path.t = 0;
}

void update_swim_point(SwimPath& swim_path, float dt) {

    // Account for the path length when traversing so that swimming moves at a steady
    // state.
    Vector2 path_start_point = swim_path.points[swim_path.start_index];
    Vector2 path_end_point = swim_path.points[swim_path.end_index];
    float path_length = Vector2Distance(path_start_point, path_end_point);

    swim_path.t += dt * (100.f/path_length);
    if (swim_path.t > 1) {
        // TODO (maddie): we want to move to the next pair of points.
        ++swim_path.start_index;
        ++swim_path.end_index;
        swim_path.start_index %= swim_path.points.size();
        swim_path.end_index %= swim_path.points.size();
    }
    while (swim_path.t > 1) {
        swim_path.t -= 1;
    }
    path_start_point = swim_path.points[swim_path.start_index];
    path_end_point = swim_path.points[swim_path.end_index];

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

    // DrawCircleV(p0, 4.f, ORANGE);
    // DrawCircleV(p1, 4.f, PURPLE);
    // DrawCircleV(p2, 4.f, PURPLE);
    // DrawCircleV(p3, 4.f, ORANGE);

    swim_path.cur_point = lerp_cubic_bezier(p0, p1, p2, p3, swim_path.t);
}


void update_fishes(Simulation& sim, float dt) {
    // https://vanhunteradams.com/Pico/Animal_Movement/Boids-algorithm.html
    const float avoid_factor = 0.2f;//0.1f; //0.06f;
    const float alignment_factor = 0.02f;//0.015f;
    const float avg_vel_factor = 0.01f;
    const float centering_factor = 0.005f;
    const float turn_factor = 1.5f;
    for (Fish& fish : sim.fishes) {
        float dx = (fish.swim_path.cur_point.x - fish.position.x) * alignment_factor;
        float dy = (fish.swim_path.cur_point.y - fish.position.y) * alignment_factor;

        float close_dx = 0;
        float close_dy = 0;
        int count = 0;

        float avg_vel_x = 0;
        float avg_vel_y = 0;
        float avg_pos_x = 0;
        float avg_pos_y = 0;
        int avg_vel_count = 0;
        int avg_pos_count = 0;
        for (const Fish& o_fish : sim.fishes) {
            if (&fish == &o_fish) continue;
            float d = Vector2Distance(fish.position, o_fish.position);
            if (d < 20.f) {
                float diff_x = fish.position.x - o_fish.position.x;
                float diff_y = fish.position.y - o_fish.position.y;
                if (d < 4.f) {
                    close_dx += diff_x*16.f;
                    close_dy += diff_y*16.f;
                } else {
                    close_dx += diff_x;
                    close_dy += diff_y;
                }
            }
            if (d < 50.f) {
                avg_vel_x += fish.velocity.x;
                avg_vel_y += fish.velocity.y;
                ++avg_vel_count;
            }
            //if (d < 15.f) {
            if (d < 5.f) {
                avg_pos_x += fish.velocity.x;
                avg_pos_y += fish.velocity.y;
                ++avg_pos_count;
            }
        }
        dx += close_dx*avoid_factor;
        dy += close_dy*avoid_factor;

        if (avg_vel_count != 0) {
            avg_vel_x /= avg_vel_count;
            avg_vel_y /= avg_vel_count;
            dx += (avg_vel_x - fish.velocity.x) * avg_vel_factor;
            dy += (avg_vel_y - fish.velocity.y) * avg_vel_factor;
        }
        if (avg_pos_count != 0) {
            avg_pos_x /= avg_pos_count;
            avg_pos_y /= avg_pos_count;
            dx += (avg_pos_x - fish.position.x) * centering_factor;
            dy += (avg_pos_y - fish.position.y) * centering_factor;
        }

        if (fish.position.x < 100) {
            dx += turn_factor;
        }
        if (fish.position.x > sim.res_width - 100) {
            dx -= turn_factor;
        }
        if (fish.position.y < 100) {
            dy += turn_factor;
        }
        if (fish.position.y > sim.res_height - 100) {
            dy -= turn_factor;
        }

        // std::cout << "dx: " << dx << "\n";

        fish.velocity.x += dx * dt;
        fish.velocity.y += dy * dt;

        // limit the speed of the velocity.
        float speed = Vector2Length(fish.velocity);
        if (speed < 1e-4) {
            fish.velocity.x = 0.2f;
            fish.velocity.y = 0.2f;
        } else if (speed < 0.2f) {
            Vector2 norm = Vector2Normalize(fish.velocity);
            fish.velocity.x = norm.x * 0.2f;
            fish.velocity.y = norm.y * 0.2f;
        } else if (speed > fish.max_speed) {
            Vector2 norm = Vector2Normalize(fish.velocity);
            fish.velocity.x = norm.x * fish.max_speed;
            fish.velocity.y = norm.y * fish.max_speed;
        }
        // std::cout << "vel: " << fish.velocity.x << ", " << fish.velocity.y << "\n";
    }

    for (Fish& fish : sim.fishes) {
        float dx = fish.velocity.x;
        float dy = fish.velocity.y;

        fish.position.x += dx;
        fish.position.y += dy;

        // clamp the fish position to stay in the bounds of the window.
        fish.position.x = std::clamp<float>(fish.position.x, 16, sim.res_width - 16);
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

        if (fish.angle > 31515) {
            std::cout << "assigned angle: " << fish.angle << "\n";
            std::cout << "dx,dy: " << dx << "," << dy << "\n";
        }
    }
}

// -------------------------
// math

static Vector2 lerp_cubic_bezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t) {
    Vector2 l0 = Vector2Lerp(p0, p1, t);
    Vector2 l1 = Vector2Lerp(p1, p2, t);
    Vector2 l2 = Vector2Lerp(p2, p3, t);
    Vector2 q0 = Vector2Lerp(l0, l1, t);
    Vector2 q1 = Vector2Lerp(l1, l2, t);
    return Vector2Lerp(q0, q1, t);
}
