#include <seccomp.h>

#include "jail.h"


static int syscall_blacklist[] = {
    SCMP_SYS(fchmodat),
    SCMP_SYS(fchmod),
    SCMP_SYS(chmod),
};


int jail_filter_syscalls(jail_conf_t *conf) {
    int ret = -1;
    scmp_filter_ctx ctx;

    log_debug("filtering syscalls...");
    //ctx = seccomp_init(SCMP_ACT_KILL);    // as whitelist
    ctx = seccomp_init(SCMP_ACT_ALLOW); // as blacklist
    if (!ctx) {
        log_errno("error seccomp ctx init");
        return ret;
    }

    int length = sizeof(syscall_blacklist)/sizeof(int);

    for (int i = 0; i < length; i++) {
        ret = seccomp_rule_add(ctx, SCMP_ACT_KILL, syscall_blacklist[i], 0);
        if (ret < 0) {
            log_errno("error seccomp rule add");
            goto out;
        }
    }

    ret = seccomp_load(ctx);
    if (ret < 0) {
        log_errno("error seccomp load");
        goto out;
    }
out:
    seccomp_release(ctx);
    if (ret != 0) return -1;

    return 0;
}
