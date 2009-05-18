/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiempartserv.cc,v 1.169 2009-05-18 21:26:08 cvskris Exp $";

#include "uiempartserv.h"

#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "embodytr.h"
#include "emrandomposbody.h"
#include "emposid.h"
#include "empolygonbody.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "parametricsurface.h"
#include "pickset.h"
#include "posinfo.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "undo.h"
#include "varlenarray.h"

#include "uiarray2dchg.h"
#include "uiarray2dinterpol.h"
#include "uichangesurfacedlg.h"
#include "uiexpfault.h"
#include "uiexphorizon.h"
#include "uigeninputdlg.h"
#include "uihor3dfrom2ddlg.h"
#include "uihorinterpol.h"
#include "uiimpfault.h"
#include "uiimphorizon.h"
#include "uiexport2dhorizon.h"
#include "uiioobjsel.h"
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultisurfaceread.h"
#include "uirandlinegen.h"
#include "uiselsimple.h"
#include "uisurfaceman.h"
#include "uitaskrunner.h"
#include "uidlggroup.h"

#include <math.h>

const int uiEMPartServer::evDisplayHorizon()	    { return 0; }
const int uiEMPartServer::evRemoveTreeObject()	    { return 1; }
const int uiEMPartServer::evSyncGeometry()	    { return 2; }

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }

#define mDynamicCastAll(objid) \
    EM::EMObject* object = em_.getObject(objid); \
    mDynamicCastGet(EM::Surface*,surface,object) \
    mDynamicCastGet(EM::Horizon2D*,hor2d,object) \
    mDynamicCastGet(EM::Horizon3D*,hor3d,object) \
    mDynamicCastGet(EM::Fault*,fault,object) \
    mDynamicCastGet(EM::RandomPosBody*,randpos,object) \
    mDynamicCastGet(EM::PolygonBody*,polygon,object) \
    mDynamicCastGet(EM::MarchingCubesSurface*,mcsurface,object) \


uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , selemid_(-1)
    , em_(EM::EMM())
    , disponcreation_(false)
{
    em_.syncGeomReq.notify( mCB(this,uiEMPartServer,syncGeometry) );
}


uiEMPartServer::~uiEMPartServer()
{
    em_.syncGeomReq.remove( mCB(this,uiEMPartServer,syncGeometry) );
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


void uiEMPartServer::manageSurfaces( const char* typ )
{
    uiSurfaceMan dlg( parent(), typ );
    dlg.go();
}


bool uiEMPartServer::import3DHorizon( bool isgeom )
{
    uiImportHorizon dlg( parent(), isgeom );
    const bool res = dlg.go();
    if ( res && dlg.doDisplay() )
    {
	const MultiID mid = dlg.getSelID();	
	selemid_ = em_.getObjectID(mid);
	sendEvent( evDisplayHorizon() );
    }

    return res;
}


bool uiEMPartServer::export2DHorizon()
{
    ObjectSet<SurfaceInfo> hinfos;
    getAllSurfaceInfo( hinfos, true );
    uiExport2DHorizon dlg( parent(), hinfos );
    return dlg.go();
}


bool uiEMPartServer::export3DHorizon()
{
    uiExportHorizon dlg( parent() );
    return dlg.go();
}


bool uiEMPartServer::importFault( const char* type )
{
    uiImportFault3D dlg( parent(), type );
    return dlg.go();
}


bool uiEMPartServer::exportFault( const char* type )
{
    uiExportFault dlg( parent(), type );
    return dlg.go();
}


BufferString uiEMPartServer::getName( const EM::ObjectID& id ) const
{
    return em_.objectName( em_.getMultiID(id) );
}


const char* uiEMPartServer::getType( const EM::ObjectID& emid ) const
{
    return em_.objectType( em_.getMultiID(emid) );
}


bool uiEMPartServer::isChanged( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject( emid );
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isGeometryChanged( const EM::ObjectID& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,em_.getObject(emid));
    return emsurf && emsurf->geometry().isChanged(0);
}


int uiEMPartServer::nrAttributes( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject( emid );
    if ( !emobj ) return 0;

    EM::SurfaceIOData sd;
    FixedString err = EM::EMM().getSurfaceData( emobj->multiID(), sd );
    return err.isEmpty() ? sd.valnames.size() : 0;
}


bool uiEMPartServer::isEmpty( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject( emid );
    return emobj && emobj->isEmpty();
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


bool uiEMPartServer::isShifted( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = em_.getObject( emid );
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return false;

    const float shift = hor3d->geometry().getShift();
    return !mIsZero(shift,mDefEps);
}


void uiEMPartServer::fillHoles( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid));
    if ( !hor3d )
    {
	pErrMsg("No horizon!");
	return;
    }
    uiHorizon3DInterpolDlg dlg( parent(), hor3d );
    dlg.go();
}


