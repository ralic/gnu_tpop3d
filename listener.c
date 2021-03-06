/*
 * listener.c:
 * Objects representing addresses on which to listen.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

static const char rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "configuration.h"
#endif /* HAVE_CONFIG_H */

#include <fcntl.h>
#include <netdb.h>

#ifdef MASS_HOSTING
#include <regex.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include "listener.h"

#ifdef USE_TLS
#include "tls.h"
#endif

#include "util.h"

/* listener_new:
 * Create a new listener object, listening on the specified address. */
listener listener_new(const struct sockaddr_in *addr, const char *domain
#ifdef MASS_HOSTING
 /* leading comma-- yuk */  , const char *regex
#endif
#ifdef USE_TLS
                            , enum tls_mode mode,
                              const char *certfile, const char *pkeyfile
#endif
                        ) {
    listener L;
    struct hostent *he;
    
    alloc_struct(_listener, L);

    L->sin = *addr;
    L->s = socket(PF_INET, SOCK_STREAM, 0);
    if (L->s == -1) {
        log_print(LOG_ERR, "listener_new: socket: %m");
        goto fail;
    } else {
        int t = 1;
        if (setsockopt(L->s, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) == -1) {
            log_print(LOG_ERR, "listener_new: setsockopt: %m");
            goto fail;
        } else if (fcntl(L->s, F_SETFL, O_NONBLOCK) == -1) {
            log_print(LOG_ERR, "listener_new: fcntl: %m");
            goto fail;
        } else if (bind(L->s, (struct sockaddr*)addr, sizeof(struct sockaddr_in)) == -1) {
            log_print(LOG_ERR, "listener_new: bind(%s:%d): %m", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
            goto fail;
        } else if (listen(L->s, SOMAXCONN) == -1) {
            log_print(LOG_ERR, "listener_new: listen: %m");
            goto fail;
        }
    }

#ifdef MASS_HOSTING
    /* Possibly a regex has been specified for mapping name-of-server to
     * domain. */
    if (regex) {
        int err;
        char errbuf[128] = {0};
        if ((err = regcomp(&L->re, regex, REG_EXTENDED | REG_ICASE))) {
            regerror(err, &L->re, errbuf, sizeof errbuf);
            log_print(LOG_WARNING, "listener_new: %s:%d: /%s/: %s", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), L->re, errbuf);
        } else if (L->re.re_nsub != 1) {
            log_print(LOG_WARNING, _("listener_new: %s:%d: /%s/: regular expression should have exactly one bracketed subexpression"), inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), regex);
            regfree(&L->re);
        } else {
            L->have_re = 1;
            L->regex = xstrdup(regex);
        }
        if (!L->have_re)
            log_print(LOG_WARNING, _("listener_new: %s:%d: cannot derive domain information for this address"), inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    } else
#endif
    /* Now, we need to find the domain associated with this socket. */
    if (!domain) {
        he = gethostbyaddr((char *)&(addr->sin_addr), sizeof(addr->sin_addr), AF_INET);
        if (!he) {
            log_print(LOG_WARNING, _("listener_new: gethostbyaddr(%s): cannot resolve name"), inet_ntoa(addr->sin_addr));
            log_print(LOG_WARNING, _("listener_new: %s:%d: cannot obtain domain suffix for this address"), inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        } else {
            /* We need to find out an appropriate domain suffix for the address.
             * FIXME we just take the first address with a "." in it, and use
             * the part after the ".". */
            char **a, *b;
            b = strchr(he->h_name, '.');
            if (b && *(b + 1)) {
                L->domain = xstrdup(b + 1);
            } else 
                for (a = he->h_aliases; *a; ++a) {
                    char *b;
                    fprintf(stderr, "%s\n", *a);
                    if ((b = strchr(*a, '.')) && *(b + 1)) {
                        L->domain = xstrdup(b + 1);
                        break;
                    }
                }

            if (!L->domain)
                log_print(LOG_WARNING, _("listener_new: %s:%d: no suitable domain suffix found for this address"), inet_ntoa(addr->sin_addr), htons(addr->sin_port));
        }
    } else L->domain = xstrdup(domain);

    /* Last try; use the nodename from uname(2). */
    if (!L->domain
#ifdef MASS_HOSTING
            && !L->have_re
#endif
            ) {
        struct utsname u;
        if (uname(&u) == -1) {
            log_print(LOG_WARNING, "listener_new: uname: %m");
            log_print(LOG_WARNING, _("listener_new: %s:%d: using domain suffix `x.invalid'"), inet_ntoa(addr->sin_addr), htons(addr->sin_port));
            L->domain = xstrdup("x.invalid");
        } else {
            log_print(LOG_WARNING, _("listener_new: %s:%d: using fallback domain suffix `%s'"), inet_ntoa(addr->sin_addr), htons(addr->sin_port), u.nodename);
            L->domain = xstrdup(u.nodename);
        }
    }

#ifdef USE_TLS
    /* Should this listener support some sort of TLS? */
    if (mode != none) {
        tls_init();
        L->tls.mode = mode;
        L->tls.ctx = tls_create_context(certfile, pkeyfile);
        if (!L->tls.ctx) {
            if (mode == immediate) {
                log_print(LOG_ERR, _("listener_new: %s:%d: cannot create TLS context for listener; dropping it"), inet_ntoa(addr->sin_addr), htons(addr->sin_port));
                goto fail;
            } else if (mode == stls) {
                log_print(LOG_ERR, _("listener_new: %s:%d: cannot create TLS context; setting TLS mode to `none'"), inet_ntoa(addr->sin_addr), htons(addr->sin_port));
                L->tls.mode = none;
            }
        }
    }
#endif /* USE_TLS */

    return L;

fail:
    listener_delete(L);
    return NULL;
}

/* listener_delete:
 * Delete a listener object, closing the associated socket. */
void listener_delete(listener L) {
    if (!L) return;
    if (L->s != -1) close(L->s); /* Do not shutdown(2). */
    xfree(L->domain);
#ifdef MASS_HOSTING
    if (L->have_re)
        regfree(&L->re);
    xfree(L->regex);
#endif
#ifdef USE_TLS
    if (L->tls.ctx)
        tls_close(L->tls.ctx);
#endif
    xfree(L);
}

#ifdef MASS_HOSTING
/* listener_obtain_domain:
 * Use the regular expression specified for the listener to obtain a domain
 * name from the address to which the given socket is connected. */
char *listener_obtain_domain(listener L, int s) {
    struct sockaddr_in sin;
    size_t l = sizeof sin;
    struct hostent *he;
    regmatch_t match[2];
    int err;

    if (!L->have_re)
        return NULL;

    if (getsockname(s, (struct sockaddr*)&sin, (int*)&l) == -1) {
        /* Shouldn't happen. */
        log_print(LOG_ERR, "listener_obtain_domain: getsockname: %m");
        return NULL;
    }

    if (!(he = gethostbyaddr((char*)&(sin.sin_addr), sizeof(sin.sin_addr), AF_INET))) {
        log_print(LOG_WARNING, _("listener_obtain_domain(%s): cannot resolve name"), inet_ntoa(sin.sin_addr));
        return NULL;
    }

    /* OK, we have a name; we need to run the regular expression against it and
     * check that we get one match exactly. */
    if ((err = regexec(&L->re, he->h_name, 2, match, 0)) == REG_NOMATCH) {
        log_print(LOG_WARNING, _("listener_obtain_domain: /%s/: %s: no regex match"), L->regex, he->h_name);
        return NULL;
    } else if (match[1].rm_so == -1) {
        log_print(LOG_WARNING, _("listener_obtain_domain: /%s/: %s: regex failed to match any subexpression"), L->regex, he->h_name);
        return NULL;
    } else if (match[1].rm_so == match[1].rm_eo) {
        log_print(LOG_WARNING, _("listener_obtain_domain: /%s/: %s: zero-length subexpression"), L->regex, he->h_name);
        return NULL;
    } else {
        char *x;
        int l;
        x = xcalloc((l = match[1].rm_eo - match[1].rm_so) + 1, 1);
        memcpy(x, he->h_name + match[1].rm_so, l);
        return x;
    }
}
#endif
