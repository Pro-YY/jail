#include <sys/resource.h>

#include "jail.h"

int jail_setup_rlimits(jail_conf_t *conf) {
    int ret = -1;

    log_debug("setting rlimit...");
	ret = setrlimit(RLIMIT_AS,
	    & (struct rlimit) {
        .rlim_max = RLIM_INFINITY,
        .rlim_cur = RLIM_INFINITY,
	});
    if (ret < 0) {
		log_errno("error setrlimit virtual memory");
		return -1;
	}

    /*
	ret = setrlimit(RLIMIT_NOFILE,
        & (struct rlimit) {
		.rlim_max = RLIM_INFINITY,
		.rlim_cur = RLIM_INFINITY,
	});
    if (ret < 0) {
		log_errno("error setrlimit file");
		return -1;
	}
    */

	return 0;
}
