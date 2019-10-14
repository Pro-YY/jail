#include <stdio.h>
#include <stdlib.h>

#include "jail.h"


#define DEFAULT_MOUNT_DIR_BASE "."

#define MOUNT_DIR_SIZE 1024


jail_conf_t *jail_conf_init(jail_args_t *args) {
    jail_conf_t *conf;

    conf = malloc(sizeof(jail_conf_t));
    if (!conf) {
        log_errno("malloc failed");
        return NULL;
    }
    conf->program = args->program;
    conf->args = args->args;
    conf->name = args->name;
    conf->mount_dir = malloc(MOUNT_DIR_SIZE);
    if (!conf->mount_dir) {
        log_errno("malloc failed");
        return NULL;
    }
    snprintf(conf->mount_dir, MOUNT_DIR_SIZE, "%s/%s", DEFAULT_MOUNT_DIR_BASE, conf->name);

    return conf;
}

void jail_conf_free(jail_conf_t *conf) {
    if (conf->mount_dir) free(conf->mount_dir);
    if (conf) free(conf);
}

void jail_conf_dump(jail_conf_t *conf) {
    if (!LOG_VERBOSE) return;
    // TODO snprintf with log_debug
    fprintf(stderr, "[CONF DUMP BEGIN]\n");
    fprintf(stderr, "program: %s ", conf->program);
    for (int i = 0; conf->args[i]; i++) {
        fprintf(stderr, "%s ", conf->args[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "name: %s\n", conf->name);
    fprintf(stderr, "mount_dir: %s\n", conf->mount_dir);
    fprintf(stderr, "efd: %d\n", conf->efd);
    fprintf(stderr, "[CONF DUMP END]\n");
}