void uiEMPartServer::filterSurface( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid))
    uiFilterHorizonDlg dlg( parent(), hor3d );
    dlg.go();
}


void uiEMPartServer::removeTreeObject( const EM::ObjectID& emid )
{
    selemid_ = emid;
    sendEvent( evRemoveTreeObject() );
}


void uiEMPartServer::syncGeometry( CallBacker* cb )
{
    mCBCapsuleUnpack( const EM::ObjectID&, emid, cb );
    selemid_ = emid;
    sendEvent( evSyncGeometry() );
}


EM::EMObject* uiEMPartServer::selEMObject()
{ return em_.getObject( selemid_ ); }


void uiEMPartServer::deriveHor3DFrom2D( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon2D*,h2d,em_.getObject(emid))
    uiHor3DFrom2DDlg dlg( parent(), *h2d, this );

    if ( dlg.go() && dlg.doDisplay() )
    {
	RefMan<EM::Horizon3D> hor = dlg.getHor3D();
	selemid_ = hor->id();
	sendEvent( evDisplayHorizon() );
    }
}


#define mMkMsg(s) \
	BufferString msg( emobj->getTypeStr() ); \
	msg += " '"; msg += emobj->name(); msg += "' "; msg += s; \
	msg += ".\nDo you want to save it?"

void uiEMPartServer::askUserToSave( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    if ( !emobj || emobj->isEmpty() ) return;

    if ( !isChanged(emid) )
	return;
    mMkMsg( "has changed" );
    if ( !uiMSG().askSave( msg ) )
	return;

    storeObject( emid, !isFullyLoaded(emid) );
}


void uiEMPartServer::selectHorizons( TypeSet<EM::ObjectID>& ids, bool is2d )
{
    selectSurfaces( ids, is2d ? EMHorizon2DTranslatorGroup::keyword()
	    		      : EMHorizon3DTranslatorGroup::keyword() );
}


void uiEMPartServer::selectFaults( TypeSet<EM::ObjectID>& ids, bool is2d )
{
    if ( !is2d )
	selectSurfaces( ids, EMFault3DTranslatorGroup::keyword() );
}


void uiEMPartServer::selectFaultStickSets( TypeSet<EM::ObjectID>& ids )
{  selectSurfaces( ids, EMFaultStickSetTranslatorGroup::keyword() ); } 


void uiEMPartServer::selectBodies( TypeSet<EM::ObjectID>& ids )
{
    CtxtIOObj context( EMBodyTranslatorGroup::ioContext() );
    context.ctxt.forread = true;

    uiIOObjSelDlg dlg( parent(), context );
    if ( !dlg.go() )
	return;

    if ( !dlg.ioObj() )
	return;

    const IOObj* ioobj = dlg.ioObj();
    const char* translator = ioobj->translator();

    EM::EMObject* object = 0;
    if ( !strcmp( translator, polygonEMBodyTranslator::sKeyUserName() ) )
    {
	object = EM::EMM().createTempObject(EM::PolygonBody::typeStr());
    }
    else if ( !strcmp( translator, randposEMBodyTranslator::sKeyUserName() ) )
    {
	object = EM::EMM().createTempObject(EM::RandomPosBody::typeStr());
    }
    else if ( !strcmp( translator, mcEMBodyTranslator::sKeyUserName() ) )
    {
	object =EM::EMM().createTempObject(EM::MarchingCubesSurface::typeStr());
    }
    else
    {
	pErrMsg("Hmm");
	return;
    }
    
    if ( !object ) return;
    object->ref();

    object->setMultiID( dlg.ioObj()->key() );
    Executor* exec = object->loader();

    uiTaskRunner execdlg( parent() );
    if ( !execdlg.execute(*exec) )
    {
	object->unRef();
	delete exec;
	return;
    }

    delete exec;

    ids += object->id();

    object->unRefNoDelete();
}


