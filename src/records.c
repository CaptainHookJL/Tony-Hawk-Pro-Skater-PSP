#include "session.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

static int valid(const PlayerRecords *r)
{
    return r->best_score >= 0 && r->best_score <= 99999999 &&
        r->best_combo >= 0 && r->best_combo <= 99999999 &&
        r->best_goals >= 0 && r->best_goals <= GOAL_COUNT &&
        r->completed_runs >= 0 && r->completed_runs <= 999999;
}

static int load_one(const char *path, PlayerRecords *records)
{
    FILE *file;
    char line[256], *cursor, *end;
    int values[4], i, character;
    PlayerRecords loaded = {0};
    memset(records,0,sizeof(*records));
    file = fopen(path,"r");
    if (!file) return 0;
    if (!fgets(line,sizeof(line),file)) { fclose(file); return 0; }
    line[strcspn(line,"\r\n")] = '\0';
    if (strcmp(line,"SIDEWALK1") != 0 || !fgets(line,sizeof(line),file)) {
        fclose(file); return 0;
    }
    cursor = line;
    for (i=0;i<4;i++) {
        long value;
        errno = 0;
        value = strtol(cursor,&end,10);
        if (end == cursor || errno == ERANGE || value < 0 || value > 99999999) {
            fclose(file); return 0;
        }
        values[i] = (int)value;
        cursor = end;
    }
    while (isspace((unsigned char)*cursor)) cursor++;
    if (*cursor) { fclose(file); return 0; }
    while ((character = fgetc(file)) != EOF)
        if (!isspace((unsigned char)character)) { fclose(file); return 0; }
    fclose(file);
    loaded.best_score = values[0]; loaded.best_combo = values[1];
    loaded.best_goals = values[2]; loaded.completed_runs = values[3];
    if (!valid(&loaded)) return 0;
    *records = loaded;
    return 1;
}

int records_load(const char *path, PlayerRecords *records)
{
    char backup[512];
    int size;
    if (load_one(path,records)) return 1;
    size = snprintf(backup,sizeof(backup),"%s.bak",path);
    if (size < 0 || (unsigned int)size >= sizeof(backup)) return 0;
    return load_one(backup,records);
}

int records_save(const char *path, const PlayerRecords *records)
{
    FILE *file;
    char temporary[512], backup[512];
    int result, closed, moved = 0;
    PlayerRecords previous;
    if (!valid(records)) return 0;
    result = snprintf(temporary,sizeof(temporary),"%s.tmp",path);
    if (result < 0 || (unsigned int)result >= sizeof(temporary)) return 0;
    result = snprintf(backup,sizeof(backup),"%s.bak",path);
    if (result < 0 || (unsigned int)result >= sizeof(backup)) return 0;
    file = fopen(temporary,"w");
    if (!file) return 0;
    result = fprintf(file,"SIDEWALK1\n%d %d %d %d\n",records->best_score,
        records->best_combo,records->best_goals,records->completed_runs);
    closed = fclose(file);
    if (result < 0 || closed != 0) {
        remove(temporary);
        return 0;
    }
    /* PSP rename does not replace a destination. Keep the previous good save
       until the completed temporary file has been installed. */
    file = fopen(path,"r");
    if (file) {
        fclose(file);
        if (load_one(path,&previous) || !load_one(backup,&previous)) {
            if ((remove(backup) != 0 && errno != ENOENT) || rename(path,backup) != 0) {
                remove(temporary); return 0;
            }
            moved = 1;
        } else if (remove(path) != 0) {
            remove(temporary); return 0;
        }
    }
    if (rename(temporary,path) != 0) {
        if (moved) rename(backup,path);
        remove(temporary);
        return 0;
    }
    return 1;
}
