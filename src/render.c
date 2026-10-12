#include "render.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

#define WIDTH 480
#define HEIGHT 272
#define STRIDE 512
#define FRAME_BYTES (STRIDE * HEIGHT * 4)
#define COLOR(r, g, b) GU_RGBA(r, g, b, 255)

typedef struct { unsigned int color; float x, y, z; } Vertex;
typedef ScePspFVector3 Vec;
static unsigned int command_list[262144] __attribute__((aligned(16)));
static void *draw_buffer;
static float camera_heading;
static Vec camera;
static int camera_ready;
static float animation_time;

static Vec vec(float x, float y, float z)
{
    Vec v = {x, y, z};
    return v;
}

static Vertex vertex(Vec p, unsigned int color)
{
    Vertex v = {color, p.x, p.y, p.z};
    return v;
}

static void triangle(Vec a, Vec b, Vec c, unsigned int color)
{
    Vertex *v = sceGuGetMemory(3 * sizeof(Vertex));
    v[0] = vertex(a, color); v[1] = vertex(b, color); v[2] = vertex(c, color);
    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 3, 0, v);
}

static void quad(Vec a, Vec b, Vec c, Vec d, unsigned int color)
{
    Vertex *v = sceGuGetMemory(6 * sizeof(Vertex));
    v[0] = vertex(a, color); v[1] = vertex(b, color); v[2] = vertex(c, color);
    v[3] = vertex(a, color); v[4] = vertex(c, color); v[5] = vertex(d, color);
    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, 0, v);
}

static unsigned int shade(unsigned int c, float factor)
{
    unsigned int r = (unsigned int)((float)(c & 255u) * factor);
    unsigned int g = (unsigned int)((float)((c >> 8) & 255u) * factor);
    unsigned int b = (unsigned int)((float)((c >> 16) & 255u) * factor);
    return COLOR(r, g, b);
}

static void box(float x0, float y0, float z0, float x1, float y1, float z1, unsigned int c)
{
    Vec a = vec(x0,y0,z0), b = vec(x1,y0,z0), d = vec(x0,y0,z1), e = vec(x1,y0,z1);
    Vec f = vec(x0,y1,z0), g = vec(x1,y1,z0), h = vec(x0,y1,z1), i = vec(x1,y1,z1);
    quad(f,g,i,h,c);
    quad(a,b,g,f,shade(c,0.8f));
    quad(d,e,i,h,shade(c,0.8f));
    quad(a,d,h,f,shade(c,0.65f));
    quad(b,e,i,g,shade(c,0.65f));
    quad(a,b,e,d,shade(c,0.5f));
}

static void bank(float center)
{
    float l = center - 3.0f, r = center + 3.0f;
    quad(vec(l,0,2),vec(r,0,2),vec(r,2,6),vec(l,2,6),COLOR(183,164,133));
    quad(vec(l,2,6),vec(r,2,6),vec(r,0,10),vec(l,0,10),COLOR(145,130,105));
    triangle(vec(l,0,2),vec(l,2,6),vec(l,0,10),COLOR(106,98,84));
    triangle(vec(r,0,2),vec(r,2,6),vec(r,0,10),COLOR(106,98,84));
    box(l,1.99f,5.96f,r,2.04f,6.04f,COLOR(224,194,65));
}