void uiEMPartServer::selectSurfaces( TypeSet<EM::ObjectID>& objids,
				     const char* typ )
{
    uiMultiSurfaceReadDlg dlg( parent(), typ );
    if ( !dlg.go() ) return;

    TypeSet<MultiID> surfaceids;
    dlg.iogrp()->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    dlg.iogrp()->getSurfaceSelection( sel );

    PtrMan<Executor> exec = em_.objectLoader( surfaceids, &sel );
    if ( !exec ) return;

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	EM::EMObject* obj = em_.getObject( em_.getObjectID(surfaceids[idx]) );
	obj->ref();
	obj->setBurstAlert( true );
    }

    uiTaskRunner execdlg( parent() );
    if ( !execdlg.execute(*exec) )
    {
	for ( int idx=0; idx<surfaceids.size(); idx++ )
	{
	    EM::EMObject* obj =
			em_.getObject( em_.getObjectID(surfaceids[idx]) );
	    obj->unRef();
	}

	return;
    }

    exec.erase(); //We don't want executor to unref objs at end of function

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	const EM::ObjectID objid = em_.getObjectID( surfaceids[idx] );
	EM::EMObject* obj = em_.getObject( objid );
	obj->unRefNoDelete();
	obj->setBurstAlert( false );
	objids += objid;
    }
}


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
				  const TypeSet<int>& selattribs )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Surface data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiTaskRunner exdlg( parent() );
    return exdlg.execute( exgrp );
}



int uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return -1;
    
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
    return loadAuxData( id, selattribs )
	? hor3d->auxdata.auxDataIndex(attrnm) : -1;
}


bool uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    EM::SurfaceIOData sd;
    const MultiID mid = em_.getMultiID( id );
    em_.getSurfaceData( mid, sd );
    uiSelectFromList::Setup setup( "Surface data", sd.valnames );
    setup.dlgtitle( "Select one or more attributes to be displayed\n"
	    	    "on the horizon. After loading, use 'Page Up'\n"
		    "and 'Page Down' buttons to scroll.\n"
	   	    "Make sure the attribute treeitem is selected\n"
		    "and that the mouse pointer is in the scene." );
    uiSelectFromList dlg( parent(), setup );
    if ( dlg.selFld() )
	dlg.selFld()->setMultiSelect( true );
    if ( !dlg.go() || !dlg.selFld() ) return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getSelectedItems( selattribs );
    if ( selattribs.isEmpty() ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Loading surface data" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiTaskRunner exdlg( parent() );
    return exdlg.execute( exgrp );
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas ) const
{
    MultiID dummykey;
    return storeObject( id, storeas, dummykey );
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas,
				  MultiID& storagekey ) const
{
    mDynamicCastAll(id);
    if ( !object ) return false;

    PtrMan<Executor> exec = 0;
    MultiID key = object->multiID();

    if ( storeas )
    {
	if ( surface )
	{
	    uiWriteSurfaceDlg dlg( parent(), *surface );
	    if ( !dlg.go() ) return false;

	    EM::SurfaceIOData sd;
	    EM::SurfaceIODataSelection sel( sd );
	    dlg.getSelection( sel );

	    key = dlg.ioObj() ? dlg.ioObj()->key() : "";
	    exec = surface->geometry().saver( &sel, &key );
	    if ( exec && dlg.replaceInTree() )
		    surface->setMultiID( key );
	}
	else
	{
	    CtxtIOObj context( object->getIOObjContext(),
		    	       IOM().get(object->multiID()) );

	    context.ctxt.forread = false;

	    uiIOObjSelDlg dlg( parent(), context );
	    if ( !dlg.go() )
		return false;

	    if ( dlg.ioObj() )
	    {
		key = dlg.ioObj()->key();
		object->setMultiID( key );
	    }

	    exec = object->saver();
	}
    }
    else 
	exec = object->saver();

    if ( !exec )
	return false;

    storagekey = key;
    uiTaskRunner exdlg( parent() );
    return exdlg.execute( *exec );
}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id,
				   BufferString& auxdatanm, bool storeas ) const
{
    mDynamicCastAll(id);
    if ( !hor3d ) return false;

    int dataidx = -1;
    bool overwrite = false;
    if ( storeas )
    {
	uiStoreAuxData dlg( parent(), *hor3d );
	if ( !dlg.go() ) return false;

	dataidx = 0;
	overwrite = dlg.doOverWrite();
	auxdatanm = dlg.auxdataName();
    }

    PtrMan<Executor> saver = hor3d->auxdata.auxDataSaver( dataidx, overwrite );
    if ( !saver )
    {
	uiMSG().error( "Cannot save attribute" );
	return false;
    }

    uiTaskRunner exdlg( parent() );
    return exdlg.execute( *saver );
}


