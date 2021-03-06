/*
 * substvars.c:
 * Function for substituting variables in strings.
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

#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

/* xstrncat:
 * Catenate one string onto another, reallocating space for the first. */
char *xstrncat(char *pfx, const char *sfx, const size_t n) {
    char *s;
    s = xmalloc(strlen(pfx) + n + 1);
    if (!s) return NULL;
    strcpy(s, pfx);
    strncat(s, sfx, n);
    xfree(pfx);
    return s;
}

/* substitute_variables:
 * Substitute variables of the form $(foo) or $(bar[2]) in a string. */
#define SET_ERR(cde, txt, off) do {                                  \
                                    if (err) {                       \
                                        err->code = cde;             \
                                        err->msg = txt;              \
                                        err->offset = (off_t)(off);  \
                                    }                                \
                                    xfree(res);                      \
                                    res = NULL;                      \
                                } while (0)

char *substitute_variables(const char *spec, struct sverr *err, const int nvars, ...) {
    const char **var, **val;
    const char *s, *t;
    int n;
    va_list ap;
    char *res;
    
    var = xcalloc(nvars, sizeof *var);
    val = xcalloc(nvars, sizeof *val);
    res = xstrdup("");

    va_start(ap, nvars);
    for (n = 0; n < nvars; ++n) {
        var[n] = va_arg(ap, const char *);
        val[n] = va_arg(ap, const char *);
    }
    va_end(ap);

    s = spec;
    do {
        t = strstr(s, "$(");
        if (!t) {
            res = xstrncat(res, s, strlen(s));
            s += strlen(s);
        } else {
            const char **u, **u_val;
            res = xstrncat(res, s, t - s);
            for (u = var, u_val = val; u < var + nvars; ++u, ++u_val) {
                if (strncmp(t + 2, *u, strlen(*u)) == 0) {
                    /* Found a variable $(.... */
                    const char *c = t + 2 + strlen(*u); /* Character after variable name. */
                    if (*c == '[') {
                        /* Could be an index into the string. */
                        const char *v;
                        char *w;
                        long off;
                        v = c + 1; /* Start of index. */
                        off = strtol(v, &w, 10);
                        if (v == w || *w != ']' || *(w + 1) != ')') {
                            /* Index was invalid. */
                            SET_ERR(sv_syntax, _("Syntax error in character index"), t - spec);
                            goto fail;
                        }

                        if (!*u_val) {
                            /* Null value of variable. */
                            SET_ERR(sv_nullvalue, _("Variable has null value"), t - spec);
                            goto fail;
                        }

                        /* Negative indices correspond to the end of the string. */
                        if (off < 0) off += strlen(*u_val);

                        if (off < 0 || off >= strlen(*u_val)) {
                            /* String was too short. */
                            SET_ERR(sv_range, _("Character index out of range"), t - spec);
                            goto fail;
                        }

                        res = xstrncat(res, *u_val + off, 1);
                        s = w + 2;
                        break;      /* Successful substitution. */
                    } else if (*c == ')') {
                        /* Simple substitution. */
                        if (!*u_val) {
                            /* Null value of variable. */
                            SET_ERR(sv_nullvalue, _("Variable has null value"), t - spec);
                            goto fail;
                        }
                        
                        res = xstrncat(res, *u_val, strlen(*u_val));
                        s = c + 1;
                        break;      /* Successful substitution. */
                    }
                }
            }
            if (u == var + nvars) {
                /* No substitution. */
                SET_ERR(sv_unknown, _("Syntax error or unknown variable"), t - spec);
                goto fail;
            }
        }
    } while (*s);

fail:
    xfree(var);
    xfree(val);

    return res;
}

#undef SET_ERR


#if 0
/* Simple test program. */
int main(int argc, char **argv) {
    struct sverr foo;
    char *x;
    char *sstring = argv[1];
    x = substitute_variables(sstring, &foo, 4, "user", "chris", "domain", "ex-parrot.com", "home", "/home/chris", "homer", "simpson");
    if (x) printf("x = %s\n", x);
    else printf("err = %s near %.16s\n", foo.msg, sstring + foo.offset);
}
#endif
