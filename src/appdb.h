#ifndef __APPDB_H__
#define __APPDB_H__

#include <stdint.h>

typedef struct appinfo {
    char title_id[256];
    char real_id[256];
    char title[256];
    char eboot[256];
    char dev[256];
    struct appinfo *next;
    struct appinfo *prev;
} appinfo;

typedef struct applist {
    uint32_t count;
    appinfo *items;
} applist;

int get_applist(applist *list);
#endif
