/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.36 2003-11-24 08:54:54 kristofer Exp $
________________________________________________________________________

-*/

#include "uiempartserv.h"

#include "datainpspec.h"
#include "emhorizontransl.h"
#include "emmanager.h"
#include "emfaulttransl.h"
#include "emsurfaceiodata.h"
#include "emsticksettransl.h"
#include "emposid.h"
#include "geommeshsurface.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "survinfo.h"
#include "surfaceinfo.h"
#include "uiimphorizon.h"
#include "uiimpfault.h"
#include "uiexphorizon.h"
#include "uiiosurfacedlg.h"
#include "uigeninputdlg.h"
#include "uisurfaceman.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uimenu.h"
#include "ptrman.h"

const int uiEMPartServer::evDisplayHorizon = 0;

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }

uiEMPartServer::uiEMPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, selemid_(*new MultiID(""))
{
}


uiEMPartServer::~uiEMPartServer()
{
    delete &selemid_;
}


void uiEMPartServer::manageSurfaces()
{
    uiSurfaceMan dlg( appserv().parent() );
    dlg.go();
}


bool uiEMPartServer::ioHorizon( bool imp )
{
    PtrMan<uiDialog> dlg = imp
		? (uiDialog*)new uiImportHorizon( appserv().parent() )
		: (uiDialog*)new uiExportHorizon( appserv().parent() );
    if ( !dlg->go() )
	return false;

    mDynamicCastGet(uiImportHorizon*,impdlg,dlg.ptr());
    if ( impdlg && impdlg->doDisplay() )
    {
	selemid_ = impdlg->getSelID();	
	sendEvent( evDisplayHorizon );
    }

    return true;    
}


bool uiEMPartServer::importHorizon() { return ioHorizon( true ); }

bool uiEMPartServer::exportHorizon() { return ioHorizon( false ); }


BufferString uiEMPartServer::getName(const MultiID& mid) const
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::EMObject*,emobj,em.getObject(em.multiID2ObjectID(mid)));
    if ( !emobj ) return 0;
    return emobj->name();
}


bool uiEMPartServer::selectHorizon( MultiID& id )
{
    uiReadSurfaceDlg dlg( appserv().parent() );
    if ( !dlg.go() ) return false;

    IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return false;
    
    id = ioobj->key();

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    dlg.getSelection( sel );
    return loadSurface( id, &sel );
}


bool uiEMPartServer::selectStickSet( MultiID& multiid )
{
   PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(EMStickSet);
   ctio->ctxt.forread = true;
   uiIOObjSelDlg dlg( appserv().parent(), *ctio );
   if ( !dlg.go() ) return false;

    multiid = dlg.ioObj()->key();

    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.multiID2ObjectID(multiid);
    if ( em.getObject(objid) )
	return true;

    PtrMan<Executor> loadexec = em.load(multiid);
    if ( !loadexec)
	mErrRet( IOM().nameOf(multiid) );

    EM::EMM().ref( objid );
    uiExecutor exdlg( appserv().parent(), *loadexec );
    if ( exdlg.go() <= 0 )
    {
	EM::EMM().unRef( objid );
	return false;
    }

    EM::EMM().unRefNoDel( objid );
    return true;
}


bool uiEMPartServer::createStickSet( MultiID& id )
{
    DataInpSpec* inpspec = new StringInpSpec();
    uiGenInputDlg dlg( appserv().parent(), "Enter name", "EnterName", inpspec );

    bool success = false;
    while ( !success )
    {
	if ( !dlg.go() )
	    break;

	id = EM::EMM().add( EM::EMManager::StickSet, dlg.text() );
	if ( id.ID(0)!=-1 )
	    success = true;
    }

    return success;
}


bool uiEMPartServer::loadAuxData( const MultiID& id, int selidx )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(id);
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( !hor ) return false;

    hor->removeAllAuxdata();
    PtrMan<Executor> exec = hor->loader( 0, selidx );
    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go();
}


bool uiEMPartServer::loadAuxData( const MultiID& id, const char* attrnm )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(id);
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( !hor ) return false;
    
    EM::SurfaceIOData sd;
    em.getSurfaceData( id, sd );
    const int nritems = sd.valnames.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = *sd.valnames[idx];
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx < 0 ) return false;
    return loadAuxData( id, selidx );
}


int uiEMPartServer::createAuxDataSubMenu( uiPopupMenu& mnu, int startidx, 
					  const MultiID& id, bool hasauxdata )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(id);
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( !hor ) return 0;

    const char* curval = hor->auxDataName( 0 );
    EM::SurfaceIOData sd;
    em.getSurfaceData( id, sd );

    const int nritems = sd.valnames.size();
    uiPopupMenu* popmnu = new uiPopupMenu( appserv().parent(), "Surface data" );
    mnu.insertItem( popmnu );
    popmnu->setEnabled( nritems );

    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = *sd.valnames[idx];
	uiMenuItem* itm = new uiMenuItem( nm );
	popmnu->insertItem( itm, startidx+idx );
	bool docheck = hasauxdata && nm == curval;
	itm->setChecked( docheck );
	if ( docheck )
	    popmnu->setChecked( true );
    }

    return nritems;
}


bool uiEMPartServer::storeObject( const MultiID& id )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(id);
    PtrMan<Executor> exec = 0;

    EM::EMObject* object = em.getObject(objid);
    if ( !object )
	return false;

    mDynamicCastGet(EM::Horizon*,hor,object);
    if ( hor )
    {
	uiWriteSurfaceDlg dlg( appserv().parent(), *hor );
	if ( !dlg.go() ) return false;

	EM::SurfaceIOData sd;
	EM::SurfaceIODataSelection sel( sd );
	dlg.getSelection( sel );

	bool auxdataonly = dlg.auxDataOnly();
	const MultiID& key = dlg.ioObj() ? dlg.ioObj()->key() : "";
	exec = hor->saver( &sel, auxdataonly, &key );
    }
    else
    {
	mDynamicCastGet(EM::Fault*,fault,object);
	exec = fault ? fault->saver() : object->saver();
    }

    if ( !exec )
	return false;

    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go();
}


