#include <sys/capability.h>
#include <sys/prctl.h>

#include "jail.h"


static int drop_caps[] = {
    CAP_MKNOD,  // prohibits `mknod`
    CAP_NET_RAW, // prohibits `ping`
};


int jail_drop_capabilities(jail_conf_t *conf) {
    int ret = -1;
    size_t num_caps = sizeof(drop_caps) / sizeof(*drop_caps);

    log_debug("drop capabilities...%ld", num_caps);
    if (num_caps == 0) return 0;

    for (size_t i = 0; i < num_caps; i++) {
        ret = prctl(PR_CAPBSET_DROP, drop_caps[i], 0, 0, 0);
        if (ret != 0) {
            log_errno("error drop cap");
            return ret;
        }
    }

    cap_t caps = NULL;
    if (!(caps = cap_get_proc())
        || cap_set_flag(caps, CAP_INHERITABLE, num_caps, drop_caps, CAP_CLEAR)
        || cap_set_proc(caps)) {
        log_errno("clear inheritable cap error");
    }

    if (caps) cap_free(caps);

    if (ret != 0) return ret;
    return 0;
}
