#include <stdio.h>
#include <stdlib.h>

#include "jail.h"


#define MOUNT_DIR_SIZE 1024


jail_conf_t *jail_config; // global config


jail_conf_t *jail_conf_init(jail_args_t *args) {
    jail_conf_t *conf;

    conf = malloc(sizeof(jail_conf_t));
    if (!conf) {
        log_errno("malloc failed");
        return NULL;
    }
    memset(conf, 0, sizeof(jail_conf_t));

    // from args
    conf->program = args->program;
    conf->args = args->args;
    conf->name = args->name;
    conf->root = args->root;
    conf->detach = args->detach;
    conf->writable = args->writable;
    conf->ip_address = args->ip_address;

    // internal states
    conf->pid = -1;

    conf->efd = -1;

    conf->mount_dir = malloc(MOUNT_DIR_SIZE);
    if (!conf->mount_dir) {
        log_errno("malloc failed");
        return NULL;
    }
    snprintf(conf->mount_dir, MOUNT_DIR_SIZE, "%s/%s", args->base, conf->name);

    // envp: 2 default as example
    conf->envp = malloc((MAX_USER_DEFINED_ENV+2+1) * sizeof(char *));
    if (!conf->envp) {
        log_errno("malloc failed");
        return NULL;
    }
    conf->envp[0] = "PATH=/usr/local/node/bin:/usr/local/bin:/usr/bin:/bin";
    conf->envp[1] = "NODE_PATH=/usr/local/node/lib/node_modules";
    for (int i = 0; i < MAX_USER_DEFINED_ENV && args->envp[i]; i++) {
        conf->envp[i+2] = args->envp[i];
    }
    conf->envp[MAX_USER_DEFINED_ENV+2] = NULL;

    return conf;
}

void jail_conf_free(jail_conf_t *conf) {
    log_debug("free config");
    if (conf->mount_dir) free(conf->mount_dir);
    if (conf->envp) free(conf->envp);
    if (conf) free(conf);
}

void jail_conf_dump(jail_conf_t *conf) {
    if (LOG_VERBOSE < LOG_LEVEL_DEBUG) return;
    // TODO snprintf with log_debug
    fprintf(stderr, "[CONF DUMP BEGIN]\n");
    fprintf(stderr, "program: %s ", conf->program);
    for (int i = 0; conf->args[i]; i++) {
        fprintf(stderr, "%s ", conf->args[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "env:\n");
    for (int i = 0; conf->envp[i]; i++) {
        fprintf(stderr, "%s ", conf->envp[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "name: %s\n", conf->name);
    fprintf(stderr, "pid: %d\n", conf->pid);
    fprintf(stderr, "efd: %d\n", conf->efd);
    fprintf(stderr, "mount_dir: %s\n", conf->mount_dir);
    fprintf(stderr, "root: %s\n", conf->root);
    fprintf(stderr, "detach: %d\n", conf->detach);
    fprintf(stderr, "writable: %d\n", conf->writable);
    fprintf(stderr, "ip_address: %s\n", conf->ip_address);
    fprintf(stderr, "[CONF DUMP END]\n");
}
