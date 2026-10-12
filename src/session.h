#ifndef SESSION_H
#define SESSION_H
#include "skate.h"

#define GAME_TITLE "Sidewalk Sessions"
#define PARK_NAME "Foundry Yard"
#define RUN_SECONDS 120.0f
#define FINISH_SECONDS 3.0f
#define SPOT_COUNT 5
#define GOAL_COUNT 5

typedef enum { GAME_MENU, GAME_RUNNING, GAME_FINISHING, GAME_RESULTS } GameState;
typedef enum { MODE_SCORE, MODE_FREE } GameMode;
typedef enum { RANK_NONE, RANK_BRONZE, RANK_SILVER, RANK_GOLD } RunRank;

typedef struct {
    int best_score, best_combo, best_goals, completed_runs;
} PlayerRecords;

typedef struct {
    Skater skater;
    PlayerRecords records;
    GameState state;
    GameMode mode;
    float remaining, finish_remaining;
    unsigned int spots, goals;
    int paused, save_status;
} SkateSession;

void session_init(SkateSession *game, const PlayerRecords *records);
void session_start(SkateSession *game, GameMode mode);
void session_menu(SkateSession *game);
void session_finish(SkateSession *game);
void session_step(SkateSession *game, const SkateInput *input, float dt);
void session_respawn(SkateSession *game);
int session_count(unsigned int bits);
RunRank session_rank(const SkateSession *game);
const char *session_rank_name(RunRank rank);
const char *session_goal_name(int goal);
void session_goal_progress(const SkateSession *game, int goal, char *text, unsigned int size);
void session_spot(int index, float *x, float *y, float *z);

int records_load(const char *path, PlayerRecords *records);
int records_save(const char *path, const PlayerRecords *records);

#endif
