#ifndef RENDER_H
#define RENDER_H
#include "skate.h"
void render_init(void);
void render_reset_camera(void);
void render_frame(const Skater *skater, int paused, float dt);
void render_shutdown(void);
#endif
