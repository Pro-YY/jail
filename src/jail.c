#include <stdio.h>
#include <stdlib.h>

#include "jail.h"

int main(int argc, char *argv[]) {
    int ret = -1;

    jail_args_t args = jail_args_parse(argc, argv);
    LOG_VERBOSE = args.verbose;
    //jail_args_dump(&args);

    if (check_root()) {
        log_info("You should be root, quit.");
        return EXIT_FAILURE;
    }

    jail_config = jail_conf_init(&args);
    if (!jail_config) {
        log_error("error config init");
        return EXIT_FAILURE;
    }

    if (jail_config->detach) {
        ret = daemonize();
        if (ret < 0) goto error;
    }

    // start jail spawning
    ret = spawn_jail(jail_config);
    if (ret < 0) goto error;
    log_debug("jail spawned");
    jail_conf_dump(jail_config);

    // start event loop
    ret = jail_loop();
    if (ret) goto error;

    goto free;
error:
    log_error("something failed...");
free:
    jail_conf_free(jail_config);

    return EXIT_FAILURE;
}
