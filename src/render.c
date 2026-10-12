#include "render.h"
#include <math.h>
#include <stdint.h>
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
            unsigned int c = ((x/4 + z/4) & 1) ? COLOR(73,91,105) : COLOR(80,99,113);
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

static void panel(float y0, float y1)
{
    Vertex *v = sceGuGetMemory(2*sizeof(Vertex));
    v[0] = vertex(vec(0,y0,0),COLOR(20,29,42));
    v[1] = vertex(vec(WIDTH,y1,0),COLOR(20,29,42));
    sceGuDrawArray(GU_SPRITES,GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D,2,0,v);
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

void render_frame(const Skater *s, int paused, float dt)
{
    float alpha = fminf(1.0f,dt*5.0f);
    Vec target = vec(s->x,s->y+0.85f,s->z+0.8f);
    Vec up = vec(0,1,0), desired;
    if (!camera_ready) {
        camera_heading = s->heading;
        camera = vec(s->x,4.5f,s->z-7.0f);
        camera_ready = 1;
    }
    camera_heading += skate_wrap_angle(s->heading-camera_heading)*alpha;
    desired = vec(s->x-sinf(camera_heading)*7.0f,s->y+4.5f,s->z-cosf(camera_heading)*7.0f);
    camera.x += (desired.x-camera.x)*alpha;
    camera.y += (desired.y-camera.y)*alpha;
    camera.z += (desired.z-camera.z)*alpha;

    sceGuStart(GU_DIRECT,command_list);
    sceGuClearColor(COLOR(114,160,190));
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
    skater(s);
    sceGuDisable(GU_DEPTH_TEST);
    panel(0,35);
    panel(238,272);
    if (paused) panel(112,155);
    sceGuFinish();
    sceGuSync(0,0);

    pspDebugScreenSetOffset((int)(uintptr_t)draw_buffer);
    pspDebugScreenEnableBackColor(0);
    pspDebugScreenSetTextColor(COLOR(244,247,251));
    pspDebugScreenSetXY(1,0);
    pspDebugScreenPrintf("SKATE LAB   SCORE %d   BEST %d",s->total_score,s->best_combo);
    pspDebugScreenSetXY(1,1);
    pspDebugScreenPrintf("SPEED %2.0f   COMBO %d x %d   LAST %d",s->speed,s->combo_points,s->combo_tricks,s->last_score);
    pspDebugScreenSetTextColor(COLOR(255,204,100));
    pspDebugScreenSetXY(1,3);
    if (s->notice_time > 0.0f) pspDebugScreenPrintf("%s",s->notice);
    pspDebugScreenSetTextColor(COLOR(207,221,232));
    pspDebugScreenSetXY(1,30);
    pspDebugScreenPrintf("X PUSH/RELEASE OLLIE  [] FLIP  O GRAB  /\\ GRIND");
    pspDebugScreenSetXY(1,32);
    pspDebugScreenPrintf("STICK STEER  DOWN BRAKE  L/R SPIN  START PAUSE");
    if (paused) {
        pspDebugScreenSetTextColor(COLOR(255,204,100));
        pspDebugScreenSetXY(24,15);
        pspDebugScreenPrintf("PAUSED");
        pspDebugScreenSetXY(13,17);
        pspDebugScreenPrintf("START RESUME / SELECT RESPAWN");
    }
    sceDisplayWaitVblankStart();
    draw_buffer = sceGuSwapBuffers();
}

void render_shutdown(void)
{
    sceGuTerm();
}
