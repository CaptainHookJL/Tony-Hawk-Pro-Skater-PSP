#include "skate.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clamp(float value, float low, float high)
{
    return fmaxf(low, fminf(high, value));
}

float skate_wrap_angle(float angle)
{
    while (angle > SKATE_PI) angle -= 2.0f * SKATE_PI;
    while (angle < -SKATE_PI) angle += 2.0f * SKATE_PI;
    return angle;
}

float park_ground(float x, float z)
{
    /* Two triangular banks, matching the renderer's geometry. */
    if ((fabsf(x + 8.0f) <= 3.0f || fabsf(x - 8.0f) <= 3.0f) &&
        z >= 2.0f && z <= 10.0f) {
        return z <= 6.0f ? (z - 2.0f) * 0.5f : (10.0f - z) * 0.5f;
    }
    if (fabsf(x) <= 3.0f && z >= 12.0f && z <= 15.0f) return 0.7f;
    return 0.0f;
}

static void notice(Skater *s, const char *text)
{
    snprintf(s->notice, sizeof(s->notice), "%s", text);
    s->notice_time = 2.0f;
}

static void clear_combo(Skater *s)
{
    s->combo_points = 0;
    s->combo_tricks = 0;
    s->spin = 0.0f;
    s->flip_remaining = 0.0f;
    s->flip_angle = 0.0f;
    s->flips = 0;
    s->grab_time = 0.0f;
    s->grab_counted = 0;
    s->grind_points = 0.0f;
    s->grind_time = 0.0f;
}

void skate_init(Skater *s)
{
    memset(s, 0, sizeof(*s));
    s->z = -12.0f;
    notice(s, "HOLD X TO PUSH - RELEASE TO POP");
}

void skate_respawn(Skater *s)
{
    Skater old = *s;
    skate_init(s);
    s->total_score = old.total_score;
    s->best_combo = old.best_combo;
    s->landed_flips = old.landed_flips;
    s->landed_grabs = old.landed_grabs;
    s->landed_spins = old.landed_spins;
    s->landed_combos = old.landed_combos;
    s->landed_grind_time = old.landed_grind_time;
    s->bails = old.bails;
    notice(s, "RESPAWNED");
}

static void bail(Skater *s)
{
    s->bails++;
    clear_combo(s);
    s->airborne = s->grinding = 0;
    s->speed = s->vertical_speed = s->ground_velocity = s->crouch = 0.0f;
    s->y = park_ground(s->x, s->z);
    s->stun = 0.8f;
    s->last_score = 0;
    notice(s, "BAIL - COMBO LOST");
}

static void jump(Skater *s)
{
    s->airborne = 1;
    s->grinding = 0;
    s->vertical_speed = 6.0f + 2.5f * clamp(s->crouch, 0.0f, 1.0f);
    s->crouch = 0.0f;
    s->ground_velocity = 0.0f;
    notice(s, "OLLIE");
}

static void land(Skater *s, float ground)
{
    int half_turns = (int)roundf(s->spin / SKATE_PI);
    float alignment_error = fabsf(s->spin - (float)half_turns * SKATE_PI);
    s->y = ground;
    if (s->flip_remaining > 0.0f || alignment_error > 0.65f) {
        bail(s);
        return;
    }
    s->heading = skate_wrap_angle(s->heading + (float)half_turns * SKATE_PI);
    if (half_turns) {
        s->combo_points += 180 * (half_turns < 0 ? -half_turns : half_turns);
        s->combo_tricks++;
    }
    s->last_score = s->combo_points * s->combo_tricks;
    s->total_score += s->last_score;
    s->landed_flips += s->flips;
    s->landed_grabs += s->grab_counted;
    s->landed_spins += half_turns < 0 ? -half_turns : half_turns;
    s->landed_grind_time += s->grind_time;
    if (s->last_score > 0) s->landed_combos++;
    if (s->last_score > s->best_combo) s->best_combo = s->last_score;
    notice(s, s->last_score > 0 ? "LANDED - COMBO BANKED" : "LANDED");
    s->airborne = s->grinding = 0;
    s->vertical_speed = s->ground_velocity = 0.0f;
    clear_combo(s);
}

