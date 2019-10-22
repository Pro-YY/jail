#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "jail.h"


#define PROGRAM_PATH_SIZE   1024
#define MOUNT_DIR_SIZE      1024
#define MAX_NAME_SIZE       10  // limited by network device


jail_conf_t *jail_config; // global config


static char name[MAX_NAME_SIZE];


static char *random_name() {
    time_t t;
    int i;
    char table[] =  "abcdefghijklmnopqrstuvwxyz"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "0123456789_-"; // length 64

    srand((unsigned) time(&t));
    name[0] = 'j';
    name[1] = '_';
    for (i = 2; i < 10; i++) name[i] = table[rand()&63];

    return name;
}


jail_conf_t *jail_conf_init(jail_args_t *args) {
    jail_conf_t *conf;

    conf = malloc(sizeof(jail_conf_t));
    if (!conf) {
        log_errno("malloc failed");
        return NULL;
    }
    memset(conf, 0, sizeof(jail_conf_t));

    // from args
    conf->program = malloc(PROGRAM_PATH_SIZE*sizeof(char));
    if (!conf->program) {
        log_errno("malloc failed");
        return NULL;
    }

    memset(conf->program, 0, PROGRAM_PATH_SIZE);
    if (args->program[0] != '/') {
        // handle relative path
        getcwd(conf->program, PROGRAM_PATH_SIZE);
        strncat(conf->program, "/", PROGRAM_PATH_SIZE);
    }
    strncat(conf->program, args->program, PROGRAM_PATH_SIZE);

    conf->args = args->args;

    if (args->name) {
        if (strlen(args->name) > MAX_NAME_SIZE) {
            log_error("name is too long");
            return NULL;
        }
        strncpy(name, args->name, MAX_NAME_SIZE);
        conf->name = name;
    }
    else {
        conf->name = random_name();
    }

    conf->root = args->root;
    conf->detach = args->detach;
    conf->timeout = args->timeout;
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
    if (conf->program) free(conf->program);
    if (conf->mount_dir) free(conf->mount_dir);
    if (conf->envp) free(conf->envp);
    if (conf) free(conf);
}

void jail_conf_dump(jail_conf_t *conf) {
#ifdef DEBUG
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
    fprintf(stderr, "timeout: %d\n", conf->timeout);
    fprintf(stderr, "writable: %d\n", conf->writable);
    fprintf(stderr, "ip_address: %s\n", conf->ip_address);
    fprintf(stderr, "[CONF DUMP END]\n");
#endif
}
