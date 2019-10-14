#include <stdio.h>
#include <stdlib.h>

#include "jail.h"

jail_conf_t *config; // global config

int main(int argc, char *argv[]) {
    jail_args_t args = jail_args_parse(argc, argv);
    LOG_VERBOSE = args.verbose;

    //jail_args_dump(&args);
    log_info("Hello, Jail!");

    config = jail_conf_init(&args);
    if (!config) {
        log_error("config init error");
        return EXIT_FAILURE;
    }
    jail_conf_dump(config);


    goto clean;

/*
error:
    log_error("jail error\n");
*/

clean:
    log_debug("cleaning resources...");
    jail_conf_free(config);

    log_debug("done.");

    return EXIT_SUCCESS;
}