static void park(void)
{
    int x, z, n = 0;
    Vertex *v = sceGuGetMemory(12 * 12 * 6 * sizeof(Vertex));
    for (x = -24; x < 24; x += 4) {
        for (z = -24; z < 24; z += 4) {
            unsigned int c = ((x/4 + z/4) & 1) ? COLOR(91,109,116) : COLOR(98,117,123);
            Vec a = vec((float)x,0,(float)z), b = vec((float)x+4,0,(float)z);
            Vec d = vec((float)x,0,(float)z+4), e = vec((float)x+4,0,(float)z+4);
            v[n++] = vertex(a,c); v[n++] = vertex(b,c); v[n++] = vertex(e,c);
            v[n++] = vertex(a,c); v[n++] = vertex(e,c); v[n++] = vertex(d,c);
        }
    }
    sceGumDrawArray(GU_TRIANGLES,GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,n,0,v);
    box(-24,0,-24,-23.5f,1.2f,24,COLOR(105,124,141));
    box(23.5f,0,-24,24,1.2f,24,COLOR(105,124,141));
    box(-24,0,-24,24,1.2f,-23.5f,COLOR(105,124,141));
    box(-24,0,23.5f,24,1.2f,24,COLOR(105,124,141));
    bank(-8.0f);
    bank(8.0f);
    box(-3,0,12,3,0.7f,15,COLOR(154,163,166));
    box(-3,0.69f,11.97f,3,0.74f,12.04f,COLOR(226,194,62));
    box(-0.065f,RAIL_HEIGHT-0.08f,RAIL_START,0.065f,RAIL_HEIGHT,RAIL_END,COLOR(213,228,234));
    box(-0.04f,0,RAIL_START+0.5f,0.04f,RAIL_HEIGHT,RAIL_START+0.6f,COLOR(138,164,182));
    box(-0.04f,0,RAIL_END-0.6f,0.04f,RAIL_HEIGHT,RAIL_END-0.5f,COLOR(138,164,182));
    /* An orange stripe points from spawn toward the rail. */
    quad(vec(-0.12f,0.01f,-11),vec(0.12f,0.01f,-11),vec(0.12f,0.01f,-3),vec(-0.12f,0.01f,-3),COLOR(219,149,64));
    /* Original industrial scenery beyond the playable boundary. */
    box(-29,0,25,-7,7,32,COLOR(83,115,122));
    box(7,0,25,29,5,32,COLOR(183,126,91));
    box(-29,7,25,-7,7.25f,32,COLOR(41,62,72));
    box(7,5,25,29,5.25f,32,COLOR(64,78,85));
    box(-20,7,27,-19,12,28,COLOR(101,115,119));
    box(-30,0,-18,-25,4,15,COLOR(62,91,104));
    box(25,0,-18,30,3,15,COLOR(69,130,120));
    for (x=-26;x<-8;x+=4)
        box((float)x,3.2f,24.92f,(float)x+2.2f,5.2f,25,COLOR(166,204,202));
    for (x=10;x<27;x+=4)
        box((float)x,2.0f,24.92f,(float)x+2.2f,3.8f,25,COLOR(229,194,151));
    for (z=-16;z<14;z+=3) {
        box(24.9f,0.1f,(float)z,25.1f,2.9f,(float)z+0.1f,COLOR(45,97,92));
        box(-25.1f,0.1f,(float)z,-24.9f,3.9f,(float)z+0.1f,COLOR(46,68,83));
    }
}

static void spots(const SkateSession *game)
{
    int i;
    for (i=0;i<SPOT_COUNT;i++) {
        float x,y,z,r = 0.38f;
        Vec top,bottom,a,b,c,d;
        if (game->spots & (1u << i)) continue;
        session_spot(i,&x,&y,&z);
        y += sinf(animation_time*2.2f+(float)i)*0.12f;
        top = vec(x,y+0.5f,z); bottom = vec(x,y-0.5f,z);
        a=vec(x-r,y,z); b=vec(x,y,z+r); c=vec(x+r,y,z); d=vec(x,y,z-r);
        triangle(top,a,b,COLOR(255,221,103)); triangle(top,b,c,COLOR(244,178,71));
        triangle(top,c,d,COLOR(255,221,103)); triangle(top,d,a,COLOR(244,178,71));
        triangle(bottom,a,b,COLOR(186,123,51)); triangle(bottom,b,c,COLOR(210,148,55));
        triangle(bottom,c,d,COLOR(186,123,51)); triangle(bottom,d,a,COLOR(210,148,55));
    }
}

static void skater(const Skater *s)
{
    float hips = 0.7f - 0.25f * s->crouch;
    Vec position = vec(s->x, s->y + 0.08f, s->z);
    unsigned int shirt = s->stun > 0.0f ? COLOR(245,64,64) : COLOR(241,137,61);
    /* A simple ground shadow gives the ollie a visible reference. */
    float sy = park_ground(s->x,s->z)+0.02f;
    quad(vec(s->x-0.32f,sy,s->z-0.7f),vec(s->x+0.32f,sy,s->z-0.7f),
         vec(s->x+0.32f,sy,s->z+0.7f),vec(s->x-0.32f,sy,s->z+0.7f),COLOR(38,48,58));
    sceGumPushMatrix();
    sceGumTranslate(&position);
    sceGumRotateY(s->heading+s->spin);
    sceGumPushMatrix();
    sceGumRotateZ(s->flip_angle);
    box(-0.22f,0,-0.58f,0.22f,0.065f,0.58f,COLOR(75,200,162));
    box(-0.26f,-0.055f,-0.4f,0.26f,0.015f,-0.29f,COLOR(29,35,43));
    box(-0.26f,-0.055f,0.29f,0.26f,0.015f,0.4f,COLOR(29,35,43));
    sceGumPopMatrix();
    box(-0.17f,0.08f,-0.34f,-0.02f,hips,-0.13f,COLOR(42,50,73));
    box(0.02f,0.08f,0.12f,0.17f,hips,0.34f,COLOR(42,50,73));
    box(-0.2f,hips,-0.2f,0.2f,hips+0.47f,0.2f,shirt);
    box(-0.14f,hips+0.48f,-0.14f,0.14f,hips+0.75f,0.14f,COLOR(232,181,137));
    box(-0.16f,hips+0.7f,-0.16f,0.16f,hips+0.8f,0.16f,COLOR(36,51,73));
    box(-0.32f,hips+0.05f,-0.13f,-0.2f,hips+0.42f,0.05f,COLOR(232,181,137));
    box(0.2f,hips+0.05f,-0.05f,0.32f,hips+0.42f,0.13f,COLOR(232,181,137));
    sceGumPopMatrix();
}

