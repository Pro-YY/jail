#ifndef _JAIL_H_INCLUDED_
#define _JAIL_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


/*
 * logging utility
 */
int LOG_VERBOSE;

#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_DEBUG 2


#ifdef DEBUG

#define log_errno(...)                                              \
do {                                                                \
    fprintf(stdout, "[ERROR]%s: %d: ", __FILE__, __LINE__);         \
    fprintf(stdout, "[errno %d: %s] ", errno, strerror(errno));     \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)


#define log_error(...)                                              \
do {                                                                \
    fprintf(stdout, "[ERROR]%s: %d: ", __FILE__, __LINE__);         \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)

#else

#define log_errno(...)                                              \
do {                                                                \
    fprintf(stdout, "[ERROR] ");                                    \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)


#define log_error(...)                                              \
do {                                                                \
    fprintf(stdout, "[ERROR] ");                                    \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)

#endif


#define log_info(...)                                               \
do {                                                                \
    if (!(LOG_VERBOSE >= LOG_LEVEL_INFO)) break;                    \
    fprintf(stdout, "[INFO] ");                                     \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)


#ifdef DEBUG
#define log_debug(...)                                              \
do {                                                                \
    if (!(LOG_VERBOSE >= LOG_LEVEL_DEBUG)) break;                   \
    fprintf(stdout, "[DEBUG]%s: %d: ", __FILE__, __LINE__);         \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)
#else
#define log_debug(...)
#endif


#define MAX_USER_DEFINED_ENV    16

/*
 * jail args parse from command-line
 */
typedef struct {
    int     verbose;
    char   *name;       // jail name
    char   *hint;
    char   *program;    // executable pragram to be run
    char  **args;       // and its args
    char   *envp[MAX_USER_DEFINED_ENV];  // and its envs
    char   *base;
    char   *root;
    int     detach;
    int     timeout;
    int     writable;
    char   *ip_address;
} jail_args_t;


jail_args_t jail_args_parse(int argc, char **argv);
void jail_args_dump(jail_args_t *args);


/*
 * runtime conf
 */
typedef struct {
    char   *program;
    char  **args;
    char  **envp;
    char   *name;
    pid_t   pid;
    int     efd;
    char   *mount_dir;
    char   *root;
    int     detach;
    int     timeout;
    int     writable;
    char   *ip_address;
} jail_conf_t;


extern jail_conf_t *jail_config; // global config


jail_conf_t *jail_conf_init(jail_args_t *args);
void jail_conf_free(jail_conf_t *conf);
void jail_conf_dump(jail_conf_t *conf);


int check_root();
int daemonize();
int spawn_jail(jail_conf_t *conf);
int clean_jail(jail_conf_t *conf);

int jail_loop();

int jail_filter_syscalls(jail_conf_t *conf);

int jail_drop_capabilities(jail_conf_t *conf);

int jail_setup_cgroups(jail_conf_t *conf);
int jail_clean_cgroups(jail_conf_t *conf);

int jail_setup_rlimits(jail_conf_t *conf);

#endif /* _JAIL_H_INCLUDED_ */
