/*
 * auth_perl.h:
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 *
 * $Id$
 *
 */

#ifndef __AUTH_PERL_H_ /* include guard */
#define __AUTH_PERL_H_

#ifdef HAVE_CONFIG_H
#include "configuration.h"
#endif /* HAVE_CONFIG_H */

#ifdef AUTH_PERL

#include "authswitch.h"
#include "stringmap.h"

/* auth_perl.c */
void xs_init(void);
int auth_perl_init(void);
void auth_perl_close(void);
stringmap auth_perl_callfn(const char *perlfn, const int nvars, ...);
authcontext auth_perl_new_apop(const char *name, const char *timestamp, const unsigned char *digest);
authcontext auth_perl_new_user_pass(const char *user, const char *pass);

#endif /* AUTH_PERL */

#endif /* __AUTH_PERL_H_ */