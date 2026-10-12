#define _POSIX_C_SOURCE 200809L
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int checks, failures;
#define CHECK(condition) do { \
    checks++; \
    if (!(condition)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); failures++; } \
} while (0)

static void advance(SkateSession *g, SkateInput input, int ticks)
{
    int i;
    for (i=0;i<ticks;i++) session_step(g,&input,1.0f/120.0f);
}

static void combo(SkateSession *g, int spin)
{
    SkateInput in = {0};
    session_respawn(g);
    g->skater.x = -16.0f;
    in.push = 1; advance(g,in,120);
    in.push = 0; advance(g,in,1);
    in.flip = 1; in.grab = 1; in.spin_right = spin;
    advance(g,in,120);
    in.flip = in.grab = in.spin_right = 0;
    advance(g,in,50);
}

static void grind(SkateSession *g)
{
    SkateInput in = {0};
    session_respawn(g);
    g->skater.x = 0.1f; g->skater.z = -1.5f; g->skater.y = 1.05f;
    g->skater.speed = 3.0f; g->skater.vertical_speed = -2.0f; g->skater.airborne = 1;
    in.grind = 1; advance(g,in,1); advance(g,in,120);
    CHECK(g->skater.grinding);
    CHECK(!(g->goals & (1u<<3)));
    in.grind = 0; advance(g,in,100);
}

static void test_menu_pause_and_retry(void)
{
    SkateSession g;
    SkateInput in = {0};
    PlayerRecords records = {700,400,1,2};
    float z,time;
    session_init(&g,&records);
    in.push = 1; advance(&g,in,240);
    CHECK(g.state==GAME_MENU && g.remaining==RUN_SECONDS && g.skater.z==-12.0f);
    session_start(&g,MODE_SCORE);
    advance(&g,in,120);
    z = g.skater.z; time = g.remaining;
    g.paused = 1; in.push = 0; advance(&g,in,240);
    CHECK(g.skater.z==z && g.remaining==time);
    g.paused = 0; advance(&g,in,1);
    CHECK(!g.skater.airborne);
    session_finish(&g);
    CHECK(g.state==GAME_RESULTS && g.records.best_score==700 && g.records.completed_runs==3);
    session_finish(&g);
    CHECK(g.records.completed_runs==3);
    in.push = 1; z = g.skater.z; advance(&g,in,120);
    CHECK(g.skater.z==z);
    session_start(&g,MODE_SCORE);
    CHECK(g.state==GAME_RUNNING && g.remaining==RUN_SECONDS && g.skater.total_score==0);
    CHECK(g.goals==0 && g.spots==0 && g.records.best_score==700);
    session_menu(&g);
    CHECK(g.state==GAME_MENU && g.records.completed_runs==3);
    puts("menu, pause, button release, results, and retry checked");
}

static void test_goals_and_banking(void)
{
    SkateSession g;
    SkateInput in = {0};
    PlayerRecords records = {500,200,0,1};
    int i;
    session_init(&g,&records); session_start(&g,MODE_FREE);
    for (i=0;i<3;i++) combo(&g,0);
    CHECK(g.skater.total_score==1200 && g.skater.landed_flips==3);
    CHECK(g.goals & (1u<<2));
    CHECK(session_rank(&g)==RANK_BRONZE);
    g.skater.airborne = 1; g.skater.y = 0.01f; g.skater.vertical_speed = -3;
    g.skater.combo_points = 900; g.skater.combo_tricks = 2; g.skater.flips = 2;
    in.flip = 1; advance(&g,in,1);
    CHECK(g.skater.total_score==1200 && g.skater.landed_flips==3 && g.skater.bails==1);
    grind(&g);
    CHECK(g.goals & (1u<<3));
    CHECK(g.skater.landed_grind_time>=1.0f-0.00001f);
    session_respawn(&g);
    CHECK(g.skater.landed_flips==3 && g.skater.landed_grind_time>=1.0f-0.00001f);
    combo(&g,1);
    CHECK(g.skater.best_combo>=1000 && (g.goals & (1u<<1)));
    CHECK((g.goals & 1u) && session_rank(&g)==RANK_SILVER);
    combo(&g,1);
    combo(&g,0);
    CHECK(g.skater.total_score>=5000 && session_rank(&g)==RANK_SILVER);
    memset(&in,0,sizeof(in));
    for (i=0;i<SPOT_COUNT;i++) {
        float x,y,z;
        session_spot(i,&x,&y,&z);
        session_respawn(&g);
        g.skater.x=x; g.skater.z=z; g.skater.y=park_ground(x,z);
        advance(&g,in,1);
        CHECK(session_count(g.spots)==i+1);
    }
    advance(&g,in,20);
    CHECK(session_count(g.spots)==SPOT_COUNT && session_count(g.goals)==GOAL_COUNT);
    CHECK(session_rank(&g)==RANK_GOLD);
    CHECK(g.remaining==RUN_SECONDS);
    session_finish(&g);
    CHECK(g.records.best_score==500 && g.records.completed_runs==1);
    CHECK(g.records.best_combo==g.skater.best_combo);
    puts("banked trick/grind goals, bails, pickups, medal progression, and Free Skate checked");
}

