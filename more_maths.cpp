
#include <raylib.h>
#include <raymath.h>

float degrees(float radians) {
    return radians * (180.f / PI);
}

Vector2 lerp_cubic_bezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t) {
    Vector2 l0 = Vector2Lerp(p0, p1, t);
    Vector2 l1 = Vector2Lerp(p1, p2, t);
    Vector2 l2 = Vector2Lerp(p2, p3, t);
    Vector2 q0 = Vector2Lerp(l0, l1, t);
    Vector2 q1 = Vector2Lerp(l1, l2, t);
    return Vector2Lerp(q0, q1, t);
}
