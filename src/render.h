#ifndef RENDER_H
#define RENDER_H
#include "session.h"
void render_init(void);
void render_reset_camera(void);
void render_frame(const SkateSession *game, int menu_selection, float dt);
void render_shutdown(void);
#endif
