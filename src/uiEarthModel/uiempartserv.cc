/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.4 2002-09-23 07:10:03 kristofer Exp $
________________________________________________________________________

-*/

#include "uiempartserv.h"
#include "uiimphorizon.h"
#include "uiimpfault.h"
#include "uiimpwelltrack.h"
#include "uiexphorizon.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "emhorizontransl.h"
#include "emmanager.h"
#include "emwelltransl.h"
#include "emfaulttransl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "surfaceinfo.h"

const int uiEMPartServer::evGetHorData = 0;


uiEMPartServer::uiEMPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, selhorid_(-1)
{
}


uiEMPartServer::~uiEMPartServer()
{
}


bool uiEMPartServer::importHorizon()
{
    uiImportHorizon dlg( appserv().parent() );
    return dlg.go();
}


bool uiEMPartServer::selectHorizon( MultiID& id )
{
    CtxtIOObj ctio( EarthModelHorizonTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), ctio );
    if ( !dlg.go() ) return false;

    id = dlg.ioObj()->key();
    if ( !EarthModel::EMM().isLoaded(id) )
    {
	PtrMan<Executor> exec = EarthModel::EMM().load( id );
	uiExecutor dlg( appserv().parent(), *exec );
	if ( dlg.go() <= 0 )
	    return false;
    }

    return true;
}


bool uiEMPartServer::exportHorizon( const ObjectSet<SurfaceInfo>& his )
{
    uiExportHorizon dlg( appserv().parent(), his );
    if ( !dlg.go() ) return false;

    selhorid_ = dlg.selHorID();
    deepErase( horbidzvs_ );
    sendEvent( evGetHorData );
    return dlg.writeAscii( horbidzvs_ );
}


bool uiEMPartServer::importWellTrack()
{
    uiImportWellTrack dlg( appserv().parent() );
    return dlg.go();
}


bool uiEMPartServer::selectWellTracks( ObjectSet<MultiID>& ids )
{
    CtxtIOObj ctio( EarthModelWellTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( ids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	MultiID wellid = dlg.selected(idx)->key();
	if ( !EarthModel::EMM().isLoaded( wellid ) )
	{
	    PtrMan<Executor> exec = EarthModel::EMM().load( wellid );
	    uiExecutor dlg( appserv().parent(), *exec );
	    if ( dlg.go() <= 0 )
		wellid = MultiID("");
	}
	if ( wellid != "" )
	    ids += new MultiID( wellid );
    }

    return ids.size();
}


bool uiEMPartServer::selectFault( MultiID& id )
{
    CtxtIOObj ctio( EarthModelFaultTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), ctio );
    if ( !dlg.go() ) return false;

    id = dlg.ioObj()->key();
    if ( !EarthModel::EMM().isLoaded(id) )
    {
	PtrMan<Executor> exec = EarthModel::EMM().load( id );
	uiExecutor dlg( appserv().parent(), *exec );
	if ( dlg.go() <= 0 )
	    return false;
    }

    return true;
}


bool uiEMPartServer::importLMKFault()
{
    uiImportLMKFault dlg( appserv().parent() );
    return dlg.go();
}