static void rectangle(float x0, float y0, float x1, float y1, unsigned int color)
{
    Vertex *v = sceGuGetMemory(2*sizeof(Vertex));
    v[0] = vertex(vec(x0,y0,0),color);
    v[1] = vertex(vec(x1,y1,0),color);
    sceGuDrawArray(GU_SPRITES,GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,2,0,v);
}

static void panel(float y0, float y1)
{
    rectangle(0,y0,WIDTH,y1,COLOR(18,32,42));
}

/* A compact original bitmap alphabet for the game logo and award headings. */
static void heading(const char *text, float x, float y, int scale, unsigned int color)
{
    static const unsigned char letters[26][7] = {
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
    };
    while (*text) {
        int row,column;
        char letter = *text++;
        if (letter >= 'A' && letter <= 'Z')
            for (row=0;row<7;row++) for (column=0;column<5;column++)
                if (letters[letter-'A'][row] & (1u << (4-column)))
                    rectangle(x+column*scale,y+row*scale,x+(column+1)*scale,y+(row+1)*scale,color);
        x += 6*scale;
    }
}

static void text_at(int x, int y, unsigned int color, const char *text)
{
    pspDebugScreenSetTextColor(color);
    pspDebugScreenSetXY(x,y);
    pspDebugScreenPrintf("%s",text);
}

static void overlay(const SkateSession *g, int menu_selection)
{
    unsigned int ink = COLOR(241,244,234), mint = COLOR(98,216,181), gold = COLOR(255,204,100);
    if (g->state == GAME_MENU) {
        rectangle(15,46,303,229,COLOR(18,32,42));
        rectangle(15,46,19,229,mint);
        heading("SIDEWALK",29,57,4,ink);
        heading("SESSIONS",29,93,4,mint);
        rectangle(25,menu_selection ? 157 : 133,287,menu_selection ? 173 : 149,COLOR(48,83,88));
    } else if (g->state == GAME_RESULTS) {
        rectangle(43,57,437,232,COLOR(18,32,42));
        rectangle(43,57,437,61,mint);
        heading(g->mode == MODE_FREE ? "FREE SKATE" : session_rank_name(session_rank(g)),62,89,3,gold);
    } else if (g->paused) {
        rectangle(39,43,441,231,COLOR(18,32,42));
        rectangle(39,43,441,47,mint);
        heading("PARK CHALLENGES",57,56,2,ink);
    }
}

