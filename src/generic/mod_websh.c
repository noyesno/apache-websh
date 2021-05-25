/* ----------------------------------------------------------------------------
 * mod_websh.c -- handler for websh applications for Apache-1.3
 * nca-073-9
 *
 * Copyright (c) 1996-2000 by Netcetera AG.
 * Copyright (c) 2001 by Apache Software Foundation.
 * All rights reserved.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: mod_websh.c 784114 2009-06-12 13:37:16Z ronnie $
 * ------------------------------------------------------------------------- */

/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on software written by Netcetera AG, Zurich, Switzerland.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/* ----------------------------------------------------------------------------
 * tcl/websh includes
 * ------------------------------------------------------------------------- */
#include "tcl.h"		/* tcl headers */
#include "web.h"		/* websh headers */
#include "mod_websh.h"		/* apache stuff */
#include "interpool.h"
#include "logtoap.h"

#define WEBSH_HANDLER "websh"

#ifndef APACHE2
module MODULE_VAR_EXPORT websh_module;
#define APPOOL pool
#else /* APACHE2 */
module AP_MODULE_DECLARE_DATA websh_module;
#define APPOOL apr_pool_t
#endif /* APACHE2 */

#ifdef APACHE2
  #define AP_LOG_RERROR(r, ...) ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, r, __VA_ARGS__);
#else
  #define AP_LOG_RERROR(r, ...) ap_log_rerror(APLOG_MARK, APLOG_ERR, r, __VA_ARGS__);
#endif

/* ============================================================================
 * httpd config and log handling
 * ========================================================================= */

/* Configuration stuff */

#ifndef APACHE2
static void cleanup_websh_pool(void *conf)
{
    /* cleanup the pool when server is restarted (-HUP) */
    destroyPool((websh_server_conf *) conf);
}
#else /* APACHE2 */
static apr_status_t cleanup_websh_pool(void *conf)
{
    /* cleanup the pool when server is restarted (-HUP) */
    destroyPool((websh_server_conf *) conf);
    return APR_SUCCESS;
}
#endif /* APACHE2 */

static void *create_websh_config(APPOOL * pool, server_rec * s)
{

    websh_server_conf *conf;

    conf = (websh_server_conf *) apr_palloc(pool, sizeof(websh_server_conf));

    conf->scriptName = NULL;
    conf->mainInterp = NULL;
    conf->mainInterpLock = NULL;
    conf->webshPool = NULL;
    conf->webshPoolLock = NULL;
    conf->server = s;

    apr_pool_cleanup_register(pool, conf, cleanup_websh_pool, apr_pool_cleanup_null);

    return conf;
}

static void *merge_websh_config(APPOOL * p, void *basev, void *overridesv)
{
    /* fixme-later: is this correct? (reset the locks) */

    /* When we have seperate interpreters for seperate virtual hosts,
     * and things of that nature, then we can worry about this -
     * davidw. */

/*     ((websh_server_conf *) overridesv)->mainInterpLock = NULL;
       ((websh_server_conf *) overridesv)->webshPoolLock = NULL;  */
    return basev;
}

static const char *set_webshscript(cmd_parms * cmd, void *dummy, const char *arg)
{
    server_rec *s = cmd->server;
    websh_server_conf *conf =
	(websh_server_conf *) ap_get_module_config(s->module_config, &websh_module);

    conf->scriptName = ap_server_root_relative(cmd->pool, arg);

    return NULL;
}

#ifdef APACHE2
static void websh_init_child(apr_pool_t * p, server_rec * s)
{
    /* here we create our main Interp and Pool */
    websh_server_conf *conf =
	(websh_server_conf *) ap_get_module_config(s->module_config,
						   &websh_module);
    if (!initPool(conf)) {
	ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
		     "Could not init interp pool");
	return;
    }
}

#else /* APACHE2 */
static void websh_init_child(server_rec *s, pool *p)
{
    /* here we create our main Interp and Pool */
    websh_server_conf *conf =
	(websh_server_conf *) ap_get_module_config(s->module_config,
						   &websh_module);
    if (!initPool(conf)) {
	ap_log_error(APLOG_MARK, APLOG_ERR, s,
		     "Could not init interp pool");
	return;
    }
}
#endif

static const command_rec websh_cmds[] = {
    {"WebshConfig", CMDFUNC set_webshscript, NULL, RSRC_CONF, TAKE1,
     "the name of the main websh configuration file"},
    {NULL}
};

/* ----------------------------------------------------------------------------
 * websh_run_script
 * ------------------------------------------------------------------------- */
static apr_status_t release_webinterp(void *data) {
    WebInterp *webInterp = (WebInterp *) data;

    if(webInterp->state == WIP_EXPIRED_INUSE){
       // TODO:
    }

    Tcl_DeleteAssocData(webInterp->interp, WEB_AP_ASSOC_DATA);
    Tcl_DeleteAssocData(webInterp->interp, WEB_INTERP_ASSOC_DATA);

    poolReleaseThreadWebInterp(webInterp);
}

