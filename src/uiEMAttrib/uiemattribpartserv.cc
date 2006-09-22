/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.cc,v 1.2 2006-09-22 08:24:00 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uiseiseventsnapper.h"


uiEMAttribPartServer::uiEMAttribPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , nlamodel_(0)
    , descset_(0)
{}


void uiEMAttribPartServer::createHorizonOutput( HorOutType type )
{
    if ( !descset_ ) return;

    if ( type==OnHor )
    {
	uiAttrSurfaceOut dlg( parent(), *descset_, nlamodel_, nlaid_ );
	dlg.go();
    }
    else
    {
	uiAttrTrcSelOut dlg( parent(), *descset_, nlamodel_, nlaid_,
			     type==AroundHor );
	dlg.go();
    }
}


void uiEMAttribPartServer::snapHorizon()
{
    uiSeisEventSnapper dlg( parent() );
    dlg.go();
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
