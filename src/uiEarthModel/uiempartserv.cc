/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.13 2003-06-02 08:16:58 nanne Exp $
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
#include "ioman.h"
#include "survinfo.h"
#include "surfaceinfo.h"
#include "geom2dsnappedsurface.h"

const int uiEMPartServer::evGetHorData = 0;

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }

uiEMPartServer::uiEMPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, selvisid_(-1)
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
    return loadSurface( id );
}


bool uiEMPartServer::exportHorizon( const ObjectSet<SurfaceInfo>& his )
{
    uiExportHorizon dlg( appserv().parent(), his );
    if ( !dlg.go() ) return false;

    selvisid_ = dlg.selVisID();
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
	    if ( !exec ) mErrRet( dlg.ioObj()->name() );
	    uiExecutor exdlg( appserv().parent(), *exec );
	    if ( exdlg.go() <= 0 )
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
    return loadSurface( id );
}


bool uiEMPartServer::loadSurface( const MultiID& id )
{
    if ( EarthModel::EMM().isLoaded(id) )
	return true;

    PtrMan<Executor> exec = EarthModel::EMM().load( id );
    if ( !exec ) mErrRet( IOM().nameOf(id) );
    EarthModel::EMM().ref( id );
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	EarthModel::EMM().unRef( id );
	return false;
    }

    EarthModel::EMM().unRefNoDel( id );
    return true;
}


bool uiEMPartServer::importLMKFault()
{
    uiImportLMKFault dlg( appserv().parent() );
    return dlg.go();
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    int nrobjects = EarthModel::EMM().nrObjects();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	EarthModel::EMManager& em = EarthModel::EMM();
	mDynamicCastGet(EarthModel::Horizon*,hor,em.getEMObject(idx))
	if ( hor )
	    hinfos += new SurfaceInfo( hor->name(), hor->id() );
    }
}


void uiEMPartServer::getSurfaceDef( const MultiID& id, 
				    ObjectSet< TypeSet<BinIDValue> >& bidvset,
				    const BinIDRange* br ) const
{
    EarthModel::EMManager& em = EarthModel::EMM();
    mDynamicCastGet(EarthModel::Horizon*,hor,em.getObject(id))
    if ( !hor ) return;
    hor->ref();

    deepErase( bidvset );
    const int nrsubsurf = hor->nrPatches();
    for ( int idx=0; idx<nrsubsurf; idx++ )
    {
	bidvset += new TypeSet<BinIDValue>;
	TypeSet<BinIDValue>& res = *bidvset[idx];

	const Geometry::Snapped2DSurface* surface =
	    dynamic_cast<const Geometry::Snapped2DSurface*>(
		    			hor->getSurface(hor->patchID(idx)));

	const int nrrows = surface->nrRows();
	for ( int row=0; row<nrrows; row++ )
	{
	    const int nrcols = surface->nrCols( row );
	    for ( int col=0; col<nrcols; col++ )
	    {
		const RowCol rc(row,col);
		Coord3 pos = surface->getGridPos( rc );
		BinIDValue bidval;
		bidval.binid = SI().transform( Coord( pos.x, pos.y ) );
		if ( br && br->excludes( bidval.binid ) )
		    continue;

		bidval.value = pos.z;
		res += bidval;
	    }
	}
    }

    hor->unRef();
}
