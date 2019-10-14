#ifndef _JAIL_H_INCLUDED_
#define _JAIL_H_INCLUDED_


typedef struct {
    int     verbose;
    char   *name;       // jail name
    char   *hint;
    char   *program;    // executable pragram to be run
    char  **args;       // and its args
} jail_args_t;


jail_args_t jail_args_parse(int argc, char **argv);
void jail_args_dump(jail_args_t *args);


#endif /* _JAIL_H_INCLUDED_ */