bool uiEMPartServer::getDataVal( const MultiID& emid_, 
				 ObjectSet< TypeSet<BinIDZValue> >& data, 
				 BufferString& attrnm, float& shift )
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID emid = em.multiID2ObjectID(emid_);
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(emid))
    if ( !hor ) return false;

    if ( !hor->nrAuxData() )
	return false;

    int dataidx = 0;
    attrnm = hor->auxDataName( dataidx );
    shift = hor->getShift();

    deepErase( data );
    for ( int patchidx=0; patchidx<hor->nrPatches(); patchidx++ )
    {
	const EM::PatchID patchid = hor->patchID( patchidx );
	const Geometry::MeshSurface* meshsurf = hor->getSurface( patchid );

	data += new TypeSet<BinIDZValue>;
	TypeSet<BinIDZValue>& res = *data[patchidx];

	EM::PosID posid( emid, patchid );
	const int nrnodes = meshsurf->size();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Geometry::PosID geomposid = meshsurf->getPosID(idy);
	    const Coord3 coord = meshsurf->getPos( geomposid );
	    const BinID bid = SI().transform(coord);
	    const RowCol emrc( bid.inl, bid.crl );
	    const EM::SubID subid = hor->rowCol2SubID( emrc );
	    posid.setSubID( subid );
	    const float auxvalue = hor->getAuxDataVal(dataidx,posid);

	    res += BinIDZValue(bid,auxvalue,auxvalue);
	}
    }

    return true;
}


void uiEMPartServer::setDataVal( const MultiID& emid, 
				 ObjectSet< TypeSet<BinIDZValue> >& data,
       				 const char* attrnm )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(emid);
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(objid))
    if ( !hor ) return;

    hor->removeAllAuxdata();
    int	dataidx = hor->addAuxData( attrnm );

    for ( int patchidx=0; patchidx<data.size(); patchidx++ )
    {
	const EM::PatchID patchid = hor->patchID( patchidx );
	TypeSet<BinIDZValue>& bidzvals = *data[patchidx];

	EM::PosID posid( objid, patchid );
	for ( int idx=0; idx<bidzvals.size(); idx++ )
	{
	    BinIDZValue bidzv = bidzvals[idx];
	    RowCol rc( bidzv.binid.inl, bidzv.binid.crl );
	    EM::SubID subid = hor->rowCol2SubID( rc );
	    posid.setSubID( subid );
	    hor->setAuxDataVal( dataidx, posid, bidzv.value );
	}
    }
}


bool uiEMPartServer::selectFault( MultiID& id )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(EMFault);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( !dlg.go() ) return false;

    id = dlg.ioObj()->key();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    return loadSurface( id, &sel );
}


bool uiEMPartServer::loadSurface( const MultiID& id,
       				  const EM::SurfaceIODataSelection* newsel )
{
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID objid = em.multiID2ObjectID(id);
    if ( em.getObject(objid) )
	return true;

    PtrMan<Executor> exec = em.load(id, newsel );
    if ( !exec ) mErrRet( IOM().nameOf(id) );
    EM::EMM().ref( objid );
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	EM::EMM().unRef( objid );
	return false;
    }

    EM::EMM().unRefNoDel( objid );
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


void uiEMPartServer::getSurfaceDef( const ObjectSet<MultiID>& selhorids,
				    TypeSet<BinID>& bidset,
				    TypeSet<Interval<float> >& zrgset,
				    const BinIDRange* br ) const
{
    if ( !selhorids.size() ) return;
    EM::EMManager& em = EM::EMM();
    const EM::ObjectID& id = em.multiID2ObjectID(*selhorids[0]); 
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
    if ( !hor ) return;
    hor->ref();

    EM::Horizon* hor2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor2 = (EM::Horizon*)(em.getObject(em.multiID2ObjectID(*selhorids[1])));
	hor2->ref();
    }

    bidset.erase();
    zrgset.erase();

    BinID step( SI().inlStep(), SI().crlStep() );

    for ( int inl=br->start.inl; inl<br->stop.inl; inl+=step.inl )
    {
	for ( int crl=br->start.crl; crl<br->stop.crl; crl+=step.crl )
	{
	    RowCol rc(inl,crl);
	    TypeSet<Coord3> z1pos, z2pos;
	    hor->getPos( rc, z1pos );
	    if ( !z1pos.size() ) continue;

	    if ( !hor2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		{
		    bidset += BinID(inl,crl);
		    zrgset += Interval<float>(z1pos[posidx].z,z1pos[posidx].z);
		}
	    }
	    else
	    {
		hor2->getPos( rc, z2pos );
		if ( !z2pos.size() ) continue;

		Interval<float> zintv;
		float dist = 999999;
		for ( int z1idx=0; z1idx<z1pos.size(); z1idx++ )
		{
		    for ( int z2idx=0; z2idx<z2pos.size(); z2idx++ )
		    {
			float dist_ = z2pos[z2idx].z - z1pos[z1idx].z;
			if ( fabs(dist_) < dist )
			{
			    zintv.start = z1pos[z1idx].z;
			    zintv.stop = z2pos[z2idx].z;
			}
		    }
		}

		zintv.sort();
		bidset += BinID(inl,crl);
		zrgset += zintv;
	    }
	}
    }
    
    hor->unRef();
    if ( hor2 ) hor2->unRef();
}
