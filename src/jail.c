#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "jail.h"


static void signal_handler(int signal) {
    switch (signal) {
    case SIGCHLD:
        log_debug("got SIGCHLD, jail exited");
        clean_jail(jail_config);
        jail_conf_free(jail_config);
        exit(EXIT_SUCCESS);
    case SIGTERM:
        log_info("got SIGTERM, clean jail");
        kill(jail_config->pid, SIGKILL);
        break;
    default:
        log_info("got unknown signal: %d, clean and exit", signal);
        clean_jail(jail_config);
        jail_conf_free(jail_config);
        exit(EXIT_SUCCESS);
    }
}


int main(int argc, char *argv[]) {
    int ret = -1;

    jail_args_t args = jail_args_parse(argc, argv);
    LOG_VERBOSE = args.verbose;

    //jail_args_dump(&args);
    log_debug("Hello, Jail!");

    jail_config = jail_conf_init(&args);
    if (!jail_config) {
        log_error("error config init");
        return EXIT_FAILURE;
    }

    // start jail spawning
    ret = spawn_jail(jail_config);
    if (ret < 0) goto error;
    log_debug("jail spawned");
    jail_conf_dump(jail_config);

    signal(SIGCHLD, signal_handler);
    signal(SIGTERM, signal_handler);

    // await events
    for (;;);

error:
    log_error("something failed...");
    jail_conf_free(jail_config);

    return EXIT_FAILURE;
}

