/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.cc,v 1.1 2006-09-19 09:28:11 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiemattribpartserv.h"

uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
}


uiEMAttribPartServer::~uiEMAttribPartServer()
{
}


void uiEMAttribPartServer::snapHorizon()
{
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
