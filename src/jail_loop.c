#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include "jail.h"

/*
 * event loop: handling signal
 */
// TODO seperate loop and signal handling
void jail_loop() {

    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;
    int ret = -1;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        log_error("sigprocmask");

    sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd == -1)
        log_error("signalfd");

    int epfd = -1;
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) log_errno("error epoll create");

    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event);
    if (ret) { log_errno("error epoll_ctl"); }

    int nfds = -1;
    int i = -1;
    int MAX_EVENTS_SIZE = 1024;
    struct epoll_event events[1024];

    for (;;) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS_SIZE, 1000);
        for (i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == sfd) {
                    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
                    if (s != sizeof(struct signalfd_siginfo))
                       log_errno("read");
                    if (fdsi.ssi_signo == SIGCHLD) {
                        log_debug("Got SIGCHLD, jail exited");
                        clean_jail(jail_config);
                        goto end;
                    }
                    else if (fdsi.ssi_signo == SIGTERM) {
                        log_debug("Got SIGTERM");
                        kill(jail_config->pid, SIGKILL);
                    }
                    else if (fdsi.ssi_signo == SIGINT) {
                        log_debug("Got SIGINT");
                        kill(jail_config->pid, SIGKILL);
                    }
                    else if (fdsi.ssi_signo == SIGQUIT) {
                        log_debug("Got SIGQUIT");
                        kill(jail_config->pid, SIGKILL);
                    }
                    else {
                        log_info("Got unexpected signal: %d", fdsi.ssi_signo);
                    }
                }
            }
        }
    }

end:
    log_debug("event loop stopped");
    return;
}
