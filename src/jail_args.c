#include <stdlib.h>
#include <argp.h>

#include "jail.h"

const char *argp_program_version = "jail 1.0.0";

const char *argp_program_bug_address = "<brookeyang@tencent.com>";

static char doc[] = "Jail, a pretty sandbox to run program.";

static char args_doc[] = "[<program> [<argument>...]]";

static struct argp_option options[] = {
  { "name", 'n', "STRING", 0, "Jail name", 0 },
  { "base", 'b', "STRING", 0, "Mount base dir, default to '.'", 0 },
  { "root", 'r', "STRING", 0, "Rootfs, default to '/'", 0 },
  { "detach", 'd', 0, 0, "Detach process as deaemon", 0 },
  { "writable", 'w', 0, 0, "Make rootfs writable mount", 0 },
  { "verbose", 'v', 0, 0, "Make the operation more talkative", 0 },
  { "hint", -1, "STRING", OPTION_HIDDEN, "", 0 },
  { "ip", 200, "ADDRESS", 0, "Assign ip address, within 172.17.0.1/16", 0 },
  { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    jail_args_t *arguments = state->input;

    switch (key) {
        case 'n':
            arguments->name = arg;
            break;
        case 'b':
            arguments->base = arg;
            break;
        case 'r':
            arguments->root = arg;
            break;
        case 'd':
            arguments->detach = 1;
            break;
        case 'w':
            arguments->writable = 1;
            break;
        case 'v':
            arguments->verbose += 1;
            break;
        case 200:
            arguments->ip_address = arg;
            break;
        case -1:
            arguments->hint = arg;
            break;
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;
        case ARGP_KEY_ARG:
            arguments->program = arg;
            arguments->args = &state->argv[state->next];
            state->next = state->argc;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1) argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

static int jail_args_init(jail_args_t *args) {
    args->name = "demo-1"; // length should be no more than 10
    args->verbose = 0;
    args->hint = NULL;
    args->base = ".";
    args->root = "/";
    args->writable = 0;
    args->ip_address = "172.16.0.2";

    args->program = NULL;
    args->args = NULL;

    return 0;
}

jail_args_t jail_args_parse(int argc, char **argv) {
    jail_args_t arguments;
    jail_args_init(&arguments);
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    return arguments;
}

void jail_args_dump(jail_args_t *ja) {
    if (LOG_VERBOSE < LOG_LEVEL_DEBUG) return;
    // TODO snprintf with log_debug
    fprintf(stderr, "[ARGS DUMP BEGIN]\n");
    fprintf(stderr, "verbose: %d\n", ja->verbose);
    fprintf(stderr, "program: %s ", ja->program);
    for (int i = 0; ja->args[i]; i++) {
        fprintf(stderr, "%s ", ja->args[i]);
    }
    fprintf(stderr, "\n");
    if (ja->hint) fprintf(stderr, "hint: %s\n", ja->hint);
    fprintf(stderr, "name: %s\n", ja->name);
    fprintf(stderr, "base: %s\n", ja->base);
    fprintf(stderr, "root: %s\n", ja->root);
    fprintf(stderr, "detach: %d\n", ja->detach);
    fprintf(stderr, "writable: %d\n", ja->writable);
    fprintf(stderr, "ip_address: %s\n", ja->ip_address);
    fprintf(stderr, "[ARGS DUMP END]\n");
}
