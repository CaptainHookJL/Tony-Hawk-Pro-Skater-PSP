#include "skate.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int checks, failures;
#define CHECK(condition) do { \
    checks++; \
    if (!(condition)) { \
        fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); \
        failures++; \
    } \
} while (0)

static void advance(Skater *s, SkateInput in, int frames)
{
    int n;
    for (n=0;n<frames;n++) skate_step(s,&in,1.0f/120.0f);
}

static void prepare_ollie(Skater *s)
{
    SkateInput in = {0};
    skate_init(s);
    s->x = -16.0f;
    in.push = 1;
    advance(s,in,120);
    in.push = 0;
    advance(s,in,1);
}

static void test_push_brake_ollie(void)
{
    Skater s;
    SkateInput in = {0};
    skate_init(&s);
    in.push = 1;
    advance(&s,in,120);
    CHECK(fabsf(s.speed-4.5f)<0.01f);
    CHECK(s.z > -10.0f && s.z < -9.5f);
    CHECK(!s.airborne);
    in.brake = 1;
    advance(&s,in,90);
    CHECK(s.speed==0.0f);
    CHECK(!s.airborne);
    prepare_ollie(&s);
    CHECK(s.airborne && s.vertical_speed>8.0f && s.y>0.0f);
    memset(&in,0,sizeof(in));
    advance(&s,in,180);
    CHECK(!s.airborne && s.y==0.0f && s.stun==0.0f);
    CHECK(s.total_score==0);
    puts("push, braking, charged ollie, and landing checked");
}

static void test_tricks_and_bails(void)
{
    Skater s;
    SkateInput in = {0};
    int frames;
    prepare_ollie(&s);
    in.flip = 1;
    advance(&s,in,55);
    CHECK(s.flips==1 && s.combo_points==100);
    in.flip = 0;
    in.grab = 1;
    advance(&s,in,20);
    CHECK(s.combo_points==200 && s.combo_tricks==2);
    in.grab = 0;
    advance(&s,in,160);
    CHECK(!s.airborne && s.total_score==400 && s.best_combo==400);
    CHECK(s.combo_points==0);
    /* A late flip must lose the pending combo, without erasing banked points. */
    s.airborne = 1;
    s.y = 0.01f;
    s.vertical_speed = -3.0f;
    s.combo_points = 500;
    s.combo_tricks = 2;
    in.flip = 1;
    advance(&s,in,1);
    CHECK(s.stun>0.0f && s.total_score==400 && s.combo_points==0);
    skate_respawn(&s);
    CHECK(s.total_score==400 && s.best_combo==400 && s.z==-12.0f);

    prepare_ollie(&s);
    memset(&in,0,sizeof(in));
    in.spin_right = 1;
    advance(&s,in,60);
    in.spin_right = 0;
    advance(&s,in,150);
    CHECK(s.total_score==180 && s.stun==0.0f);
    CHECK(fabsf(fabsf(s.heading)-SKATE_PI)<0.01f);
    prepare_ollie(&s);
    in.spin_right = 1;
    advance(&s,in,30);
    in.spin_right = 0;
    for (frames=0; frames<200 && s.airborne; frames++) advance(&s,in,1);
    CHECK(!s.airborne);
    CHECK(s.stun>0.0f && s.total_score==0);
    puts("flip/grab combos, 180 landings, failed tricks, and score preservation checked");
}

static void test_rail_and_park(void)
{
    Skater s;
    SkateInput in = {0};
    skate_init(&s);
    s.x = 0.1f; s.z = -1.0f; s.y = 1.05f;
    s.speed = 6.0f; s.vertical_speed = -2.0f; s.airborne = 1;
    in.grind = 1;
    advance(&s,in,1);
    CHECK(s.grinding && !s.airborne && s.x==0.0f && s.y==RAIL_HEIGHT);
    advance(&s,in,60);
    CHECK(s.grinding && s.combo_points>=224);
    in.grind = 0;
    advance(&s,in,100);
    CHECK(!s.grinding && !s.airborne && s.total_score>=224);
    skate_init(&s);
    s.x = 2.0f; s.z = 0.0f; s.y = 1.05f;
    s.speed = 6.0f; s.vertical_speed = -2.0f; s.airborne = 1;
    in.grind = 1;
    advance(&s,in,1);
    CHECK(!s.grinding);

    skate_init(&s);
    s.x = -8.0f; s.z = 2.1f; s.y = park_ground(s.x,s.z); s.speed = 10.0f;
    memset(&in,0,sizeof(in));
    advance(&s,in,52);
    CHECK(s.airborne && s.y>2.0f);
    CHECK(park_ground(-8,6)==2.0f && park_ground(8,4)==1.0f);
    CHECK(park_ground(0,13)==0.7f && park_ground(0,0)==0.0f);
    skate_init(&s);
    s.x = -16; s.z = PARK_EDGE-0.01f; s.speed = 12;
    advance(&s,in,1);
    CHECK(s.stun>0.0f && s.z==PARK_EDGE && s.speed==0);
    skate_init(&s);
    s.z = 11.98f; s.speed = 12;
    advance(&s,in,1);
    CHECK(s.stun>0.0f);
    puts("rail catch/release, ramp launch, ledge collision, and park boundaries checked");
}

static void test_long_session(void)
{
    Skater a,b;
    unsigned int rng = 7;
    int n;
    skate_init(&a); skate_init(&b);
    for (n=0;n<36000;n++) {
        SkateInput in = {0};
        rng = 1664525u*rng+1013904223u;
        in.steer = (float)((int)((rng>>16)%201)-100)/100.0f;
        in.push = n%240<200;
        in.flip = n%90<10;
        in.grab = n%150<30;
        in.grind = n%80<40;
        in.spin_right = n%360<45;
        skate_step(&a,&in,1.0f/120.0f);
        skate_step(&b,&in,1.0f/120.0f);
        if (!isfinite(a.x) || !isfinite(a.y) || !isfinite(a.z) || !isfinite(a.speed)) {
            CHECK(0); return;
        }
    }
    CHECK(fabsf(a.x)<=PARK_EDGE && fabsf(a.z)<=PARK_EDGE && a.y>=0.0f);
    CHECK(a.x==b.x && a.y==b.y && a.z==b.z && a.total_score==b.total_score);
    CHECK(a.speed>=0.0f && a.speed<=12.0f);
    puts("five-minute deterministic session checked for finite state and bounds");
}

int main(void)
{
    test_push_brake_ollie();
    test_tricks_and_bails();
    test_rail_and_park();
    test_long_session();
    printf("%d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
