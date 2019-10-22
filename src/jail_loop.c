#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>

#include "jail.h"

/*
 * signal handling
 */
static int handle_signal_event(int sfd) {
    struct signalfd_siginfo fdsi;
    ssize_t s = -1;
    int ret = -1;   // 0 exit loop/process, -1 continue

    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo)) {
       log_errno("error read signalfd");
       return ret;
    }

    switch (fdsi.ssi_signo) {
    case SIGCHLD:
        log_debug("Got SIGCHLD, jail exited");
        clean_jail(jail_config);
        ret = 0;
        break;
    case SIGTERM:
        log_info("Got SIGTERM");
        kill(jail_config->pid, SIGKILL);
        break;
    case SIGINT:
        log_info("Got SIGINT");
        kill(jail_config->pid, SIGKILL);
        break;
    case SIGQUIT:
        log_info("Got SIGQUIT");
        kill(jail_config->pid, SIGKILL);
        break;
    default:
        log_error("Got unexpected signal: %d", fdsi.ssi_signo);
    }

    return ret;
}


static int register_signal_event(int epfd) {
    int ret = -1;
    int sfd = -1;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        log_errno("error sigprocmask");
        return -1;
    }

    sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd == -1) {
        log_errno("error signalfd");
        return -1;
    }

    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event);
    if (ret) {
        log_errno("error epoll_ctl");
        close(sfd);
        return -1;
    }

    return sfd;
}


/*
 * timer event handling
 */
static int handle_timer_event(int tfd) {
    log_info("Timout!");
    kill(jail_config->pid, SIGKILL);

    /*
    int ret = -1;
    uint64_t v;
    ret = read(tfd, &v, sizeof(v));
    if (ret < 0) {
        log_errno("error read timerfd");
        close(tfd);
        return -1;
    }
    */
    close(tfd);

    return 0;
}


static int register_timer_event(int epfd) {
    int ret = -1;
    int tfd = -1;
    struct itimerspec its;


    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (tfd == -1) {
        log_errno("error timerfd create");
        return -1;
    }

    if (jail_config->timeout <= 0) return tfd;

    its.it_value.tv_sec = jail_config->timeout;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    ret = timerfd_settime(tfd, 0, &its, NULL);
    if (ret) {
        log_errno("error timerfd settime");
        return -1;
    }

    struct epoll_event event;
    event.data.fd = tfd;
    event.events = EPOLLIN | EPOLLET;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &event);
    if (ret) {
        log_errno("error epoll_ctl");
        close(tfd);
        return -1;
    }

    return tfd;
}

/*
 * event loop
 */
#define MAX_EVENTS_SIZE 1024


int jail_loop() {
    int epfd = -1, sfd = -1, tfd = -1;
    int nfds = -1;
    struct epoll_event events[MAX_EVENTS_SIZE];
    int ret = -1, i = -1;

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        log_errno("error epoll create");
        return -1;
    }

    sfd = register_signal_event(epfd);
    if (sfd < 0) {
        log_errno("error register signal fd");
        return -1;
    }

    tfd = register_timer_event(epfd);
    if (sfd < 0) {
        log_errno("error register timer fd");
        return -1;
    }

    for (;;) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS_SIZE, 1000);
        for (i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == sfd) {
                    ret = handle_signal_event(sfd);
                    if (ret == 0) goto end;
                }
                else if (events[i].data.fd == tfd) {
                    ret = handle_timer_event(tfd);
                }
            }
        }
    }

end:
    log_debug("event loop stopped");
    return 0;
}