static void interface(const SkateSession *g, int menu_selection)
{
    const Skater *s = &g->skater;
    unsigned int ink = COLOR(241,244,234), mint = COLOR(98,216,181), gold = COLOR(255,204,100);
    char line[80];
    int i;
    if (g->state == GAME_MENU) {
        text_at(2,1,mint,"FOUNDRY YARD / ARCADE SKATING");
        text_at(4,17,menu_selection == 0 ? gold : ink,"2 MINUTE SCORE RUN");
        text_at(4,20,menu_selection == 1 ? gold : ink,"FREE SKATE");
        snprintf(line,sizeof(line),"RECORD SCORE %d",g->records.best_score);
        text_at(4,23,ink,line);
        snprintf(line,sizeof(line),"BEST COMBO %d",g->records.best_combo);
        text_at(4,25,mint,line);
        text_at(2,31,ink,"UP/DOWN CHOOSE   X OR START PLAY");
        return;
    }
    if (g->state == GAME_RESULTS) {
        text_at(2,1,mint,"SIDEWALK SESSIONS / FOUNDRY YARD");
        text_at(9,9,ink,g->mode == MODE_SCORE ? "SCORE RUN COMPLETE" : "SESSION COMPLETE");
        snprintf(line,sizeof(line),"SCORE %d     BEST COMBO %d",s->total_score,s->best_combo);
        text_at(9,15,ink,line);
        snprintf(line,sizeof(line),"CHALLENGES %d/5     SPOTS %d/5",session_count(g->goals),session_count(g->spots));
        text_at(9,18,mint,line);
        snprintf(line,sizeof(line),"KICKFLIPS %d   GRIND %.1fs   BAILS %d",s->landed_flips,s->landed_grind_time,s->bails);
        text_at(9,21,ink,line);
        snprintf(line,sizeof(line),"PERSONAL RECORD %d",g->records.best_score);
        text_at(9,24,gold,line);
        if (g->save_status < 0) text_at(9,27,COLOR(255,135,107),"RECORDS COULD NOT BE SAVED");
        else text_at(9,27,mint,"PERSONAL RECORDS SAVED");
        text_at(2,31,ink,"X TRY AGAIN   O OR START MAIN MENU");
        return;
    }
    if (g->mode == MODE_SCORE) {
        int seconds = (int)ceilf(g->remaining);
        snprintf(line,sizeof(line),"TIME %02d:%02d   SCORE %d   RECORD %d",seconds/60,seconds%60,s->total_score,g->records.best_score);
    } else snprintf(line,sizeof(line),"FREE SKATE   SCORE %d   BEST COMBO %d",s->total_score,s->best_combo);
    text_at(1,0,ink,line);
    snprintf(line,sizeof(line),"GOALS %d/5  SPOTS %d/5  COMBO %d x %d",session_count(g->goals),session_count(g->spots),s->combo_points,s->combo_tricks);
    text_at(1,1,mint,line);
    if (g->state == GAME_FINISHING) text_at(1,3,gold,"TIME UP - LAND YOUR CURRENT COMBO");
    else if (s->notice_time > 0.0f) text_at(1,3,gold,s->notice);
    if (g->paused) {
        for (i=0;i<GOAL_COUNT;i++) {
            char progress[24];
            int done = !!(g->goals & (1u << i));
            snprintf(line,sizeof(line),"%s %s",done ? "+" : "-",session_goal_name(i));
            text_at(7,10+i*3,done ? mint : ink,line);
            session_goal_progress(g,i,progress,sizeof(progress));
            text_at(43,11+i*3,gold,progress);
        }
        text_at(2,31,ink,"START RESUME   TRIANGLE END RUN   SELECT RESPAWN");
    } else {
        text_at(1,30,ink,"X PUSH/POP  [] FLIP  O GRAB  /\\ GRIND  L/R SPIN");
        text_at(1,32,ink,"STICK STEER  DOWN BRAKE  START GOALS  SELECT RESET");
    }
}

void render_init(void)
{
    pspDebugScreenInit();
    sceGuInit();
    sceGuStart(GU_DIRECT,command_list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,STRIDE);
    sceGuDispBuffer(WIDTH,HEIGHT,(void*)FRAME_BYTES,STRIDE);
    sceGuDepthBuffer((void*)(2*FRAME_BYTES),STRIDE);
    sceGuOffset(2048-WIDTH/2,2048-HEIGHT/2);
    sceGuViewport(2048,2048,WIDTH,HEIGHT);
    sceGuDepthRange(65535,0);
    sceGuScissor(0,0,WIDTH,HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuShadeModel(GU_SMOOTH);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    draw_buffer = 0;
}

void render_reset_camera(void)
{
    camera_ready = 0;
}

void render_frame(const SkateSession *game, int menu_selection, float dt)
{
    const Skater *s = &game->skater;
    float alpha = fminf(1.0f,dt*5.0f);
    Vec target;
    Vec up = vec(0,1,0), desired;
    animation_time += dt;
    if (!camera_ready) {
        camera_heading = s->heading;
        camera = vec(s->x-sinf(s->heading)*6.0f,s->y+3.8f,s->z-cosf(s->heading)*6.0f);
        camera_ready = 1;
    }
    camera_heading += skate_wrap_angle(s->heading-camera_heading)*alpha;
    desired = vec(s->x-sinf(camera_heading)*6.0f,s->y+3.8f,s->z-cosf(camera_heading)*6.0f);
    target = vec(s->x+sinf(camera_heading),s->y+0.9f,s->z+cosf(camera_heading));
    camera.x += (desired.x-camera.x)*alpha;
    camera.y += (desired.y-camera.y)*alpha;
    camera.z += (desired.z-camera.z)*alpha;

    sceGuStart(GU_DIRECT,command_list);
    sceGuClearColor(COLOR(160,195,202));
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(62.0f,(float)WIDTH/(float)HEIGHT,0.25f,100.0f);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&camera,&target,&up);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    park();
    spots(game);
    skater(s);
    sceGuDisable(GU_DEPTH_TEST);
    panel(0,35);
    panel(238,272);
    overlay(game,menu_selection);
    sceGuFinish();
    sceGuSync(0,0);

    pspDebugScreenSetOffset((int)(uintptr_t)draw_buffer);
    pspDebugScreenEnableBackColor(0);
    interface(game,menu_selection);
    sceDisplayWaitVblankStart();
    draw_buffer = sceGuSwapBuffers();
}

void render_shutdown(void)
{
    sceGuTerm();
}
