/*
 * auth_passwd.h: authenticate using /etc/passwd or /etc/shadow
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 *
 * $Id$
 *
 * $Log$
 * Revision 1.3  2001/01/11 21:37:34  chris
 * Minor changes.
 *
 * Revision 1.2  2001/01/11 21:23:35  chris
 * Minor changes.
 *
 *
 */

#ifndef __AUTH_PASSWD_H_ /* include guard */
#define __AUTH_PASSWD_H_

#ifdef AUTH_PASSWD

#include "authswitch.h"

/* auth_passwd.c */
authcontext auth_passwd_new_user_pass(const char *user, const char *pass);

#endif /* AUTH_PASSWD */

#endif /* __AUTH_PASSWD_H_ */
