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

#define log_info(...)                                               \
do {                                                                \
    fprintf(stdout, "[INFO]"__VA_ARGS__"\n");                       \
} while (0)

#define log_debug(...)                                              \
do {                                                                \
    if (!LOG_VERBOSE) break;                                        \
    fprintf(stdout, "[DEBUG]%s: %d: ", __FILE__, __LINE__);         \
    fprintf(stdout, __VA_ARGS__);                                   \
    fprintf(stdout, "\n");                                          \
} while (0)


/*
 * jail args parse from command-line
 */
typedef struct {
    int     verbose;
    char   *name;       // jail name
    char   *hint;
    char   *program;    // executable pragram to be run
    char  **args;       // and its args
} jail_args_t;


jail_args_t jail_args_parse(int argc, char **argv);
void jail_args_dump(jail_args_t *args);


/*
 * runtime conf
 */
typedef struct {
    char   *program;
    char  **args;
    char   *name;
    int     efd;
    char   *mount_dir;
} jail_conf_t;


jail_conf_t *jail_conf_init(jail_args_t *args);
void jail_conf_free(jail_conf_t *conf);
void jail_conf_dump(jail_conf_t *conf);

#endif /* _JAIL_H_INCLUDED_ */
