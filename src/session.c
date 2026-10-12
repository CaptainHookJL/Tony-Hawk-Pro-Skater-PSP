#include "session.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const float spot_positions[SPOT_COUNT][2] = {
    {-15.0f,-6.0f}, {15.0f,-6.0f}, {-8.0f,6.0f}, {8.0f,6.0f}, {0.0f,19.0f}
};

int session_count(unsigned int bits)
{
    int count = 0;
    while (bits) { count += (int)(bits & 1u); bits >>= 1; }
    return count;
}

void session_spot(int index, float *x, float *y, float *z)
{
    *x = spot_positions[index][0];
    *z = spot_positions[index][1];
    *y = park_ground(*x,*z) + 1.05f;
}

void session_init(SkateSession *g, const PlayerRecords *records)
{
    memset(g,0,sizeof(*g));
    if (records) g->records = *records;
    skate_init(&g->skater);
    g->remaining = RUN_SECONDS;
    g->state = GAME_MENU;
}

void session_start(SkateSession *g, GameMode mode)
{
    PlayerRecords records = g->records;
    session_init(g,&records);
    g->mode = mode;
    g->state = GAME_RUNNING;
}

void session_menu(SkateSession *g)
{
    PlayerRecords records = g->records;
    session_init(g,&records);
}

static void update_goals(SkateSession *g)
{
    if (g->skater.total_score >= 2500) g->goals |= 1u << 0;
    if (g->skater.best_combo >= 1000) g->goals |= 1u << 1;
    if (g->skater.landed_flips >= 3) g->goals |= 1u << 2;
    if (g->skater.landed_grind_time >= 1.0f-0.00001f) g->goals |= 1u << 3;
    if (session_count(g->spots) == SPOT_COUNT) g->goals |= 1u << 4;
}

void session_finish(SkateSession *g)
{
    int goals;
    if (g->state != GAME_RUNNING && g->state != GAME_FINISHING) return;
    update_goals(g);
    goals = session_count(g->goals);
    if (g->mode == MODE_SCORE) {
        if (g->skater.total_score > g->records.best_score) g->records.best_score = g->skater.total_score;
        if (goals > g->records.best_goals) g->records.best_goals = goals;
        if (g->records.completed_runs < 999999) g->records.completed_runs++;
    }
    if (g->skater.best_combo > g->records.best_combo) g->records.best_combo = g->skater.best_combo;
    g->skater.combo_points = g->skater.combo_tricks = 0;
    g->paused = 0;
    g->state = GAME_RESULTS;
}

void session_respawn(SkateSession *g)
{
    if (g->state != GAME_RUNNING) return;
    skate_respawn(&g->skater);
}

void session_step(SkateSession *g, const SkateInput *in, float dt)
{
    SkateInput limited = *in;
    int i;
    if (!(dt > 0.0f) || dt > 0.05f) return;
    if (g->state == GAME_MENU || g->state == GAME_RESULTS || g->paused) {
        g->skater.previous = *in;
        return;
    }
    if (g->state == GAME_FINISHING) {
        /* Finish the existing combo; do not start fresh flips, grabs, or ollies. */
        limited.push = limited.flip = 0;
        limited.grab = in->grab && g->skater.grab_time > 0.0f;
        g->skater.previous.push = 0;
        skate_step(&g->skater,&limited,dt);
        update_goals(g);
        g->finish_remaining = fmaxf(0.0f,g->finish_remaining-dt);
        if ((!g->skater.airborne && !g->skater.grinding) || g->finish_remaining == 0.0f)
            session_finish(g);
        return;
    }

    skate_step(&g->skater,in,dt);
    for (i=0;i<SPOT_COUNT;i++) {
        float x,y,z,dx,dz;
        if (g->spots & (1u << i)) continue;
        session_spot(i,&x,&y,&z);
        dx = g->skater.x-x; dz = g->skater.z-z;
        if (dx*dx+dz*dz < 1.4f*1.4f && fabsf(g->skater.y+0.8f-y)<1.4f) {
            g->spots |= 1u << i;
            snprintf(g->skater.notice,sizeof(g->skater.notice),"SPOT FOUND %d/%d",session_count(g->spots),SPOT_COUNT);
            g->skater.notice_time = 2.0f;
        }
    }
    update_goals(g);
    if (g->mode == MODE_SCORE) {
        g->remaining = fmaxf(0.0f,g->remaining-dt);
        if (g->remaining == 0.0f) {
            if (g->skater.airborne || g->skater.grinding) {
                g->state = GAME_FINISHING;
                g->finish_remaining = FINISH_SECONDS;
            } else session_finish(g);
        }
    }
}

RunRank session_rank(const SkateSession *g)
{
    int count = session_count(g->goals);
    if (g->skater.total_score >= 5000 && count == GOAL_COUNT) return RANK_GOLD;
    if (g->skater.total_score >= 2500 && count >= 3) return RANK_SILVER;
    if (g->skater.total_score >= 1000) return RANK_BRONZE;
    return RANK_NONE;
}

const char *session_rank_name(RunRank rank)
{
    static const char *names[] = {"KEEP ROLLING","BRONZE","SILVER","GOLD"};
    return names[rank];
}

const char *session_goal_name(int goal)
{
    static const char *names[GOAL_COUNT] = {
        "SCORE 2,500 POINTS", "BANK A 1,000 POINT COMBO", "LAND 3 KICKFLIPS",
        "LAND 1 SECOND OF GRINDING", "FIND ALL 5 SPOT MARKERS"
    };
    return names[goal];
}

void session_goal_progress(const SkateSession *g, int goal, char *text, unsigned int size)
{
    switch (goal) {
    case 0: snprintf(text,size,"%d/2500",g->skater.total_score); break;
    case 1: snprintf(text,size,"%d/1000",g->skater.best_combo); break;
    case 2: snprintf(text,size,"%d/3",g->skater.landed_flips); break;
    case 3: snprintf(text,size,"%.2f/1.00s",g->skater.landed_grind_time); break;
    default: snprintf(text,size,"%d/5",session_count(g->spots)); break;
    }
}
