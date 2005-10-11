/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.68 2005-10-11 10:32:26 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiempartserv.h"

#include "binidselimpl.h"
#include "emmanager.h"
#include "emhistory.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "emposid.h"
#include "emhorizon.h"
#include "emfault.h"

#include "datainpspec.h"
#include "parametricsurface.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "survinfo.h"
#include "binidvalset.h"
#include "binidselimpl.h"
#include "surfaceinfo.h"
#include "cubesampling.h"
#include "uiimphorizon.h"
#include "uiimpfault.h"
#include "uiexphorizon.h"
#include "uiiosurfacedlg.h"
#include "uigeninputdlg.h"
#include "uilistboxdlg.h"
#include "uisurfaceman.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uimenu.h"
#include "ptrman.h"

#include <math.h>

const int uiEMPartServer::evDisplayHorizon = 0;

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }

#define mDynamicCastAll(objid) \
    EM::EMManager& em = EM::EMM(); \
    EM::EMObject* object = em.getObject(objid); \
    mDynamicCastGet(EM::Surface*,surface,object) \
    mDynamicCastGet(EM::Horizon*,hor,object) \
    mDynamicCastGet(EM::Fault*,fault,object) \


uiEMPartServer::uiEMPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, selemid_(-1)
{
}


uiEMPartServer::~uiEMPartServer()
{
}


MultiID uiEMPartServer::getStorageID( const EM::ObjectID& emid ) const
{
    return EM::EMM().getMultiID(emid);
}


EM::ObjectID uiEMPartServer::getObjectID( const MultiID& mid ) const
{
    return EM::EMM().getObjectID(mid);
}


void uiEMPartServer::manageSurfaces( bool hor )
{
    uiSurfaceMan dlg( appserv().parent(), hor );
    dlg.go();
}


bool uiEMPartServer::ioHorizon( bool imp )
{
    bool res = false;
    if ( imp )
    {
	uiImportHorizon dlg( appserv().parent() );
	res = dlg.go();
	if ( res && dlg.doDisplay() )
	{
	    const MultiID mid = dlg.getSelID();	
	    selemid_ = EM::EMM().getObjectID(mid);
	    sendEvent( evDisplayHorizon );
	}
    }
    else
    {
	uiExportHorizon dlg( appserv().parent() );
	res = dlg.go();
    }

    return res;    
}


bool uiEMPartServer::importHorizon() { return ioHorizon( true ); }

bool uiEMPartServer::exportHorizon() { return ioHorizon( false ); }


BufferString uiEMPartServer::getName(const EM::ObjectID& id) const
{
    EM::EMManager& em = EM::EMM();
    return em.objectName(em.getMultiID(id));
}


const char* uiEMPartServer::getType(const EM::ObjectID& emid) const
{
    EM::EMManager& em = EM::EMM();
    return em.objectType(em.getMultiID(emid));
}


bool uiEMPartServer::isChanged(const EM::ObjectID& emid) const
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(emid);
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isFullResolution(const EM::ObjectID& emid) const
{
    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::Surface*,emsurf,em.getObject(emid));
    return emsurf && emsurf->geometry.isFullResolution();
}


bool uiEMPartServer::selectHorizon( EM::ObjectID& id )
{ return selectSurface( id, true ); }


bool uiEMPartServer::selectFault( EM::ObjectID& id )
{ return selectSurface(id, false ); }


