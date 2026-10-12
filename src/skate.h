#ifndef SKATE_H
#define SKATE_H

#define SKATE_PI 3.14159265358979323846f
#define PARK_EDGE 23.0f
#define RAIL_START -2.0f
#define RAIL_END 6.0f
#define RAIL_HEIGHT 0.9f

typedef struct {
    float steer;
    int push, brake, flip, grab, grind, spin_left, spin_right;
} SkateInput;

typedef struct {
    float x, y, z, heading, speed, vertical_speed;
    float crouch, ground_velocity, spin, flip_remaining, flip_angle;
    float grind_points, grab_time, stun, notice_time;
    int airborne, grinding, flips, grab_counted;
    int combo_points, combo_tricks, total_score, best_combo, last_score;
    char notice[32];
    SkateInput previous;
} Skater;

void skate_init(Skater *skater);
void skate_respawn(Skater *skater);
void skate_step(Skater *skater, const SkateInput *input, float dt);
float park_ground(float x, float z);
float skate_wrap_angle(float angle);

#endif
