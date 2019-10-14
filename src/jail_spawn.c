#define _GNU_SOURCE
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/eventfd.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#include "jail.h"

/*
 * jail process spawned by main process
 */
static int await_setup_event(int efd)
{
    uint64_t u;
    ssize_t ret;
    ret = read(efd, &u, sizeof(uint64_t));
    if (ret != sizeof(uint64_t)) {
        log_error("jail process await setup event");
        return -1;
    }
    return 0;
}

/*
 * mount rootfs
 */
#define PIVOT_ROOT_DIR "/pivot_root"

int mount_rootfs(jail_conf_t *conf) {
    int ret = -1;

    log_debug("remounting everything as slave...");
    ret = mount(NULL, "/", NULL, MS_SLAVE | MS_REC, NULL);
    if (ret) { log_errno("error remount as slave"); return -1; }

    log_debug("make dir: %s", conf->mount_dir);
    ret = mkdir(conf->mount_dir, 0);
    if (ret) { log_errno("error mkdir"); return -1; }

    log_debug("bind mount...");
    char *rootfs = conf->root;
    ret = mount(rootfs, conf->mount_dir, NULL, MS_BIND | MS_REC, NULL);
    if (ret) { log_errno("error bind mount error"); return -1; }

    char inner_mount_dir[256] = {0};
    snprintf(inner_mount_dir, 256-10, "%s", conf->mount_dir);
    strncat(inner_mount_dir, PIVOT_ROOT_DIR, 256);
    log_debug("make inner mount dir: %s ...", inner_mount_dir);
    ret = mkdir(inner_mount_dir, 0);
    if (ret) { log_error("error make inner mount dir"); return -1; }

    log_debug("pivoting root...");
    ret = syscall(SYS_pivot_root, conf->mount_dir, inner_mount_dir);
    if (ret) { log_error("error pivot root"); return -1; }

    log_debug("chdir to new root...");
    ret = chdir("/");
    if (ret) { log_error("error chdir to new root"); return -1; }

    log_debug("mounting proc...");
    ret = mount("proc", "/proc", "proc", MS_RDONLY, NULL);
    if (ret) { log_error("error mount proc"); return -1; }

    log_debug("umount old root...");
    ret = umount2(PIVOT_ROOT_DIR, MNT_DETACH);
    if (ret) { log_error("error unmount old root"); return -1; }

    log_debug("remove old root...");
    ret = rmdir(PIVOT_ROOT_DIR);
    if (ret) { log_error("error remove old root dir"); return -1; }

    if (conf->writable == 0) {
        log_debug("ro-remount root...");
        ret = mount("", "/", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY, NULL);
        if (ret) { log_error("error remount as readonly"); return -1; }
    }

    log_debug("mount tmpfs..."); // read-write tmpfs
    ret = mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    if (ret) { log_error("error mount tmpfs"); return -1; }

    return 0;
}


#define MAX_ARGV 1024
#define HOSTNAME_PREFIX "jail-"
#define MAX_HOSTNAME 256


static int jail_process(void *args) {
    jail_conf_t *conf = args;
    char *jail_argv[MAX_ARGV] = {0};
    int ret = -1;

    log_debug("in jail process, pid: %d", getpid());

    // prepare argv
    jail_argv[0] = conf->program;
    for (int i = 1; i < MAX_ARGV && conf->args[i-1]; i++) {
        jail_argv[i] = conf->args[i-1];
    }

    // set hostname (uname -n)
    char hostname[MAX_HOSTNAME] = HOSTNAME_PREFIX;
    strncat(hostname, conf->name, MAX_HOSTNAME);
    ret = sethostname(hostname, strlen(hostname));
    if (ret != 0) {
        log_error("error sethostname");
        return ret;
    }

    log_debug("waiting parent setting up...");
    await_setup_event(conf->efd);

    // mount rootfs
    ret = mount_rootfs(conf);
    if (ret != 0) {
        log_error("error mount rootfs");
        return ret;
    }

    log_debug("-------- Jail Start --------");
    ret = execve(jail_argv[0], jail_argv, NULL);
    if (ret != 0) {
        log_error("exec error!");
        return -1;
    }
    // can never reach here because of exec
    return 0;
}

/*
 * main process
 */
static int notify_setup_event(int efd) {
    ssize_t ret = write(efd, &(uint64_t){1}, sizeof(uint64_t));
    if (ret != sizeof(uint64_t)) {
        log_error("error notify setup done event");
        return -1;
    }
    else return 0;
}


char *jail_stack;

const int stack_size = 1<<16;


int spawn_jail(jail_conf_t *conf) {
    int ret = -1;
    int flags = 0;

    conf->efd = eventfd(0, 0);
    if (conf->efd < 0) {
        log_error("create eventfd error!");
        return -1;
    }

    flags = SIGCHLD | CLONE_NEWNS |
        CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | CLONE_NEWNET |
        CLONE_NEWCGROUP;

    char *jail_stack = NULL;

    int stack_size = 1<<16;
    jail_stack = malloc(stack_size);
    if (!jail_stack) {
        log_error("errro malloc stack");
        return -1;
    }
    ret = clone(jail_process, jail_stack + stack_size, flags, (void *)conf);
    if (ret < 0) {
        log_errno("error clone");
        return -1;
    }
    conf->pid = ret;

    // setup something here, like network/uidmap

    ret = notify_setup_event(conf->efd);
    if (ret < 0) {
        log_error("error notify, killing jail process");
        kill(conf->pid, SIGKILL);
    }

    return 0;
}


int await_jail(jail_conf_t *conf) {
    int ret = 0;
    int wstatus = -1;

    ret = waitpid(conf->pid, &wstatus, 0);
    log_debug("-------- Jail Exit ---------");
    if (ret < 0) {
        log_error("error wait jail");
        goto clean;
    }
    if (WIFEXITED(wstatus)) {
        log_debug("jail process %d exited with status: %d", ret, wstatus);
    }
    else if (WIFSIGNALED(wstatus)) {
        log_debug("jail process %d killed by signal %d %s", ret,
                WTERMSIG(wstatus), strsignal(WTERMSIG(wstatus)));
    }
    else {
        log_debug("what happend with jail? %d, with status: %d", ret, wstatus);
    }

clean:
    log_debug("clean jail process resource...");
    close(conf->efd);
    if (jail_stack) free(jail_stack);

    log_debug("remove mount dir: %s ...", conf->mount_dir);
    rmdir(conf->mount_dir);
    if (ret) log_error("rmdir failed: %s", conf->mount_dir);
    log_debug("done.");

    return ret;
}