int uiEMPartServer::setAuxData( const EM::ObjectID& id,
				DataPointSet& data, 
				const char* attribnm, int idx, float shift )
{
    mDynamicCastAll(id);
    if ( !hor3d ) { uiMSG().error( "Cannot find horizon" ); return -1; }
    if ( !data.size() ) { uiMSG().error( "No data calculated" ); return -1; }

    const BinIDValueSet& bivs = data.bivSet();
    const int nrdatavals = bivs.nrVals();
    if ( idx>=nrdatavals ) return -1;

    BufferString auxnm;
    if ( attribnm )
	auxnm = attribnm;
    else
    {
	auxnm = "AuxData";
	auxnm += idx;
    }

    hor3d->auxdata.removeAll();
    const int auxdatanr = hor3d->auxdata.addAuxData( auxnm );
    hor3d->auxdata.setAuxDataShift( auxdatanr, shift );

    BinID bid;
    BinIDValueSet::Pos pos;
    const EM::SectionID sectionid = hor3d->sectionID( 0 );
    mAllocVarLenArr( float, vals, bivs.nrVals() );

    EM::PosID posid( id, sectionid );
    while ( bivs.next(pos) )
    {
	bivs.get( pos, bid, vals );
	RowCol rc( bid.inl, bid.crl );
	EM::SubID subid = rc.getSerialized();
	posid.setSubID( subid );
	hor3d->auxdata.setAuxDataVal( auxdatanr, posid, vals[idx] );
    }

    return auxdatanr;
}


bool uiEMPartServer::getAuxData( const EM::ObjectID& oid, int auxdatanr,
				 DataPointSet& auxdata ) const
{
    mDynamicCastAll(oid);
    if ( !hor3d || !hor3d->auxdata.auxDataName(auxdatanr) )
	return false;

    auxdata.setName( hor3d->auxdata.auxDataName( auxdatanr ) );
    auxdata.bivSet().setNrVals( 2 );
    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( idx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	    continue;

	BinID bid;
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    bid.setSerialized( pid.subID() );
	    DataPointSet::Pos newpos( bid, 0 );
	    DataPointSet::DataRow dtrow( newpos );
	    dtrow.data_ += hor3d->auxdata.getAuxDataVal( auxdatanr, pid );
	    auxdata.addRow( dtrow );
	}
    }

    auxdata.dataChanged();
    return true;
}


