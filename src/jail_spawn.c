#define _GNU_SOURCE
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
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

    log_debug("remount everything as slave...");
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

    log_debug("pivot root...");
    ret = syscall(SYS_pivot_root, conf->mount_dir, inner_mount_dir);
    if (ret) { log_error("error pivot root"); return -1; }

    log_debug("chdir to new root...");
    ret = chdir("/");
    if (ret) { log_error("error chdir to new root"); return -1; }

    log_debug("mount proc...");
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


/*
 * networks
 */
static int setup_network(jail_conf_t *conf) {
    /*
     * host should enable ip forward and set nat rules first, i.e.
     * 1. sysctl -w net.ipv4.ip_forward=1
     * 2. iptables -t nat -A POSTROUTING -s 172.17.0.0/16 -j MASQUERADE
     */
    int ret = -1;
    char cmd[256] =  {0};

    const char bridge[] = "jail0";

    /*
     * prepare host
     */
    sprintf(cmd, "ip link | grep %s 1>/dev/null 2>&1", bridge);
    ret = system(cmd);
    if (!ret) { goto create_veth; } // bridge already exists
    else { log_debug("bridge not exists yet, creating"); }

    sprintf(cmd, "ip link add name %s type bridge", bridge);
    ret = system(cmd);
    if (ret) { log_error("error add bridge"); goto error; }

    sprintf(cmd, "ip a add 172.17.255.254/16 brd + dev %s", bridge);
    ret = system(cmd);
    if (ret) { log_error("error set bridge address"); goto error; }

    sprintf(cmd, "ip link set %s up", bridge);
    ret = system(cmd);
    if (ret) { log_error("error set bridge up"); goto error; }

create_veth:
    sprintf(cmd, "ip link add veth-%s type veth peer name eth0 netns %d", conf->name, conf->pid);
    ret = system(cmd);
    if (ret) { log_error("error add link"); return -1; }

    sprintf(cmd, "ip link set veth-%s up", conf->name);
    ret = system(cmd);
    if (ret) { log_error("error set link up"); return -1; }

    sprintf(cmd, "ip link set veth-%s master %s", conf->name, bridge);
    ret = system(cmd);
    if (ret) { log_error("error set bridge master"); return -1; }

    return 0;

error:
    log_error("delete %s bridge", bridge);
    sprintf(cmd, "ip link del %s", bridge);
    ret = system(cmd);
    if (ret) { log_error(" error delete bridge"); }

    return -1;
}


static int init_jail_network(jail_conf_t *conf) {
    int ret = -1;
    char cmd[256] =  {0};
    char address[256] = {0};
    char gateway[] = "172.17.255.254";

    snprintf(address, 256, "%s", conf->ip_address);
    strncat(address, "/16", 256);

    ret = system("ip link set lo up");
    if (ret) { log_error("error set lo up"); return -1; }

    ret = system("ip link set eth0 up");
    if (ret) { log_error("error set eth0 up"); return -1; }

    sprintf(cmd, "ip address add %s dev eth0", address);
    ret = system(cmd);
    if (ret) { log_error("error add address"); return -1; }

    sprintf(cmd, "ip route add default via %s", gateway);
    ret = system(cmd);
    if (ret) { log_error("error add address"); return -1; }

    return 0;
}


/*
 * jail process as child routine
 */
#define MAX_ARGV 1024
#define HOSTNAME_PREFIX ""
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
        log_errno("error sethostname");
        return ret;
    }

    log_debug("waiting parent setting up...");
    await_setup_event(conf->efd);
    log_debug("parent setup complete.");

    // init network
    ret = init_jail_network(conf);
    if (ret != 0) {
        log_error("error init jail network");
        return ret;
    }

    // mount rootfs
    ret = mount_rootfs(conf);
    if (ret != 0) {
        log_error("error mount rootfs");
        return ret;
    }

    // filter syscalls
    ret = jail_filter_syscalls(conf);
    if (ret != 0) {
        log_error("error filter syscalls");
        return ret;
    }

    // drop root capabilities
    ret = jail_drop_capabilities(conf);
    if (ret != 0) {
        log_error("error drop capabilites");
        return ret;
    }

    log_debug("-------- Jail Start --------");
    ret = execve(jail_argv[0], jail_argv, conf->envp);
    if (ret != 0) {
        log_errno("exec error!");
        return -1;
    }
    // can never reach here because of exec
    return 0;
}

/*
 * main process
 */
int daemonize() {
    int ret = -1;
    int fd;
    int maxfd = -1;

    maxfd = sysconf(_SC_OPEN_MAX);

    ret = fork();
    if (ret) _exit(EXIT_SUCCESS);   // parent exit, child proceed
    ret = setsid();     // leader of new session, 0 for success
    if (ret < 0) { log_errno("error setsid"); return -1; }
    ret = fork();
    if (ret) _exit(EXIT_SUCCESS);

    for (fd = 0; fd < maxfd; fd++) close(fd);
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) { log_errno("error open /dev/null as stdin"); return -1; }

    fd = dup2(STDIN_FILENO, STDOUT_FILENO);
    if (fd < 0) { log_errno("error dup stdout"); return -1; }

    fd = dup2(STDIN_FILENO, STDERR_FILENO);
    if (fd < 0) { log_errno("error dup stderr"); return -1; }

    return ret;
}


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

    // setup resource limit, cgroups, rlimit...
    ret = jail_setup_cgroups(conf);
    if (ret < 0) {
        log_error("error setup cgroups");
        return -1;
    }

    ret = jail_setup_rlimits(conf);
    if (ret < 0) {
        log_error("error setup rlimits");
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

    // setup something here, like network/uidmap?
    ret = setup_network(conf);
    if (ret < 0) {
        log_error("error setup network, killing jail process");
        kill(conf->pid, SIGKILL);
        return -1;
    }

    ret = notify_setup_event(conf->efd);
    if (ret < 0) {
        log_error("error notify, killing jail process");
        kill(conf->pid, SIGKILL);
        return -1;
    }

    return 0;
}


/*
 *  no need to wait child jail process, just handle signal SIGCHLD
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
    ret = rmdir(conf->mount_dir);
    if (ret) log_errno("rmdir failed: %s", conf->mount_dir);
    log_debug("done.");

    return ret;
}
*/


int clean_jail(jail_conf_t *conf) {
    int ret = -1;

    log_debug("-------- Jail Exit ---------");

    log_debug("clean jail process resource...");

    ret = jail_clean_cgroups(conf);
    if (ret) log_error("clean cgroups failed");

    close(conf->efd);
    if (jail_stack) free(jail_stack);

    log_debug("remove mount dir: %s ...", conf->mount_dir);
    ret = rmdir(conf->mount_dir);
    if (ret) log_errno("rmdir failed: %s", conf->mount_dir);
    log_debug("cleaning completed");

    return ret;
}