static int websh_run_script(request_rec * r)
{

    WebInterp *webInterp = NULL;
    websh_server_conf *conf =
	(websh_server_conf *) ap_get_module_config(r->server->module_config,
						   &websh_module);
    // TODO: script timeout check
    /* checkme: check type of timeout in MP case */
    /* ap_soft_timeout("!!! timeout for run_websh_script expired", r); */

    webInterp = poolGetThreadWebInterp(conf, r->filename, (long) r->finfo.mtime, r);

    if (webInterp == NULL){
	return HTTP_INTERNAL_SERVER_ERROR;
    }


    apr_pool_cleanup_register(r->pool, webInterp, release_webinterp, apr_pool_cleanup_null);

    webInterp->time_request = r->request_time;
    webInterp->time_ready   = apr_time_now();

    int status = OK;
    do {

      if (createApchannel(webInterp->interp, r) != TCL_OK) {
          expireWebInterp(webInterp);  // XXX: mark this interp as expired

	  AP_LOG_RERROR(r, "mod_websh - cannot create apchannel");
      
	  status = HTTP_INTERNAL_SERVER_ERROR;
          break;
      }

      Tcl_SetAssocData(webInterp->interp, WEB_AP_ASSOC_DATA, NULL, (ClientData) r);
      Tcl_SetAssocData(webInterp->interp, WEB_INTERP_ASSOC_DATA, NULL, (ClientData) webInterp);

      //-------------------------------------------------------//

      if (Tcl_Eval(webInterp->interp, "web::ap::perReqInit") != TCL_OK) {
          expireWebInterp(webInterp);  // XXX: mark this interp as expired

	  AP_LOG_RERROR(r, "mod_websh - cannot init per-request Websh code: %s", Tcl_GetStringResult(webInterp->interp));
	  status = HTTP_INTERNAL_SERVER_ERROR;
          break;
      }


      int res;

      Tcl_IncrRefCount(webInterp->code);
      res = Tcl_EvalObjEx(webInterp->interp, webInterp->code, 0);
      Tcl_DecrRefCount(webInterp->code);

      if (res != TCL_OK) {
	  expireWebInterp(webInterp);  // XXX: mark this interp as expired

	  char *errorInfo = NULL;
	  errorInfo =
	      (char *) Tcl_GetVar(webInterp->interp, "errorInfo", TCL_GLOBAL_ONLY);

	  logToAp(webInterp->interp, NULL, errorInfo);

	  AP_LOG_RERROR(r, "mod_websh - websh script error : %s", Tcl_GetStringResult(webInterp->interp));

          // TODO: treat script error as HTTP_INTERNAL_SERVER_ERROR or not?
	  // status = HTTP_INTERNAL_SERVER_ERROR;
      }

      Tcl_ResetResult(webInterp->interp);

      if (Tcl_Eval(webInterp->interp, "web::ap::perReqCleanup") != TCL_OK) {
          expireWebInterp(webInterp);  // XXX: mark this interp as expired
	  AP_LOG_RERROR(r, "mod_websh - error while cleaning-up: %s", Tcl_GetStringResult(webInterp->interp));
	  status = HTTP_INTERNAL_SERVER_ERROR;
      }
      //-------------------------------------------------------//

      if (destroyApchannel(webInterp->interp) != TCL_OK) {
          expireWebInterp(webInterp);  // XXX: mark this interp as expired

	  AP_LOG_RERROR(r, "mod_websh - error closing ap-channel");
	  status = HTTP_INTERNAL_SERVER_ERROR;
      }

    } while(0); 

    apr_pool_cleanup_run(r->pool, webInterp, release_webinterp);

    return status;
}

/* ----------------------------------------------------------------------------
 * websh_handler
 * ------------------------------------------------------------------------- */

static int websh_handler(request_rec * r)
{

    int res;

    if (!r->handler || strcmp(r->handler, WEBSH_HANDLER)){
	return DECLINED;
    }

    /* We don't check to see if the file exists, because it might be
     * mapped with web::interpmap. */

    /* XXX: prepare to receive data */
    if ((res = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)))
	return res;

    /* SERVER_SIGNATURE, REMOTE_PORT, .... */
    ap_add_common_vars(r);

    /* GATEWAY_INTERFACE, SERVER_PROTOCOL, ... */
    ap_add_cgi_vars(r);

#ifdef CHARSET_EBCDIC
    ap_bsetflag(r->connection->client, B_EBCDIC2ASCII, 1);
#endif /*CHARSET_EBCDIC */

    /* ---------------------------------------------------------------------
     * ready to rumble
     * --------------------------------------------------------------------- */
    int status;

    status = websh_run_script(r);

    return status;			/* NOT r->status, even if it has changed. */
}

static int websh_post_config(apr_pool_t *pconf, apr_pool_t *ptemp,
                          apr_pool_t *plog, server_rec *s)
{
    char buf[255];

    sprintf(buf, "mod_websh/%s", VERSION);
    ap_add_version_component(pconf, buf);

    return OK;
}


#ifndef APACHE2

static const handler_rec websh_handlers[] = {
    {WEBSH_HANDLER, websh_handler},
    {NULL}
};

module MODULE_VAR_EXPORT websh_module = {
    STANDARD_MODULE_STUFF,
    NULL,			/* initializer */
    NULL,			/* dir config creater */
    NULL,			/* dir merger --- default is to override */
    create_websh_config,	/* server config */
    merge_websh_config,		/* merge server config */
    websh_cmds,			/* command table */
    websh_handlers,		/* handlers */
    NULL,			/* filename translation */
    NULL,			/* check_user_id */
    NULL,			/* check auth */
    NULL,			/* check access */
    NULL,			/* type_checker */
    NULL,			/* fixups */
    NULL,			/* logger */
    NULL,			/* header parser */
    websh_init_child,		/* child_init */
    NULL,		        /* child_exit */
    NULL			/* post read-request */
};

#else /* APACHE2 */

static void register_websh_hooks(apr_pool_t *p)
{

    ap_hook_handler(websh_handler, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_child_init(websh_init_child, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_post_config(websh_post_config, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA websh_module = {
    STANDARD20_MODULE_STUFF,
    NULL,			/* dir config creater */
    NULL,			/* dir merger --- default is to override */
    create_websh_config,	/* server config */
    merge_websh_config,		/* merge server config */
    websh_cmds,			/* command table */
    register_websh_hooks	/* register hooks */
};

#endif /* APACHE2 */