bool uiEMPartServer::selectSurface( EM::ObjectID& id, bool selhor )
{
    uiReadSurfaceDlg dlg( appserv().parent(), selhor );
    if ( !dlg.go() ) return false;

    IOObj* ioobj = dlg.ioObj();
    if ( !ioobj ) return false;
    
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    dlg.getSelection( sel );
    if ( !loadSurface( ioobj->key(), &sel ) )
	return false;

    id = EM::EMM().getObjectID(ioobj->key());
    return true;
}


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
				  const TypeSet<int>& selattribs )
{
    mDynamicCastAll(id);
    if ( !surface ) return false;

    surface->auxdata.removeAll();
    ExecutorGroup exgrp( "Surface data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( surface->auxdata.auxDataLoader(selattribs[idx]) );

    uiExecutor exdlg( appserv().parent(), exgrp );
    return exdlg.go();
}



bool uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm )
{
    mDynamicCastAll(id);
    if ( !surface ) return false;
    
    EM::SurfaceIOData sd;
    const MultiID mid = em.getMultiID( id );
    em.getSurfaceData( mid, sd );
    const int nritems = sd.valnames.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = *sd.valnames[idx];
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx < 0 ) return false;
    TypeSet<int> selattribs( 1, selidx );
    return loadAuxData( id, selattribs );
}


bool uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    mDynamicCastAll(id);
    if ( !surface ) return false;

    EM::SurfaceIOData sd;
    const MultiID mid = em.getMultiID( id );
    em.getSurfaceData( mid, sd );
    uiListBoxDlg dlg( appserv().parent(), sd.valnames, "Surface data" );
    dlg.box()->setMultiSelect();
    if ( !dlg.go() ) return false;

    TypeSet<int> selattribs;
    dlg.box()->getSelectedItems( selattribs );
    if ( !selattribs.size() ) return false;

    surface->auxdata.removeAll();
    ExecutorGroup exgrp( "Surface data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( surface->auxdata.auxDataLoader(selattribs[idx]) );

    uiExecutor exdlg( appserv().parent(), exgrp );
    return exdlg.go();
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas )
{
    mDynamicCastAll(id);
    if ( !object ) return false;

    PtrMan<Executor> exec = 0;

    if ( storeas && surface )
    {
	uiWriteSurfaceDlg dlg( appserv().parent(), *surface );
	if ( !dlg.go() ) return false;

	EM::SurfaceIOData sd;
	EM::SurfaceIODataSelection sel( sd );
	dlg.getSelection( sel );

	const MultiID& key = dlg.ioObj() ? dlg.ioObj()->key() : "";
	exec = surface->geometry.saver( &sel, &key );
    }
    else
	exec = object->saver();

    if ( !exec )
	return false;

    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go();
}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id, bool storeas )
{
    mDynamicCastAll(id);
    if ( !surface ) return false;

    int dataidx = -1;
    int fileidx = -1;
    if ( storeas )
    {
	uiStoreAuxData dlg( appserv().parent(), *surface );
	if ( !dlg.go() ) return false;

	dataidx = 0;
	fileidx = dlg.getDataFileIdx();
    }

    PtrMan<Executor> saver = surface->auxdata.auxDataSaver( dataidx, fileidx );
    if ( !saver )
    {
	uiMSG().error( "Cannot save attribute" );
	return false;
    }

    uiExecutor exdlg( appserv().parent(), *saver );
    return exdlg.go();
}


void uiEMPartServer::setAuxData( const EM::ObjectID& id,
				 ObjectSet<BinIDValueSet>& data, 
				 const char* attribnm )
{
    BufferStringSet nms; nms.add( attribnm );
    setAuxData( id, data, nms );
}