static void test_deadline_and_overtime(void)
{
    SkateSession g;
    SkateInput in = {0};
    session_init(&g,0); session_start(&g,MODE_SCORE);
    advance(&g,in,14500);
    CHECK(g.state==GAME_RESULTS && g.remaining==0.0f && g.records.completed_runs==1);
    session_start(&g,MODE_SCORE);
    g.skater.x=-16;
    in.push=1; advance(&g,in,120);
    in.push=0; advance(&g,in,1);
    in.flip=1; advance(&g,in,1);
    g.remaining=0.025f; advance(&g,in,4);
    CHECK(g.state==GAME_FINISHING && g.skater.airborne);
    advance(&g,in,160);
    CHECK(g.state==GAME_RESULTS && g.skater.total_score==100);
    CHECK(g.records.best_score==100 && g.records.completed_runs==2);
    session_start(&g,MODE_SCORE);
    g.remaining=0.01f; g.skater.airborne=1; g.skater.y=100;
    g.skater.combo_points=500; g.skater.combo_tricks=2;
    memset(&in,0,sizeof(in)); advance(&g,in,400);
    CHECK(g.state==GAME_RESULTS && g.skater.total_score==0 && g.skater.combo_points==0);
    CHECK(g.records.best_score==100);
    puts("full score-run deadline, landing grace, and bounded unfinished combos checked");
}

static void test_records(void)
{
    char path[]="/tmp/sidewalk-records-test-XXXXXX";
    char backup[256];
    PlayerRecords original={5300,1680,5,7}, loaded, invalid=original;
    FILE *file;
    int descriptor=mkstemp(path);
    CHECK(descriptor>=0);
    if (descriptor<0) return;
    close(descriptor);
    snprintf(backup,sizeof(backup),"%s.bak",path);
    CHECK(records_save(path,&original));
    CHECK(records_load(path,&loaded));
    CHECK(loaded.best_score==5300 && loaded.best_combo==1680 && loaded.best_goals==5 && loaded.completed_runs==7);
    original.best_score=7000;
    CHECK(records_save(path,&original) && records_load(path,&loaded) && loaded.best_score==7000);
    invalid.best_goals=6;
    CHECK(!records_save(path,&invalid));
    CHECK(records_load(path,&loaded) && loaded.best_score==7000);
    file=fopen(path,"w");
    CHECK(file!=0);
    if (file) { fputs("SIDEWALK1\n999999999999999999999 0 0 0\n",file); fclose(file); }
    CHECK(records_load(path,&loaded) && loaded.best_score==5300);
    file=fopen(path,"w");
    if (file) { fputs("BROKEN\n12 12 1 1\n",file); fclose(file); }
    CHECK(records_load(path,&loaded) && loaded.best_score==5300);
    remove(path);
    CHECK(records_load(path,&loaded) && loaded.completed_runs==7);
    CHECK(records_save(path,&original) && records_load(path,&loaded) && loaded.best_score==7000);
    remove(path);
    remove(backup);
    CHECK(!records_load(path,&loaded) && loaded.completed_runs==0);
    puts("record persistence, replacement, backup recovery, and corrupt saves checked");
}

int main(void)
{
    test_menu_pause_and_retry();
    test_goals_and_banking();
    test_deadline_and_overtime();
    test_records();
    printf("%d session checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
