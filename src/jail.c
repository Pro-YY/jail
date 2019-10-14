#include <stdio.h>
#include <stdlib.h>

#include "jail.h"


jail_conf_t *jail_config; // global config


int main(int argc, char *argv[]) {
    int ret = -1;

    jail_args_t args = jail_args_parse(argc, argv);
    LOG_VERBOSE = args.verbose;

    //jail_args_dump(&args);
    log_info("Hello, Jail!");

    jail_config = jail_conf_init(&args);
    if (!jail_config) {
        log_error("error config init");
        return EXIT_FAILURE;
    }

    // start jail spawning
    ret = spawn_jail(jail_config);
    if (ret < 0) {
        log_error("error spawn jail");
        goto error;
    }
    log_debug("jail spawned");
    jail_conf_dump(jail_config);

    // wait jail exit
    await_jail(jail_config);
    log_debug("jail exited");

    /* end normal flow */
    goto clean;
error:
    log_error("something failed...");

clean:
    jail_conf_free(jail_config);

    if (ret) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