void uiEMPartServer::setAuxData( const EM::ObjectID& id,
				 ObjectSet<BinIDValueSet>& data, 
				 const BufferStringSet& attribnms )
{
    mDynamicCastAll(id);
    if ( !surface ) { uiMSG().error( "Cannot find surface" ); return; }
    if ( !data.size() ) { uiMSG().error( "No data calculated" ); return; }

    surface->auxdata.removeAll();

    const int nrdatavals = data[0]->nrVals();
    TypeSet<int> dataidxs;
    for ( int idx=0; idx<nrdatavals; idx++ )
    {
	BufferString name;
	if ( idx<attribnms.size() )
	    name = attribnms.get(idx);
	else
	{
	    name = "AuxData"; name += idx;
	}

	dataidxs += surface->auxdata.addAuxData( name );
    }

    BinID bid;
    BinIDValueSet::Pos pos;
    const int nrvals = surface->auxdata.nrAuxData();
    float vals[nrvals];
    for ( int sidx=0; sidx<data.size(); sidx++ )
    {
	const EM::SectionID sectionid = surface->geometry.sectionID( sidx );
	BinIDValueSet& bivs = *data[sidx];

	EM::PosID posid( id, sectionid );
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    RowCol rc( bid.inl, bid.crl );
	    EM::SubID subid = surface->geometry.rowCol2SubID( rc );
	    posid.setSubID( subid );
	    for ( int idv=0; idv<nrvals; idv++ )
		surface->auxdata.setAuxDataVal( dataidxs[idv], posid, 
						vals[idv+1] );
	}
    }
}


void  uiEMPartServer::removeHistory()
{
    EM::EMM().history().empty();
}



bool uiEMPartServer::loadSurface( const MultiID& mid,
       				  const EM::SurfaceIODataSelection* newsel )
{
    EM::EMManager& em = EM::EMM();
    if ( em.getObject(em.getObjectID(mid)) )
	return true;

    PtrMan<Executor> exec = em.objectLoader( mid, newsel );
    if ( !exec )
    {
	PtrMan<IOObj> ioobj = IOM().get(mid);
	BufferString nm = ioobj
	    ? (const char*) ioobj->name()
	    : (const char*) mid;
	mErrRet( nm );
    }

    EM::EMObject* obj = em.getObject(em.getObjectID(mid));
    obj->ref();
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	obj->unRef();
	return false;
    }

    obj->unRefNoDelete();
    return true;
}


bool uiEMPartServer::importLMKFault()
{
    uiImportLMKFault dlg( appserv().parent() );
    return dlg.go();
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    EM::EMManager& em = EM::EMM();
    for ( int idx=0; idx<em.nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon*,hor,em.getObject(em.objectID(idx)));
	if ( hor ) hinfos += new SurfaceInfo( hor->name(), hor->id() );
    }
}


void uiEMPartServer::getSurfaceDef( const TypeSet<EM::ObjectID>& selhorids,
				    BinIDValueSet& bivs,
				    const BinIDRange* br ) const
{
    bivs.empty(); bivs.setNrVals( 2, false );
    PtrMan<BinIDRange> sibr = 0;
    if ( !selhorids.size() ) return;
    else if ( !br )
    {
	sibr = new BinIDRange; br = sibr;
	sibr->start = SI().sampling(false).hrg.start;
	sibr->stop = SI().sampling(false).hrg.stop;
    }

    EM::EMManager& em = EM::EMM();
    const EM::ObjectID& id = selhorids[0]; 
    mDynamicCastGet(EM::Horizon*,hor,em.getObject(id))
    if ( !hor ) return;
    hor->ref();

    EM::Horizon* hor2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor2 = (EM::Horizon*)(em.getObject(selhorids[1]));
	hor2->ref();
    }

    const BinID step( SI().inlStep(), SI().crlStep() );
    BinID bid;
    for ( bid.inl=br->start.inl; bid.inl<br->stop.inl; bid.inl+=step.inl )
    {
	for ( bid.crl=br->start.crl; bid.crl<br->stop.crl; bid.crl+=step.crl )
	{
	    RowCol rc(bid.inl,bid.crl);
	    TypeSet<Coord3> z1pos, z2pos;
	    hor->geometry.getPos( rc, z1pos );
	    if ( !z1pos.size() ) continue;

	    if ( !hor2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		    bivs.add( bid, z1pos[posidx].z, z1pos[posidx].z );
	    }
	    else
	    {
		hor2->geometry.getPos( rc, z2pos );
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
		bivs.add( bid, zintv.start, zintv.stop );
	    }
	}
    }
    
    hor->unRef();
    if ( hor2 ) hor2->unRef();
}
