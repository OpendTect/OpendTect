/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: uiemattribpartserv.cc,v 1.4 2008-06-23 06:38:52 cvsraman Exp $
________________________________________________________________________

-*/


#include "uiemattribpartserv.h"

#include "uiattrsurfout.h"
#include "uiattrtrcselout.h"
#include "uiimphorizon2d.h"
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


void uiEMAttribPartServer::import2DHorizon() const
{
    uiImportHorizon2D dlg( parent() );
    dlg.go();
}
