#ifndef CITY_MANAGER_H
#define CITY_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

/* ─── Structure d'un rapport (taille fixe) ─── */
typedef struct {
    int    report_id;
    char   inspector[64];
    double latitude;
    double longitude;
    char   category[32];
    int    severity;
    time_t timestamp;
    char   description[256];
} __attribute__((packed)) Report;

/* ─── Prototypes ─── */

/* Utilitaires */
void mode_to_str(mode_t mode, char *buf);
int  check_access(const char *path, const char *role, int need_write);
void log_action(const char *district, const char *role,
                const char *user,    const char *action);

/* Création */
void create_district(const char *district_id);

/* Commandes */
void cmd_add(const char *district, const char *role, const char *user,
             double lat, double lon, const char *category,
             int severity, const char *description);
void cmd_list(const char *district, const char *role);
void cmd_view(const char *district, int report_id, const char *role);
void cmd_remove_report(const char *district, int report_id, const char *role, const char *user);
void cmd_update_threshold(const char *district, int value,
                          const char *role, const char *user);
void cmd_filter(const char *district, const char *role,
                int cond_count, char **conditions);

/* Filter (fonctions AI-assistées) */
int parse_condition(const char *input, char *field, char *op, char *value);
int match_condition(Report *r, const char *field, const char *op, const char *value);

#endif
