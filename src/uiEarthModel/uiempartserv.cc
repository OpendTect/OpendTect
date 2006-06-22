/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiempartserv.cc,v 1.82 2006-06-22 18:46:46 cvskris Exp $
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
#include "uimultisurfaceread.h"
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
    EM::EMObject* object = em_.getObject(objid); \
    mDynamicCastGet(EM::Surface*,surface,object) \
    mDynamicCastGet(EM::Horizon*,hor,object) \
    mDynamicCastGet(EM::Fault*,fault,object) \


uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , selemid_(-1)
    , em_(EM::EMM())
{
}


uiEMPartServer::~uiEMPartServer()
{
    em_.empty();
}


MultiID uiEMPartServer::getStorageID( const EM::ObjectID& emid ) const
{
    return em_.getMultiID(emid);
}


EM::ObjectID uiEMPartServer::getObjectID( const MultiID& mid ) const
{
    return em_.getObjectID(mid);
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
	    selemid_ = em_.getObjectID(mid);
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


BufferString uiEMPartServer::getName( const EM::ObjectID& id ) const
{
    return em_.objectName(em_.getMultiID(id));
}


const char* uiEMPartServer::getType( const EM::ObjectID& emid ) const
{
    return em_.objectType(em_.getMultiID(emid));
}


bool uiEMPartServer::isChanged( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isFullResolution( const EM::ObjectID& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,em_.getObject(emid));
    return emsurf && emsurf->geometry().isFullResolution();
}


bool uiEMPartServer::isFullyLoaded( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = em_.getObject(emid);
    return emobj && emobj->isFullyLoaded();
}


bool uiEMPartServer::askUserToSave( const EM::ObjectID& emid ) const
{
    if ( !isChanged(emid) )
	return true;

    const EM::EMObject* emobj = em_.getObject(emid);
    BufferString msg( emobj->getTypeStr() );
    msg += " '";
    msg += emobj->name(); msg += "' has changed.\nDo you want to save it?";
    if ( uiMSG().notSaved( msg,0,false) )
	return storeObject(emid,!isFullyLoaded(emid) );

    return true;
}


void uiEMPartServer::selectHorizons( TypeSet<EM::ObjectID>& ids )
{ selectSurfaces( ids, true ); }


void uiEMPartServer::selectFaults( TypeSet<EM::ObjectID>& ids )
{ selectSurfaces( ids, false ); }


void uiEMPartServer::selectSurfaces( TypeSet<EM::ObjectID>& objids, bool ishor )
{
    BufferString lbl( ishor ? "Horizon" : "Fault" ); lbl += " selection";
    uiDialog dlg( appserv().parent(), uiDialog::Setup(lbl) );
    uiMultiSurfaceRead* uiobj = new uiMultiSurfaceRead( &dlg, ishor );
    if ( !dlg.go() ) return;

    TypeSet<MultiID> surfaceids;
    uiobj->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    uiobj->getSurfaceSelection( sel );

    PtrMan<Executor> exec = em_.objectLoader( surfaceids, &sel );
    if ( !exec ) return;

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	EM::EMObject* obj = em_.getObject( em_.getObjectID(surfaceids[idx]) );
	obj->ref();
    }

    uiExecutor execdlg( appserv().parent(), *exec );
    if ( !execdlg.go() )
    {
	for ( int idx=0; idx<surfaceids.size(); idx++ )
	{
	    EM::EMObject* obj =
			em_.getObject( em_.getObjectID(surfaceids[idx]) );
	    obj->unRef();
	}

	return;
    }

    exec = 0; //We don't want executor to unref objs at end of function

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	const EM::ObjectID objid = em_.getObjectID( surfaceids[idx] );
	EM::EMObject* obj = em_.getObject( objid );
	obj->unRefNoDelete();
	objids += objid;
    }
}


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
				  const TypeSet<int>& selattribs )
{
    mDynamicCastAll(id);
    if ( !hor ) return false;

    hor->auxdata.removeAll();
    ExecutorGroup exgrp( "Surface data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor->auxdata.auxDataLoader(selattribs[idx]) );

    uiExecutor exdlg( appserv().parent(), exgrp );
    return exdlg.go();
}



int uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm )
{
    mDynamicCastAll(id);
    if ( !hor ) return -1;
    
    EM::SurfaceIOData sd;
    const MultiID mid = em_.getMultiID( id );
    em_.getSurfaceData( mid, sd );
    const int nritems = sd.valnames.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = *sd.valnames[idx];
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx<0 ) return -1;
    TypeSet<int> selattribs( 1, selidx );
    return loadAuxData( id, selattribs ) ? selidx : -1;
}


int uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    mDynamicCastAll(id);
    if ( !hor ) return -1;

    EM::SurfaceIOData sd;
    const MultiID mid = em_.getMultiID( id );
    em_.getSurfaceData( mid, sd );
    uiListBoxDlg dlg( appserv().parent(), sd.valnames, "Surface data" );
    dlg.box()->setMultiSelect(false);
    if ( !dlg.go() ) return -1;

    TypeSet<int> selattribs;
    dlg.box()->getSelectedItems( selattribs );
    if ( !selattribs.size() ) return -1;

    hor->auxdata.removeAll();
    PtrMan<Executor> exec = hor->auxdata.auxDataLoader( selattribs[0] );
    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go() ? 0 : -1;
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas ) const
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
	exec = surface->geometry().saver( &sel, &key );
	if ( exec ) surface->setMultiID( key );
    }
    else
	exec = object->saver();

    if ( !exec )
	return false;

    uiExecutor exdlg( appserv().parent(), *exec );
    return exdlg.go();
}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id, bool storeas ) const
{
    mDynamicCastAll(id);
    if ( !hor ) return false;

    int dataidx = -1;
    bool overwrite = false;
    if ( storeas )
    {
	uiStoreAuxData dlg( appserv().parent(), *hor );
	if ( !dlg.go() ) return false;

	dataidx = 0;
	overwrite = dlg.doOverWrite();
    }

    PtrMan<Executor> saver = hor->auxdata.auxDataSaver( dataidx, overwrite );
    if ( !saver )
    {
	uiMSG().error( "Cannot save attribute" );
	return false;
    }

    uiExecutor exdlg( appserv().parent(), *saver );
    return exdlg.go();
}