bool uiEMPartServer::getAllAuxData( const EM::ObjectID& oid,
				    DataPointSet& data,
				    TypeSet<float>* shifts ) const
{
    mDynamicCastAll(oid);
    if ( !hor3d ) return false;

    BufferStringSet nms;
    for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
    {
	if ( hor3d->auxdata.auxDataName(idx) )
	{
	    const char* nm = hor3d->auxdata.auxDataName( idx );
	    *shifts += hor3d->auxdata.auxDataShift( idx );
	    nms.add( nm );
	    data.dataSet().add( new DataColDef(nm) );
	}
    }

    data.bivSet().allowDuplicateBids(false);

    for ( int sidx=0; sidx<hor3d->nrSections(); sidx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( sidx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	    continue;

	mAllocVarLenArr( float, auxvals, nms.size()+1 );
	auxvals[0] = 0;
	BinID bid;
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    for ( int idx=0; idx<nms.size(); idx++ )
	    {
		const int auxidx = hor3d->auxdata.auxDataIndex( nms.get(idx) );
		auxvals[idx+1] = hor3d->auxdata.getAuxDataVal( auxidx, pid );
	    }
	    bid.setSerialized( pid.subID() );
	    data.bivSet().add( bid, auxvals );
	}
    }
    data.dataChanged();
    
    return true;
}


BinIDValueSet* uiEMPartServer::interpolateAuxData( const EM::ObjectID& oid,
						   const char* nm )
{ return changeAuxData( oid, nm, true ); }

BinIDValueSet* uiEMPartServer::filterAuxData( const EM::ObjectID& oid,
					      const char* nm )
{ return changeAuxData( oid, nm, false ); }


BinIDValueSet* uiEMPartServer::changeAuxData( const EM::ObjectID& oid,
					      const char* nm,
					      bool interpolate )
{
    mDynamicCastAll(oid);
    if ( !hor3d ) return 0;

    const int auxidx = hor3d->auxdata.auxDataIndex( nm );
    if ( auxidx < 0 )
    {
	uiMSG().error("Cannot find attribute data");
	return 0;
    }

    const EM::SectionID sid = hor3d->sectionID( 0 );
    const StepInterval<int> rowrg = hor3d->geometry().rowRange( sid );
    const StepInterval<int> colrg = hor3d->geometry().colRange( sid );

    PtrMan< Array2D<float> > arr2d =
	hor3d->auxdata.createArray2D( auxidx, sid );

    PtrMan<Task> changer;
    uiTaskRunner execdlg( parent() );
    if ( interpolate )
    {
	uiSingleGroupDlg dlg( parent(),
		uiDialog::Setup::Setup( "Interpolate horizon data",
					"Interpolation parameters",
					(const char*) 0 ) );

	uiArray2DInterpolSel* settings =
	    new uiArray2DInterpolSel( &dlg, false, false );

	dlg.setGroup( settings );
	if ( !dlg.go() ) return 0;

	Array2DInterpol* interp = settings->getResult();
	if ( !interp )
	    return 0;

	changer = interp;

	interp->setFillType( Array2DInterpol::Full );

	const float inldist = SI().inlDistance();
	const float crldist = SI().crlDistance();

	interp->setRowStep( rowrg.step*inldist );
	interp->setColStep( colrg.step*crldist );


	PtrMan< Array2D<float> > arr = hor3d->createArray2D( sid );
	const float* arrptr = arr ? arr->getData() : 0;
	if ( arrptr )
	{
	    Array2D<bool>* mask = new Array2DImpl<bool>( arr->info() );
	    bool* maskptr = mask->getData();
	    if ( maskptr )
	    {
		for ( int idx=mask->info().getTotalSz()-1; idx>=0; idx-- )
		{
		    *maskptr = !mIsUdf(*arrptr);
		    maskptr++;
		    arrptr++;
		}
	    }

	    interp->setMask( mask, OD::TakeOverPtr );
	}

	if ( !interp->setArray( *arr2d, &execdlg ) )
	    return 0;
    }
    else
    {
	uiArr2DFilterParsDlg dlg( parent() );
	if ( !dlg.go() ) return 0;

	Array2DFilterer<float>* filter =
	    new Array2DFilterer<float>( *arr2d, dlg.getInput() );
	changer = filter;
    }

    if ( !execdlg.execute(*changer) )
	return 0;

    hor3d->auxdata.setArray2D( auxidx, sid, *arr2d );
    BinIDValueSet* res = new BinIDValueSet( 2, false );
    for ( int row=rowrg.start; row<=rowrg.stop; row+=rowrg.step )
    {
	for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
	{
	    res->add( BinID(row,col), 0, arr2d->get(rowrg.getIndex(row),
						    colrg.getIndex(col)) );
	}
    }

    mDynamicCastGet(const Array2DInterpol*,interp,changer.ptr())
    const char* infomsg = interp ? interp->infoMsg() : 0;
    if ( infomsg && *infomsg )
	uiMSG().message( infomsg );

    return res;
}