void skate_step(Skater *s, const SkateInput *in, float dt)
{
    float old_ground, ground;
    int jump_released;
    if (!(dt > 0.0f) || dt > 0.05f) return;
    s->notice_time = fmaxf(0.0f, s->notice_time - dt);
    jump_released = s->previous.push && !in->push;
    if (s->stun > 0.0f) {
        s->stun = fmaxf(0.0f, s->stun - dt);
        s->previous = *in;
        return;
    }

    if (s->grinding) {
        s->crouch = in->push ? fminf(1.0f, s->crouch + dt) : s->crouch;
        if (jump_released) jump(s);
        else if (!in->grind || s->z <= RAIL_START || s->z >= RAIL_END) {
            s->grinding = 0;
            s->airborne = 1;
            s->vertical_speed = 0.6f;
        } else {
            s->speed = fmaxf(2.5f, s->speed - 0.35f * dt);
            s->z += cosf(s->heading) * s->speed * dt;
            s->grind_points += 250.0f * dt;
            s->grind_time += dt;
            while (s->grind_points >= 1.0f) {
                s->combo_points++;
                s->grind_points -= 1.0f;
            }
            s->previous = *in;
            return;
        }
    }

    old_ground = park_ground(s->x, s->z);
    if (!s->airborne) {
        float acceleration = in->push ? 4.5f : -0.8f;
        if (in->brake) acceleration = -9.0f;
        s->speed = clamp(s->speed + acceleration * dt, 0.0f, 12.0f);
        s->heading = skate_wrap_angle(s->heading + clamp(in->steer, -1.0f, 1.0f) * 2.0f * dt);
        if (in->push) s->crouch = fminf(1.0f, s->crouch + dt);
        if (jump_released) jump(s);
    }
    s->x += sinf(s->heading) * s->speed * dt;
    s->z += cosf(s->heading) * s->speed * dt;
    ground = park_ground(s->x, s->z);
    if (fabsf(s->x) > PARK_EDGE || fabsf(s->z) > PARK_EDGE) {
        s->x = clamp(s->x, -PARK_EDGE, PARK_EDGE);
        s->z = clamp(s->z, -PARK_EDGE, PARK_EDGE);
        bail(s);
        s->previous = *in;
        return;
    }

    if (!s->airborne) {
        float slope_velocity = (ground - old_ground) / dt;
        if (ground - old_ground > 0.35f) bail(s);
        else if ((s->ground_velocity > 1.8f && slope_velocity < s->ground_velocity - 1.5f) ||
                 old_ground - ground > 0.15f) {
            s->airborne = 1;
            s->vertical_speed = fmaxf(1.0f, s->ground_velocity);
            notice(s, "AIR OFF THE RAMP");
        } else {
            s->y = ground;
            s->ground_velocity = slope_velocity;
        }
    }

    if (s->airborne) {
        s->vertical_speed -= 14.0f * dt;
        s->y += s->vertical_speed * dt;
        if (in->flip && !s->previous.flip && s->flip_remaining <= 0.0f) {
            s->flip_remaining = 0.42f;
            s->flip_angle = 0.0f;
            notice(s, "KICKFLIP");
        }
        if (s->flip_remaining > 0.0f) {
            s->flip_remaining = fmaxf(0.0f, s->flip_remaining - dt);
            s->flip_angle = (1.0f - s->flip_remaining / 0.42f) * 2.0f * SKATE_PI;
            if (s->flip_remaining == 0.0f) {
                s->combo_points += s->flips == 0 ? 100 : 50;
                s->combo_tricks++;
                s->flips++;
                s->flip_angle = 0.0f;
            }
        }
        s->spin += (float)(in->spin_right - in->spin_left) * 2.0f * SKATE_PI * dt;
        if (in->grab) {
            s->grab_time += dt;
            if (!s->grab_counted && s->grab_time >= 0.15f) {
                s->combo_points += 100;
                s->combo_tricks++;
                s->grab_counted = 1;
                notice(s, "INDY GRAB");
            }
        }
        if (in->grind && fabsf(s->x) < 0.65f && s->z > RAIL_START && s->z < RAIL_END &&
            s->y >= RAIL_HEIGHT - 0.3f && s->y < RAIL_HEIGHT + 0.35f &&
            s->vertical_speed <= 1.0f && fabsf(sinf(s->heading)) < 0.45f &&
            s->speed > 1.0f && s->flip_remaining <= 0.0f) {
            s->x = 0.0f;
            s->y = RAIL_HEIGHT;
            s->heading = cosf(s->heading) >= 0.0f ? 0.0f : SKATE_PI;
            s->vertical_speed = 0.0f;
            s->grinding = 1;
            s->airborne = 0;
            s->spin = 0.0f;
            s->combo_points += 100;
            s->combo_tricks++;
            notice(s, "50-50 GRIND");
        } else if (s->y <= ground && s->vertical_speed <= 0.0f) {
            land(s, ground);
        }
    }
    s->previous = *in;
}