int uiEMPartServer::setAuxData( const EM::ObjectID& id,
				ObjectSet<const BinIDValueSet>& data, 
				const char* attribnm, int idx )
{
    mDynamicCastAll(id);
    if ( !hor ) { uiMSG().error( "Cannot find horizon" ); return -1; }
    if ( !data.size() ) { uiMSG().error( "No data calculated" ); return -1; }

    const int nrdatavals = data[0]->nrVals();
    if ( idx>=nrdatavals ) return -1;

    BufferString name;
    if ( attribnm )
	name = attribnm;
    else
    {
	name = "AuxData";
	name += idx;
    }

    hor->auxdata.removeAll();
    const int auxdatanr = hor->auxdata.addAuxData( name );

    BinID bid;
    BinIDValueSet::Pos pos;
    const int nrvals = hor->auxdata.nrAuxData();
    float vals[nrvals];
    for ( int sidx=0; sidx<data.size(); sidx++ )
    {
	const EM::SectionID sectionid = hor->sectionID( sidx );
	const BinIDValueSet& bivs = *data[sidx];

	EM::PosID posid( id, sectionid );
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    RowCol rc( bid.inl, bid.crl );
	    EM::SubID subid = rc.getSerialized();
	    posid.setSubID( subid );
	    hor->auxdata.setAuxDataVal( auxdatanr, posid, vals[idx] );
	}
    }

    return auxdatanr;
}


bool uiEMPartServer::getAuxData( const EM::ObjectID& oid, int auxdatanr,
				 BufferString& auxdataname,
	                         ObjectSet<BinIDValueSet>& auxdata ) const
{
    mDynamicCastAll(oid);
    if ( !hor || !hor->auxdata.auxDataName(auxdatanr) )
	return false;

    auxdataname = hor->auxdata.auxDataName( auxdatanr );
    deepErase( auxdata );
    auxdata.allowNull( true );

    for ( int idx=0; idx<hor->nrSections(); idx++ )
    {
	const EM::SectionID sid = hor->sectionID(idx);
	if ( !hor->geometry().sectionGeometry( sid ) )
	{
	    auxdata += 0;
	    continue;
	}

	BinIDValueSet* res = new BinIDValueSet( 2, false );
	auxdata += res;

	float auxvals[2];
	BinID bid;
	auxvals[0] = 0;
	PtrMan<EM::EMObjectIterator> iterator = hor->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    auxvals[1] = hor->auxdata.getAuxDataVal( auxdatanr, pid );
	    bid.setSerialized( pid.subID() );
	    res->add( bid, auxvals );
	}
    }

    return true;
}


void uiEMPartServer::removeHistory()
{
    em_.history().empty();
}


bool uiEMPartServer::loadSurface( const MultiID& mid,
       				  const EM::SurfaceIODataSelection* newsel )
{
    if ( em_.getObject(em_.getObjectID(mid)) )
	return true;

    Executor* exec = em_.objectLoader( mid, newsel );
    if ( !exec )
    {
	PtrMan<IOObj> ioobj = IOM().get(mid);
	BufferString nm = ioobj ? (const char*) ioobj->name()
				: (const char*) mid;
	mErrRet( nm );
    }

    EM::EMObject* obj = em_.getObject( em_.getObjectID(mid) );
    obj->ref();
    uiExecutor exdlg( appserv().parent(), *exec );
    if ( exdlg.go() <= 0 )
    {
	obj->unRef();
	return false;
    }

    delete exec;
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
    for ( int idx=0; idx<em_.nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon*,hor,em_.getObject(em_.objectID(idx)));
	if ( hor ) hinfos += new SurfaceInfo( hor->name(), hor->multiID() );
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

    const EM::ObjectID& id = selhorids[0]; 
    mDynamicCastGet(EM::Horizon*,hor,em_.getObject(id))
    if ( !hor ) return;
    hor->ref();

    EM::Horizon* hor2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor2 = (EM::Horizon*)(em_.getObject(selhorids[1]));
	hor2->ref();
    }

    const BinID step( SI().inlStep(), SI().crlStep() );
    BinID bid;
    for ( bid.inl=br->start.inl; bid.inl<br->stop.inl; bid.inl+=step.inl )
    {
	for ( bid.crl=br->start.crl; bid.crl<br->stop.crl; bid.crl+=step.crl )
	{
	    const EM::SubID subid = bid.getSerialized();
	    TypeSet<Coord3> z1pos, z2pos;
	    for ( int idx=hor->nrSections()-1; idx>=0; idx-- )
	    {
		const EM::SectionID sid = hor->sectionID( idx );
		if ( hor->isDefined( sid, subid ) )
		    z1pos += hor->getPos( sid, subid );
	    }

	    if ( !z1pos.size() ) continue;

	    if ( !hor2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		    bivs.add( bid, z1pos[posidx].z, z1pos[posidx].z );
	    }
	    else
	    {
		for ( int idx=hor2->nrSections()-1; idx>=0; idx-- )
		{
		    const EM::SectionID sid = hor2->sectionID( idx );
		    if ( hor2->isDefined( sid, subid ) )
			z2pos += hor2->getPos( sid, subid );
		}

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
