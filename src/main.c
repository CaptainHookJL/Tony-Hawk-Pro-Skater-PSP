#include "render.h"
#include "skate.h"
#include <math.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <psprtc.h>
#include <psppower.h>
#ifdef SKATE_SMOKE_TEST
#include <stdio.h>
#endif

PSP_MODULE_INFO("SkateLab",0,0,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(8*1024);

static volatile int running = 1;

static int exit_callback(int arg1, int arg2, void *common)
{
    (void)arg1; (void)arg2; (void)common;
    running = 0;
    return 0;
}

static int callback_thread(SceSize args, void *argp)
{
    int callback;
    (void)args; (void)argp;
    callback = sceKernelCreateCallback("ExitCallback",exit_callback,0);
    sceKernelRegisterExitCallback(callback);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void)
{
    int thread = sceKernelCreateThread("CallbackThread",callback_thread,0x11,0x1000,0,0);
    if (thread >= 0) sceKernelStartThread(thread,0,0);
}

static SkateInput input_from_pad(const SceCtrlData *pad)
{
    SkateInput in = {0};
    float steer = ((float)pad->Lx-128.0f)/127.0f;
    if (fabsf(steer) < 0.18f) steer = 0.0f;
    else steer = copysignf((fabsf(steer)-0.18f)/0.82f,steer);
    if (pad->Buttons & PSP_CTRL_LEFT) steer = -1.0f;
    if (pad->Buttons & PSP_CTRL_RIGHT) steer = 1.0f;
    in.steer = steer;
    in.push = !!(pad->Buttons & PSP_CTRL_CROSS);
    in.brake = !!(pad->Buttons & PSP_CTRL_DOWN);
    in.flip = !!(pad->Buttons & PSP_CTRL_SQUARE);
    in.grab = !!(pad->Buttons & PSP_CTRL_CIRCLE);
    in.grind = !!(pad->Buttons & PSP_CTRL_TRIANGLE);
    in.spin_left = !!(pad->Buttons & PSP_CTRL_LTRIGGER);
    in.spin_right = !!(pad->Buttons & PSP_CTRL_RTRIGGER);
    return in;
}

#ifdef SKATE_SMOKE_TEST
/* A separate build exercises the real PSP loop without physical input. */
static void smoke_pad(SceCtrlData *pad, unsigned int frame)
{
    pad->Lx = pad->Ly = 128;
    pad->Buttons = 0;
    if (frame < 100) pad->Buttons |= PSP_CTRL_CROSS;
    if (frame == 105) pad->Buttons |= PSP_CTRL_SQUARE;
    if (frame >= 135 && frame < 150) pad->Buttons |= PSP_CTRL_CIRCLE;
    if (frame >= 150 && frame < 215) pad->Buttons |= PSP_CTRL_TRIANGLE;
    if (frame == 250 || frame == 261) pad->Buttons |= PSP_CTRL_START;
    if (frame == 280) pad->Buttons |= PSP_CTRL_SELECT;
}
#endif

int main(void)
{
    Skater skater;
    unsigned int previous_buttons = 0;
    u64 previous_tick;
    float accumulator = 0.0f;
    int paused = 0;
    const float fixed_dt = 1.0f/120.0f;
#ifdef SKATE_SMOKE_TEST
    unsigned int smoke_frame = 0;
#endif
    setup_callbacks();
    scePowerSetClockFrequency(333,333,166);
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    skate_init(&skater);
    render_init();
    sceRtcGetCurrentTick(&previous_tick);
    while (running) {
        SceCtrlData pad;
        SkateInput input;
        u64 tick;
        float elapsed;
        unsigned int pressed;
        sceCtrlPeekBufferPositive(&pad,1);
#ifdef SKATE_SMOKE_TEST
        smoke_pad(&pad,smoke_frame);
#endif
        sceRtcGetCurrentTick(&tick);
        elapsed = (float)(tick-previous_tick)/(float)sceRtcGetTickResolution();
        previous_tick = tick;
        elapsed = fminf(0.05f,fmaxf(0.0f,elapsed));
        pressed = pad.Buttons & ~previous_buttons;
        previous_buttons = pad.Buttons;
        input = input_from_pad(&pad);
        if (pressed & PSP_CTRL_START) {
            paused = !paused;
            accumulator = 0.0f;
            skater.previous = input;
        }
        if (pressed & PSP_CTRL_SELECT) {
            skate_respawn(&skater);
            render_reset_camera();
            skater.previous = input;
        }
        if (!paused) {
            accumulator += elapsed;
            while (accumulator >= fixed_dt) {
                skate_step(&skater,&input,fixed_dt);
                accumulator -= fixed_dt;
            }
        }
        render_frame(&skater,paused,elapsed);
#ifdef SKATE_SMOKE_TEST
        smoke_frame++;
        if (smoke_frame == 300) {
            int passed = skater.total_score >= 400 && skater.best_combo >= 400 &&
                skater.z == -12.0f && !skater.airborne && !paused;
            printf("SKATE_SMOKE_%s frames=%u score=%d best=%d\n",
                passed ? "OK" : "FAIL",smoke_frame,skater.total_score,skater.best_combo);
            running = 0;
        }
#endif
    }
    render_shutdown();
    sceKernelExitGame();
    return 0;
}