void uiEMPartServer::removeUndo()
{
    em_.undo().removeAll();
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
    uiTaskRunner exdlg( parent() );
    if ( exdlg.execute(*exec) <= 0 )
    {
	obj->unRef();
	return false;
    }

    delete exec;
    obj->unRefNoDelete();
    return true;
}


const char* uiEMPartServer::genRandLine( int opt )
{
    const char* res = 0;
    if ( opt == 0 )
    {
	uiGenRanLinesByShift dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else if ( opt == 1 )
    {
	uiGenRanLinesByContour dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else
    {
	uiGenRanLineFromPolygon dlg( parent() );
	if ( dlg.go() )
	{
	    res = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    return res;
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos )
{
    for ( int idx=0; idx<em_.nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(em_.objectID(idx)));
	if ( hor3d ) hinfos += new SurfaceInfo( hor3d->name(), hor3d->multiID() );
    }
}


void uiEMPartServer::getAllSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos,
					bool is2d )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    const char* groupstr = is2d ? "2D Horizon" : "Horizon";
    ObjectSet<IOObj> ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( strcmp(ioobj->translator(),"dGB") ) continue;
	if ( !strcmp(ioobj->group(), groupstr ) )
	    hinfos += new SurfaceInfo( ioobj->name(), ioobj->key() );
    }
}


