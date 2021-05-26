/*
 * cfg.h --- websh configuration
 * nca-073-9
 * 
 * Copyright (c) 1996-2000 by Netcetera AG.
 * Copyright (c) 2001 by Apache Software Foundation.
 * All rights reserved.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: cfg.h 322352 2002-09-11 09:14:06Z ronnie $
 *
 */
#include "tcl.h"
#include "webutl.h"
#include "request.h"
#include "crypt.h"
#include "webout.h"
#include "log.h"


#ifndef CFG_H
#define CFG_H

#define WEB_CFG_ASSOC_DATA "web::cfgData"

#define WEBSH_CONFIG_DEFAULT_TIMESTAMP 0
#define WEBSH_CONFIG_DEFAULT_PUTXTAG   1
#define WEBSH_CONFIG_DEFAULT_ENCRYPT   ""
#define WEBSH_CONFIG_DEFAULT_DECRYPT   ""

#if 0
#define WEBSH_CONFIG_DEFAULT_ENCRYPT   "web::encryptd"
#define WEBSH_CONFIG_DEFAULT_DECRYPT   "web::decryptd"
#endif


/* ----------------------------------------------------------------------------
 * CfgData
 * ------------------------------------------------------------------------- */
typedef struct CfgData
{
    RequestData *requestData;
    CryptData *cryptData;
    OutData *outData;
    LogData *logData;
}
CfgData;


/*void dCfgData(ClientData clientData);*/

CfgData *createCfgData(Tcl_Interp * interp);
void destroyCfgData(ClientData clientData, Tcl_Interp * interp);

int cfg_Init(Tcl_Interp * interp);

int Web_Cfg(ClientData clientData,
	    Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[]);

int Web_ConfigPath(Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[]);

#endif
