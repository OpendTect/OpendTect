/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.15 2003-07-16 09:58:06 nanne Exp $
________________________________________________________________________

-*/

#include "uiempartserv.h"
#include "uiimphorizon.h"
#include "uiimpfault.h"
#include "uiimpwelltrack.h"
#include "uiexphorizon.h"
#include "uiiosurfacedlg.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "emhorizontransl.h"
#include "emmanager.h"
#include "emwelltransl.h"
#include "emfaulttransl.h"
#include "emposid.h"
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
    CtxtIOObj ctio( EMHorizonTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOSurfaceDlg dlg( appserv().parent(), ctio );
    if ( !dlg.go() ) return false;

    IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return false;
    
    id = ioobj->key();
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


bool uiEMPartServer::storeSurface( const MultiID& id )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
    if ( !hor ) return false;

    uiSaveSurfaceDlg dlg( appserv().parent(), *hor );
    if ( !dlg.go() ) return false;
    return dlg.doWrite();
}


bool uiEMPartServer::getDataVal( const MultiID& id, 
				 ObjectSet< TypeSet<BinIDZValue> >& data, 
				 BufferString& attrnm )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
    if ( !hor ) return false;

    if ( !hor->nrAuxData() )
	return false;

    int dataidx = 0;
    attrnm = hor->auxDataName( dataidx );

    deepErase( data );
    for ( int patchidx=0; patchidx<hor->nrPatches(); patchidx++ )
    {
	const EM::PatchID patchid = hor->patchID( patchidx );
	const Geometry::GridSurface* gridsurf = hor->getSurface( patchid );

	data += new TypeSet<BinIDZValue>;
	TypeSet<BinIDZValue>& res = *data[0];

	const int nrnodes = gridsurf->size();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Geometry::PosID geomposid = gridsurf->getPosID(idy);
	    const Coord3 coord = gridsurf->getPos( geomposid );
	    const BinID bid = SI().transform(coord);
	    const RowCol emrc( bid.inl, bid.crl );
	    const EM::SubID subid = hor->rowCol2SubID( emrc );
	    const EM::PosID posid( id, patchid, subid );
	    const float auxvalue = hor->getAuxDataVal(dataidx,posid);

	    res += BinIDZValue(bid,auxvalue,auxvalue);
	}
    }

    return true;
}


void uiEMPartServer::setDataVal( const MultiID& id, 
				 ObjectSet< TypeSet<BinIDZValue> >& data,
       				 const char* attrnm )
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
    if ( !hor ) return;

    for ( int idx=0; idx<hor->nrAuxData(); idx++ )
    {
	if ( !strcmp(hor->auxDataName(idx),attrnm) )
	    return;
    }

    int	dataidx = hor->addAuxData( attrnm );

    for ( int patchidx=0; patchidx<data.size(); patchidx++ )
    {
	const EM::PatchID patchid = hor->patchID( patchidx );
	TypeSet<BinIDZValue>& bidzvals = *data[patchidx];

	for ( int idx=0; idx<bidzvals.size(); idx++ )
	{
	    BinIDZValue bidzv = bidzvals[idx];
	    RowCol rc( bidzv.binid.inl, bidzv.binid.crl );
	    EM::SubID subid = hor->rowCol2SubID( rc );
	    EM::PosID posid( id, patchid, subid );
	    hor->setAuxDataVal( dataidx, posid, bidzv.value );
	}
    }
}


bool uiEMPartServer::importWellTrack()
{
    uiImportWellTrack dlg( appserv().parent() );
    return dlg.go();
}


bool uiEMPartServer::selectWellTracks( ObjectSet<MultiID>& ids )
{
    CtxtIOObj ctio( EMWellTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), ctio, 0, true );
    if ( !dlg.go() ) return false;

    deepErase( ids );
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	MultiID wellid = dlg.selected(idx)->key();
	if ( !EM::EMM().isLoaded( wellid ) )
	{
	    PtrMan<Executor> exec = EM::EMM().load( wellid );
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
    CtxtIOObj ctio( EMFaultTranslator::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), ctio );
    if ( !dlg.go() ) return false;

    id = dlg.ioObj()->key();
    return loadSurface( id );
}


bool uiEMPartServer::loadSurface( const MultiID& id )
{
    if ( EM::EMM().isLoaded(id) )
	return true;

    PtrMan<Executor> exec = EM::EMM().load( id );
    if ( !exec ) mErrRet( IOM().nameOf(id) );
    EM::EMM().ref( id );
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	EM::EMM().unRef( id );
	return false;
    }

    EM::EMM().unRefNoDel( id );
    return true;
}


bool uiEMPartServer::importLMKFault()
{
    uiImportLMKFault dlg( appserv().parent() );
    return dlg.go();
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    int nrobjects = EM::EMM().nrObjects();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	EM::EMManager& em = EM::EMM();
	mDynamicCastGet(EM::Horizon*,hor,em.getEMObject(idx))
	if ( hor )
	    hinfos += new SurfaceInfo( hor->name(), hor->id() );
    }
}


void uiEMPartServer::getSurfaceDef( const MultiID& id, 
				    ObjectSet< TypeSet<BinIDValue> >& bidvset,
				    const BinIDRange* br ) const
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
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
