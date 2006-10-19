/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.cc,v 1.3 2006-10-19 11:53:45 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uiseiseventsnapper.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"


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


void uiEMAttribPartServer::snapHorizon( const EM::ObjectID& emid )
{
    IOObj* ioobj = IOM().get( EM::EMM().getMultiID(emid) );
    uiSeisEventSnapper dlg( parent(), ioobj );
    dlg.go();
    delete ioobj;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
