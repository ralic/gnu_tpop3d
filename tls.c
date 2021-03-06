/*
 * tls.c:
 * TLS stuff for tpop3d.
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
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

#include "configuration.h"

#ifdef USE_TLS

static const char rcsid[] = "$Id$";

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "config.h"
#include "tls.h"
#include "util.h"

#define tls_errorstr()  ERR_reason_error_string(ERR_get_error())

/* Normally, tpop3d will not read a pass phrase for a certificate from the
 * terminal. This is to prevent it from blocking during the boot phase, waiting
 * for the user to type something in. Reading of pass phrases can be enabled
 * using the -P switch to tpop3d. */
int noreadpassphrase = 1;

/* tls_getpassphrase:
 * Obtain a pass phrase from the user. */
static int tls_getpass(char *buf, int size, int rwflag, void *userdata) {
    char *prompt, *s;
    memset(buf, 0, size);
    if (noreadpassphrase) return 0;
    prompt = xmalloc(strlen((char*)userdata) + 40);
    sprintf(prompt, "Enter pass phrase for `%s': ", (char*)userdata);
    /* XXX some systems have unreasonable limits on the length of strings
     * returned by getpass(3). */
    s = getpass(prompt);
    xfree(prompt);
    strncpy(buf, s, size - 1);
    memset(s, 0, strlen(s));    /* paranoia */
    return strlen(buf);
}

/* tls_init
 * Global TLS initialisation. */
static int tls_init_called;
int tls_init(void) {
    if (tls_init_called)
        return 1;
    SSL_load_error_strings();
    SSL_library_init();
    tls_init_called = 1;
    return 1;
}

/* tls_create_context CERTFILE PKEYFILE
 * Create a new SSL_CTX, reading the certificate and private key from CERTFILE
 * and PKEYFILE. If PKEYFILE is NULL, then we attempt to read the private key
 * from the certificate file. Returns a valid SSL context on success or NULL
 * on failure. */
SSL_CTX *tls_create_context(const char *certfile, const char *pkeyfile) {
    int ret;
    SSL_CTX *ctx;
    
    if (!(ctx = SSL_CTX_new(SSLv23_server_method()))) {
        log_print(LOG_ERR, "tls_create_context: SSL_CTX_new: %s", tls_errorstr());
        return NULL;
    }

    /* Set up the password call back. */
    SSL_CTX_set_default_passwd_cb(ctx, tls_getpass);

    /* Load certificate, and, if specified, separate private key. */
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)certfile);
    if ((ret = SSL_CTX_use_certificate_chain_file(ctx, certfile)) <= 0) {
        log_print(LOG_ERR, "tls_create_context: %s: %s", certfile, ERR_reason_error_string(ERR_get_error()));
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)pkeyfile);
    if ((ret = SSL_CTX_use_PrivateKey_file(ctx, pkeyfile ? pkeyfile : certfile, SSL_FILETYPE_PEM)) <= 0) {
        log_print(LOG_ERR, "tls_create_context: %s: %s", pkeyfile ? pkeyfile : certfile, tls_errorstr());
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Verify that the private key matches the certificate. */
    if (!SSL_CTX_check_private_key(ctx)) {
        log_print(LOG_ERR, _("tls_create_context: private key does not match certificate public key"));
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Set various useful options on the context. */
    SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    if (!config_get_bool("tls-no-bug-workarounds"))
        SSL_CTX_set_options(ctx, SSL_OP_ALL);   /* bug workarounds */

    return ctx;
}

/* tls_close:
 * Shut down TLS stuff. */
void tls_close(SSL_CTX *ctx) {
    SSL_CTX_free(ctx);
}

#endif /* USE_TLS */
