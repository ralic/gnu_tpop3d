/*
 * signals.c:
 * Signal handlers for tpop3d.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 *
 */

static const char rcsid[] = "$Id$";

#include <signal.h>
#include <syslog.h>

#include <sys/wait.h>

#include "connection.h"
#include "signals.h"
#include "util.h"

/* set_signals:
 * Set the relevant signals to be ignored/handled.
 */
void set_signals() {
    int ignore_signals[]    = {SIGPIPE, SIGHUP, SIGALRM, SIGUSR1, SIGUSR2, SIGFPE,
#ifdef SIGIO        
        SIGIO,
#endif
#ifdef SIGVTALRM
        SIGVTALRM,
#endif
#ifdef SIGLOST
        SIGLOST,
#endif
#ifdef SIGPWR
        SIGPWR,
#endif
        0};
    int terminate_signals[] = {SIGINT, SIGTERM, 0};
    int restart_signals[]   = {SIGHUP, 0};
    int die_signals[]       = {SIGQUIT, SIGABRT, SIGSEGV, SIGBUS, 0};
    int *i;
    struct sigaction sa;

    for (i = ignore_signals; *i; ++i) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(*i, &sa, NULL);
    }

    for (i = terminate_signals; *i; ++i) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate_signal_handler;
        sigaction(*i, &sa, NULL);
    }
    
    for (i = restart_signals; *i; ++i) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = restart_signal_handler;
        sigaction(*i, &sa, NULL);
    }

    for (i = die_signals; *i; ++i) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = die_signal_handler;
        sigaction(*i, &sa, NULL);
    }

    /* SIGCHLD is special. */
    sa.sa_handler = child_signal_handler;
    sa.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

/* terminate_signal_handler:
 * Signal handler to handle orderly termination of the program.
 */
extern int foad;                            /* in main.c */

void terminate_signal_handler(const int i) {
    foad = 1;
}

/* die_signal_handler:
 * Signal handler to log a message and quit.
 *
 * XXX This is bad, because we call out to functions which may use malloc or
 * file I/O or anything else. However, we quit immediately afterwards, so it's
 * probably OK. Alternatively we would have to siglongjmp out, but that would
 * be undefined behaviour too.
 */
extern connection this_child_connection;    /* in main.c */

void die_signal_handler(const int i) {
    struct sigaction sa;
/*    print_log(LOG_ERR, "quit: %s", sys_siglist[i]); */
    print_log(LOG_ERR, "quit: signal %d", i); /* Some systems do not have sys_siglist. */
    if (this_child_connection) connection_delete(this_child_connection);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(i, &sa, NULL);
    raise(i);
}

/* child_signal_handler:
 * Signal handler to deal with SIGCHLD.
 */
extern int num_running_children;            /* in main.c */

void child_signal_handler(const int i) {
    int status;
    
    while (waitpid(-1, &status, WNOHANG) > 0)
        --num_running_children;
}

/* restart_signal_handler:
 * Signal handler to restart the server on receivinga SIGHUP.
 */
extern int restart;                         /* in main.c */

void restart_signal_handler(const int i) {
    foad = restart = 1;
}