void uiEMPartServer::getSurfaceDef3D( const TypeSet<EM::ObjectID>& selhorids,
				    BinIDValueSet& bivs,
				    const HorSampling& hs ) const
{
    bivs.empty(); bivs.setNrVals( 2, false );

    const EM::ObjectID& id = selhorids[0]; 
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(id))
    if ( !hor3d ) return;
    hor3d->ref();

    EM::Horizon3D* hor3d2 = 0;
    if ( selhorids.size() > 1 )
    {
	hor3d2 = (EM::Horizon3D*)(em_.getObject(selhorids[1]));
	hor3d2->ref();
    }

    BinID bid;
    for ( bid.inl=hs.start.inl; bid.inl<=hs.stop.inl; bid.inl+=hs.step.inl )
    {
	for ( bid.crl=hs.start.crl; bid.crl<=hs.stop.crl; bid.crl+=hs.step.crl )
	{
	    const EM::SubID subid = bid.getSerialized();
	    TypeSet<Coord3> z1pos, z2pos;
	    for ( int idx=hor3d->nrSections()-1; idx>=0; idx-- )
	    {
		const EM::SectionID sid = hor3d->sectionID( idx );
		if ( hor3d->isDefined( sid, subid ) )
		    z1pos += hor3d->getPos( sid, subid );
	    }

	    if ( z1pos.isEmpty() ) continue;

	    if ( !hor3d2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		    bivs.add( bid, z1pos[posidx].z, z1pos[posidx].z );
	    }
	    else
	    {
		for ( int idx=hor3d2->nrSections()-1; idx>=0; idx-- )
		{
		    const EM::SectionID sid = hor3d2->sectionID( idx );
		    if ( hor3d2->isDefined( sid, subid ) )
			z2pos += hor3d2->getPos( sid, subid );
		}

		if ( z2pos.isEmpty() ) continue;

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
    
    hor3d->unRef();
    if ( hor3d2 ) hor3d2->unRef();
}


#define mGetObjId( num, id ) \
{ \
    MultiID horid = *selhorids[num]; \
    id = getObjectID(horid); \
    if ( id<0 || !isFullyLoaded(id) ) \
	loadSurface( horid ); \
    id = getObjectID(horid); \
}
void uiEMPartServer::getSurfaceDef2D( const ObjectSet<MultiID>& selhorids,
				  const ObjectSet<PosInfo::Line2DData>& geoms,
				  BufferStringSet& selectlines,
				  TypeSet<Coord>& coords,
				  TypeSet< Interval<float> >& zrgs )
{
    if ( !selhorids.size() ) return;

    EM::ObjectID id;
    const bool issecondhor = selhorids.size()>1;
    mGetObjId(0,id);
    mDynamicCastGet(EM::Horizon2D*,hor2d1,em_.getObject(id));
    
    EM::Horizon2D* hor2d2 = 0;
    if ( issecondhor )
    {
	mGetObjId(1,id);
	mDynamicCastGet(EM::Horizon2D*,hor,em_.getObject(id));
	hor2d2 = hor;
    }

    if ( !hor2d1 || ( issecondhor && !hor2d2 ) ) return;

    for ( int lidx=0; lidx<selectlines.size(); lidx++ )
    {
	const PosInfo::Line2DData* ld = geoms[lidx];
	if ( !ld ) continue;

	BufferString lnm = *selectlines[lidx];
	int lineidx = hor2d1->geometry().lineIndex( lnm );
	const int lineid1 = hor2d1->geometry().lineID( lineidx );

	int lineid2 = -1;
	if ( issecondhor )
	{
	    lineidx = hor2d2->geometry().lineIndex( lnm );
	    lineid2 = hor2d2->geometry().lineID( lineidx );
	}

	if ( lineid1<0 || ( issecondhor && lineid2<0 ) ) continue;

	for ( int trcidx=0; trcidx<ld->posns_.size(); trcidx++ )
	{
	    const EM::SubID subid1 = 
		RowCol( lineid1, ld->posns_[trcidx].nr_ ).getSerialized();
	    const float z1 = hor2d1->getPos(0,subid1).z;
	    float z2 = mUdf(float);
	    if ( issecondhor )
	    {
		const EM::SubID subid2 =
		    RowCol( lineid2, ld->posns_[trcidx].nr_ ).getSerialized();
		z2 = hor2d2->getPos(0,subid2).z;
	    }

	    if ( !mIsUdf(z1) && ( !issecondhor || !mIsUdf(z2) ) )
	    {
		Interval<float> zrg( z1, issecondhor ? z2 : z1 );	
		zrgs += zrg;
		coords += ld->posns_[trcidx].coord_;
	    }
	}
    }
}


void uiEMPartServer::fillPickSet( Pick::Set& ps, MultiID horid )
{
    EM::ObjectID id = getObjectID( horid );
    if ( id<0 || !isFullyLoaded(id) )
	loadSurface( horid );

    id = getObjectID( horid );
    if ( id < 0 )
	return;

    EM::EMObject* obj = em_.getObject( id );
    mDynamicCastGet(EM::Horizon3D*,hor,obj)
    int idx = 0;
    while ( idx < ps.size() )
    {
	const Coord pos( ps[idx].pos.x, ps[idx].pos.y );
	const BinID bid = SI().transform( pos );
	const EM::SubID subid = bid.getSerialized();
	float z = hor->getPos( hor->sectionID(0), subid ).z;
	if ( mIsUdf(z) )
	{
	    const Geometry::BinIDSurface* geom = 
		hor->geometry().sectionGeometry( hor->sectionID(0) );
	    if ( geom ) z = geom->computePosition( Coord(bid.inl,bid.crl) ).z;
	    if ( mIsUdf(z) )
	    {
		ps.remove( idx );
		continue;
	    }
	}

	ps[idx].pos.z = z;
	idx++;
    }
}

