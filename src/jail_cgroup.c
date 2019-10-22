#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "jail.h"


#define MEMORY_LIMIT_IN_BYTES   "50000000"
/*
#define SHARES "256"
#define PIDS "64"
#define WEIGHT "10"
#define FD_COUNT 64
*/
#define CPU_CFS_PERIOD_US       "1000000"
#define CPU_CFS_QUOTA_US        "1000000"


struct cgrp_control {
    char control[256];
    struct cgrp_setting {
        char name[256];
        char value[256];
    } **settings;
};


struct cgrp_setting add_to_tasks = {
    .name = "tasks",
    .value = "0" // 0 means current writing process
};


struct cgrp_control *cgrps[] = {
    & (struct cgrp_control) {
        .control = "memory",
        .settings = (struct cgrp_setting *[]) {
            & (struct cgrp_setting) {
                .name = "memory.limit_in_bytes",
                .value = MEMORY_LIMIT_IN_BYTES
            },
            & (struct cgrp_setting) {
                .name = "memory.kmem.limit_in_bytes",
                .value = MEMORY_LIMIT_IN_BYTES
            },
            &add_to_tasks,
            NULL
        }
    },
    & (struct cgrp_control) {
        .control = "cpu",
        .settings = (struct cgrp_setting *[]) {
            & (struct cgrp_setting) {
                .name = "cpu.cfs_period_us",
                .value = CPU_CFS_PERIOD_US
            },
            & (struct cgrp_setting) {
                .name = "cpu.cfs_quota_us",
                .value = CPU_CFS_QUOTA_US
            },
            &add_to_tasks,
            NULL
        }
    },
    NULL
};


int jail_setup_cgroups(jail_conf_t *conf) {
    int ret = -1;

    log_debug("setting up cgroups...");

    for (struct cgrp_control **cgrp = cgrps; *cgrp; cgrp++) {
        char dir[256] = {0};
        ret = snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s",
                (*cgrp)->control, conf->name);
        if (ret < 0) {
            log_error("error snprintf");
            return -1;
        }

        ret = mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR);
        if (ret < 0) { log_errno("error mkdir %s", dir); return -1; }

        for (struct cgrp_setting **setting = (*cgrp)->settings; *setting; setting++) {
            char path[256] = {0};
            int fd = 0;
            ret = snprintf(path, sizeof(path), "%s/%s", dir,
                     (*setting)->name);
            if (ret < 0) {
                log_error("error snprintf");
                return -1;
            }

            fd = open(path, O_WRONLY);
            if (fd < 0) {
                log_errno("error open: %s", path);
                return -1;
            }

            ret = write(fd, (*setting)->value, strlen((*setting)->value));
            if (ret < 0) {
                log_errno("error write to %s", path);
                close(fd);
                return -1;
            }
            close(fd);
        }
    }

    return 0;
}


int jail_clean_cgroups(jail_conf_t *conf) {
    int ret = -1;

    log_debug("cleaning cgroups...");
    for (struct cgrp_control **cgrp = cgrps; *cgrp; cgrp++) {
        char dir[256] = {0};
        char task[256] = {0};
        int task_fd = 0;

        ret = snprintf(dir, sizeof(dir), "/sys/fs/cgroup/%s/%s",
                 (*cgrp)->control, conf->name);
        if (ret < 0) {
            log_error("error snprintf");
            return -1;
        }
        ret = snprintf(task, sizeof(task), "/sys/fs/cgroup/%s/tasks",
                (*cgrp)->control);
        if (ret < 0) {
            log_error("error snprintf");
            return -1;
        }

        log_debug("remove cgroup dir: %s", dir);
        task_fd = open(task, O_WRONLY);
        if (task_fd < 0) {
            log_debug("error open dir: %s", task);
            return -1;
        }

        ret = write(task_fd, "0", 2);
        if (ret < 0) {
            log_debug("error write to %s", task);
            close(task_fd);
            return -1;
        }

        close(task_fd);

        ret = rmdir(dir);
        if (ret) {
            log_errno("error rmdir: %s", dir);
            return -1;
        }
    }

    return 0;
}
