/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiempartserv.h"

#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "embodytr.h"
#include "emrandomposbody.h"
#include "emposid.h"
#include "empolygonbody.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfaceio.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "parametricsurface.h"
#include "pickset.h"
#include "posinfo2d.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "undo.h"
#include "varlenarray.h"
#include "variogramcomputers.h"

#include "uiarray2dchg.h"
#include "uiarray2dinterpol.h"
#include "uibulkhorizonimp.h"
#include "uichangesurfacedlg.h"
#include "uiempreloaddlg.h"
#include "uiexpfault.h"
#include "uiexphorizon.h"
#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uihor3dfrom2ddlg.h"
#include "uihorgeom2attr.h"
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
#include "uivariogram.h"
#include "uidlggroup.h"
#include "uihorsavefieldgrp.h"

#include <math.h>


int uiEMPartServer::evDisplayHorizon()	    { return 0; }
int uiEMPartServer::evRemoveTreeObject()	    { return 1; }

#define mErrRet(s) { BufferString msg( "Cannot load '" ); msg += s; msg += "'";\
    			uiMSG().error( msg ); return false; }


uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , selemid_(-1)
    , em_(EM::EMM())
    , disponcreation_(false)
    , selectedrg_(false)
    , imphorattrdlg_(0)
    , imphorgeomdlg_(0)
    , impfltdlg_(0)
    , exphordlg_(0)
    , expfltdlg_(0)
    , expfltstickdlg_(0)
    , impfltstickdlg_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiEMPartServer,survChangedCB) );
}


uiEMPartServer::~uiEMPartServer()
{
    em_.setEmpty();
    deepErase( variodlgs_ );
}


void uiEMPartServer::survChangedCB( CallBacker* )
{
    delete imphorattrdlg_; imphorattrdlg_ = 0;
    delete imphorgeomdlg_; imphorgeomdlg_ = 0;
    delete impfltdlg_; impfltdlg_ = 0;
    delete exphordlg_; exphordlg_ = 0;
    delete expfltdlg_; expfltdlg_ = 0;
    deepErase( variodlgs_ );
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


bool uiEMPartServer::import3DHorAttr()
{
    if ( imphorattrdlg_ )
	imphorattrdlg_->raise();
    else
    {
	imphorattrdlg_ = new uiImportHorizon( parent(), false );
	imphorattrdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB));
    }

    return imphorattrdlg_->go();
}


bool uiEMPartServer::import3DHorGeom( bool bulk )
{
    if ( bulk )
    {
	uiBulkHorizonImport dlg( parent() );
	return dlg.go();
    }

    if ( imphorgeomdlg_ )
	imphorgeomdlg_->raise();
    else
    {
	imphorgeomdlg_ = new uiImportHorizon( parent(), true );
	imphorgeomdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB));
    }

    return imphorgeomdlg_->go();
}


void uiEMPartServer::importReadyCB( CallBacker* cb )
{
    mDynamicCastGet(uiImportHorizon*,dlg,cb)
    mDynamicCastGet(uiImportFault3D*,fltdlg,cb)
    if ( (!dlg || !dlg->doDisplay()) &&
	 (!fltdlg || !fltdlg->saveButtonChecked()) )
	return;

    selemid_ = em_.getObjectID( dlg ? dlg->getSelID() : fltdlg->getSelID() );
    sendEvent( evDisplayHorizon() );
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
    if ( exphordlg_ )
	exphordlg_->raise();
    else
	exphordlg_ = new uiExportHorizon( parent() );

    return exphordlg_->go();
}


bool uiEMPartServer::importFault()
{
    if ( impfltdlg_ )
	impfltdlg_->raise();
    else
    {
	impfltdlg_ =
	    new uiImportFault3D( parent(), EMFault3DTranslatorGroup::keyword());
	impfltdlg_->importReady.notify( mCB(this,uiEMPartServer,importReadyCB));
    }

    return impfltdlg_->go();
}


bool uiEMPartServer::importFaultStickSet()
{
    if ( impfltstickdlg_ )
	impfltstickdlg_->raise();
    else
    {
	impfltstickdlg_ =
	    new uiImportFault3D( parent(),
		    		 EMFaultStickSetTranslatorGroup::keyword() );
	impfltstickdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB));
    }

    return impfltstickdlg_->go();
}


bool uiEMPartServer::exportFault()
{
    if ( expfltdlg_ )
	expfltdlg_->raise();
    else
	expfltdlg_ = new uiExportFault( parent(),
					EMFault3DTranslatorGroup::keyword() );
    return expfltdlg_->go();
} 


bool uiEMPartServer::exportFaultStickSet()
{
    if ( expfltstickdlg_ )
	expfltstickdlg_->raise();
    else
	expfltstickdlg_ =
	    new uiExportFault( parent(),
		    	       EMFaultStickSetTranslatorGroup::keyword() );
    return expfltstickdlg_->go();
} 



BufferString uiEMPartServer::getName( const EM::ObjectID& id ) const
{
    if ( !em_.getObject( id )->name().isEmpty() )
	return em_.getObject( id )->name();
    
    BufferString bs;
    return bs;
}


const char* uiEMPartServer::getType( const EM::ObjectID& emid ) const
{
    return em_.getObject( emid )->getTypeStr();
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

    EM::IOObjInfo eminfo( emobj->multiID() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    return eminfo.isOK() ? attrnms.size() : 0;
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


void uiEMPartServer::displayEMObject( const MultiID& mid )
{
    selemid_ = em_.getObjectID(mid);
    if ( selemid_<0 )
    {
	loadSurface( mid );
	selemid_ = em_.getObjectID(mid);
    }

    if ( selemid_>=0 )
    	sendEvent( evDisplayHorizon() );
}


bool uiEMPartServer::fillHoles( const EM::ObjectID& emid, bool is2d )
{
    mDynamicCastGet(EM::Horizon*,hor,em_.getObject(emid));
    uiHorizonInterpolDlg dlg( parent(), hor, is2d );
    dlg.go();

    if ( dlg.saveFldGrp()->displayNewHorizon() && 
	 dlg.saveFldGrp()->getNewHorizon( ) ) 
	displayEMObject( dlg.saveFldGrp()->getNewHorizon()->multiID() ); 

    return dlg.saveFldGrp()->overwriteHorizon();
}


bool uiEMPartServer::filterSurface( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,em_.getObject(emid))
    uiFilterHorizonDlg dlg( parent(), hor3d );
    dlg.go();

    if ( dlg.saveFldGrp()->displayNewHorizon() &&  
   	 dlg.saveFldGrp()->getNewHorizon( ) )
	displayEMObject( dlg.saveFldGrp()->getNewHorizon()->multiID() );

    return dlg.saveFldGrp()->overwriteHorizon();
}


void uiEMPartServer::removeTreeObject( const EM::ObjectID& emid )
{
    selemid_ = emid;
    sendEvent( evRemoveTreeObject() );
}


void uiEMPartServer::removeUnsavedEMObjectFromTree()
{
    for ( int idx=em_.nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID objid = em_.objectID( idx );
	
	if ( em_.getMultiID(objid).isEmpty() )
	{ removeTreeObject( objid ); return; }
    }
}


EM::ObjectID uiEMPartServer::saveUnsavedEMObject()
{
    for ( int idx=em_.nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID objid = em_.objectID( idx );

	if ( em_.getMultiID(objid).isEmpty() )
	{
	    storeObject( objid, true );
	    return objid;
	}
    }

    return -1;
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

bool uiEMPartServer::askUserToSave( const EM::ObjectID& emid,
				    bool withcancel ) const
{
    EM::EMObject* emobj = em_.getObject(emid);
    if ( !emobj || !emobj->isChanged() )
	return true;

    PtrMan<IOObj> ioobj = IOM().get( getStorageID(emid) );
    if ( !ioobj && emobj->isEmpty() )
	return true;

    mMkMsg( "has changed" );
    const int ret = uiMSG().askSave( msg, withcancel );
    if ( ret == 1 )
    {
	const bool saveas = !ioobj || !isFullyLoaded(emid);
	return storeObject( emid, saveas );
    }

    return ret == 0;
}


void uiEMPartServer::selectHorizons( ObjectSet<EM::EMObject>& objs, bool is2d )
{
    selectSurfaces( objs, is2d ? EMHorizon2DTranslatorGroup::keyword()
	    		      : EMHorizon3DTranslatorGroup::keyword() );
}


void uiEMPartServer::selectFaults( ObjectSet<EM::EMObject>& objs, bool is2d )
{
    if ( !is2d )
	selectSurfaces( objs, EMFault3DTranslatorGroup::keyword() );
}


void uiEMPartServer::selectFaultStickSets( ObjectSet<EM::EMObject>& objs )
{  selectSurfaces( objs, EMFaultStickSetTranslatorGroup::keyword() ); } 


void uiEMPartServer::selectBodies( ObjectSet<EM::EMObject>& objs )
{
    CtxtIOObj context( EMBodyTranslatorGroup::ioContext() );
    context.ctxt.forread = true;

    uiIOObjSelDlg dlg( parent(), context, 0, true );
    if ( !dlg.go() )
	return;

    TypeSet<MultiID> mids;
    dlg.selGrp()->getSelected( mids );
    if ( mids.isEmpty() )
	return;

    ExecutorGroup loaders( "Loading Bodies" );
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mids[idx] );
	const BufferString& translator = ioobj->translator();

	BufferString typestr;
	if ( translator==polygonEMBodyTranslator::sKeyUserName() )
	    typestr = EM::PolygonBody::typeStr();
	else if ( translator==randposEMBodyTranslator::sKeyUserName() ) 
	    typestr = EM::RandomPosBody::typeStr();
	else if ( translator==mcEMBodyTranslator::sKeyUserName() ||
		  translator==dGBEMBodyTranslator::sKeyUserName() )
	    typestr = EM::MarchingCubesSurface::typeStr();
	else
	    continue;
    
	EM::EMObject* object = EM::EMM().createTempObject( typestr );
	if ( !object ) continue;

	object->ref();
	object->setMultiID( mids[idx] );
	objs += object;
	loaders.add( object->loader() );
    }

    uiTaskRunner execdlg( parent() );
    if ( !TaskRunner::execute( &execdlg, loaders ) )
    {
	deepUnRef( objs );
	return;
    }
}


void uiEMPartServer::selectSurfaces( ObjectSet<EM::EMObject>& objs,
				     const char* typ )
{
    uiMultiSurfaceReadDlg dlg( parent(), typ );
    if ( !dlg.go() ) return;

    TypeSet<MultiID> surfaceids;
    dlg.iogrp()->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd ), orisel( sd );
    dlg.iogrp()->getSurfaceSelection( sel );

    const bool hor3d = typ==EMHorizon3DTranslatorGroup::keyword();

    if ( hor3d )
      	selectedrg_ = sel.rg; 

    PtrMan<Executor> exec = em_.objectLoader(surfaceids,hor3d ? &sel : &orisel);
    if ( !exec )
    {
	bool allmissing = true;
	for ( int idx=0; idx<surfaceids.size(); idx++ )
	{
	    if ( em_.getObject(em_.getObjectID(surfaceids[idx])) )
	    {
		allmissing = false;
		break;
	    }
        }
	
	if ( allmissing )
	    return;
    }    

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	EM::EMObject* obj = em_.getObject( em_.getObjectID(surfaceids[idx]) );
	if ( !obj ) continue;
	obj->ref();
	obj->setBurstAlert( true );
	objs += obj;
    }

    if ( exec )
    {
    	uiTaskRunner execdlg( parent() );
    	if ( !TaskRunner::execute( &execdlg, *exec ) )
    	    deepUnRef( objs );
    }

    for ( int idx=0; idx<objs.size(); idx++ )
	objs[idx]->setBurstAlert( false );
}


void uiEMPartServer::setHorizon3DDisplayRange( const HorSampling& hs )
{ selectedrg_ = hs; }


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
			    const TypeSet<int>& selattribs, bool removeold )
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    if ( removeold )
	hor3d->auxdata.removeAll();

    ExecutorGroup exgrp( "Horizon Data loader" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiTaskRunner exdlg( parent() );
    return TaskRunner::execute( &exdlg, exgrp );
}



int uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm,
				 bool removeold )
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return -1;
    
    const MultiID mid = em_.getMultiID( id );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const int nritems = attrnms.size();
    int selidx = -1;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const BufferString& nm = attrnms.get(idx);
	if ( nm == attrnm )
	{ selidx= idx; break; }
    }

    if ( selidx<0 ) return -1;
    TypeSet<int> selattribs( 1, selidx );
    return loadAuxData( id, selattribs, removeold )
	? hor3d->auxdata.auxDataIndex(attrnm) : -1;
}


bool uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    const MultiID mid = em_.getMultiID( id );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet atrrnms;
    eminfo.getAttribNames( atrrnms );
    uiSelectFromList::Setup setup( "Horizon Data", atrrnms );
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
    ExecutorGroup exgrp( "Loading Horizon Data" );
    exgrp.setNrDoneText( "Nr done" );
    for ( int idx=0; idx<selattribs.size(); idx++ )
	exgrp.add( hor3d->auxdata.auxDataLoader(selattribs[idx]) );

    uiTaskRunner exdlg( parent() );
    return TaskRunner::execute( &exdlg, exgrp );
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas ) const
{
    MultiID dummykey;
    return storeObject( id, storeas, dummykey );
}


bool uiEMPartServer::storeObject( const EM::ObjectID& id, bool storeas,
				  MultiID& storagekey,
       				  float shift ) const
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Surface*, surface, object );
    if ( !object ) return false;

    PtrMan<Executor> exec = 0;
    MultiID key = object->multiID();

    if ( storeas )
    {
	if ( surface )
	{
	    uiWriteSurfaceDlg dlg( parent(), *surface, shift );
	    if ( !dlg.go() ) return false;

	    EM::SurfaceIOData sd;
	    EM::SurfaceIODataSelection sel( sd );
	    dlg.getSelection( sel );

	    key = dlg.ioObj() ? dlg.ioObj()->key() : "";
	    exec = surface->geometry().saver( &sel, &key );
	    if ( exec && dlg.replaceInTree() )
		    surface->setMultiID( key );

	    mDynamicCastGet( EM::dgbSurfaceWriter*, writer, exec.ptr() );
	    if ( writer ) writer->setShift( shift );
	}
	else
	{
	    CtxtIOObj context( object->getIOObjContext(),
		    	       IOM().get(object->multiID()) );

	    context.ctxt.forread = false;

	    uiIOObjSelDlg dlg( parent(), context );
	    if ( !context.ioobj )
		dlg.selGrp()->getNameField()->setText( object->name() );
	     
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

    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj->pars().find( sKey::Type() ) )
    {
	ioobj->pars().set( sKey::Type(), object->getTypeStr() );
	if ( !IOM().commitChanges( *ioobj ) )
	{
	    uiMSG().error( "Could not write to database" );
	    return false;
	}
    }

    storagekey = key;
    uiTaskRunner exdlg( parent() );
    return TaskRunner::execute( &exdlg, *exec );
}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id,
				   BufferString& auxdatanm, bool storeas ) const
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    mDynamicCastGet( EM::Fault3D*, flt3d, object );
    if ( !hor3d && !flt3d )
	return false;
    
    uiTaskRunner exdlg( parent() );
    if ( hor3d )
    {
	int dataidx = -1;
	bool overwrite = false;
	if ( storeas )
	{
	    if ( hor3d )
	    {
		uiStoreAuxData dlg( parent(), *hor3d );
		if ( !dlg.go() ) return false;

		dataidx = 0;
		overwrite = dlg.doOverWrite();
		auxdatanm = dlg.auxdataName();
	    }
	}

	PtrMan<Executor> saver = hor3d->auxdata.auxDataSaver(dataidx,overwrite);
	if ( !saver )
	{
	    uiMSG().error( "Cannot save attribute" );
	    return false;
	}

	return TaskRunner::execute( &exdlg, *saver );
    }
    else if ( flt3d )
    {
	uiStoreFaultData dlg( parent(), *flt3d );
	if ( !dlg.go() ) return false;

	PtrMan<Executor> saver = dlg.dataSaver();
	if ( !saver )
	{
	    uiMSG().error( "Cannot save attribute" );
    	    return false;
	}

	return TaskRunner::execute( &exdlg, *saver );
    }

    return true;
}


int uiEMPartServer::setAuxData( const EM::ObjectID& id,
				DataPointSet& data, 
				const char* attribnm, int idx, float shift )
{
    EM::EMObject* object = em_.getObject( id ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
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

    EM::PosID posid( id, sectionid );
    while ( bivs.next(pos) )
    {
	bid = bivs.getBinID( pos );
	const float* vals = bivs.getVals( pos );

	RowCol rc( bid.inl, bid.crl );
	EM::SubID subid = rc.toInt64();
	posid.setSubID( subid );
	hor3d->auxdata.setAuxDataVal( auxdatanr, posid, vals[idx] );
    }

    return auxdatanr;
}


bool uiEMPartServer::getAuxData( const EM::ObjectID& oid, int auxdatanr,
				 DataPointSet& data, float& shift ) const
{
    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    const char* nm = hor3d->auxdata.auxDataName( auxdatanr );
    if ( !hor3d || !nm )
	return false;

    shift = hor3d->auxdata.auxDataShift( auxdatanr );
    data.dataSet().add( new DataColDef(sKeySectionID()) );
    data.dataSet().add( new DataColDef(nm) );

    float auxvals[3];
    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( idx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	    continue;

	auxvals[1] = sid;

	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    auxvals[0] = (float) hor3d->getPos( pid ).z;
	    auxvals[2] = hor3d->auxdata.getAuxDataVal( auxdatanr, pid );
	    BinID bid = BinID::fromInt64( pid.subID() );
	    data.bivSet().add( bid, auxvals );
	}
    }

    data.dataChanged();
    return true;
}


bool uiEMPartServer::getAllAuxData( const EM::ObjectID& oid,
	DataPointSet& data, TypeSet<float>* shifts, 
	const CubeSampling* cs ) const
{
    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    data.dataSet().add( new DataColDef(sKeySectionID()) );

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
    mAllocVarLenArr( float, auxvals, nms.size()+2 );
    for ( int sidx=0; sidx<hor3d->nrSections(); sidx++ )
    {
	const EM::SectionID sid = hor3d->sectionID( sidx );
	if ( !hor3d->geometry().sectionGeometry(sid) )
	    continue;

	auxvals[0] = 0;
	auxvals[1] = sid;
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator(sid,cs);
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;
	    
	    BinID bid = BinID::fromInt64( pid.subID() );
	    if ( cs )
	    {
    		if ( !cs->hrg.includes(bid) )
    		    continue;
    
		BinID diff = bid - cs->hrg.start;
		if ( diff.inl % cs->hrg.step.inl ||
		     diff.crl % cs->hrg.step.crl )	
    		    continue;
	    }

	    auxvals[0] = (float) hor3d->getPos( pid ).z;
	    for ( int idx=0; idx<nms.size(); idx++ )
	    {
		const int auxidx = hor3d->auxdata.auxDataIndex( nms.get(idx) );
		auxvals[idx+2] = hor3d->auxdata.getAuxDataVal( auxidx, pid );
	    }
	    data.bivSet().add( bid, mVarLenArr(auxvals) );
	}
    }

    data.dataChanged();
    hor3d->auxdata.removeAll();
    return true;
}


bool uiEMPartServer::interpolateAuxData( const EM::ObjectID& oid,
	const char* nm, DataPointSet& dpset )
{ return changeAuxData( oid, nm, true, dpset ); }


bool uiEMPartServer::filterAuxData( const EM::ObjectID& oid,
	const char* nm, DataPointSet& dpset )
{ return changeAuxData( oid, nm, false, dpset ); }


static int getColID( const DataPointSet& dps, const char* nm )
{
    const BinIDValueSet& bivs = dps.bivSet();
    int cid = -1;
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
    {
	BufferString colnm = dps.dataSet().colDef(idx).name_; 
	if ( colnm.isEqual(nm) )
	    { cid = idx; break; }
    }
    return cid;
}


bool uiEMPartServer::computeVariogramAuxData( const EM::ObjectID& oid,
					      const char* nm,
					      DataPointSet& dpset )
{
    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid < 0 || mIsUdf(cid) )
	return false;

    const EM::SectionID sid = hor3d->sectionID( 0 );
    const StepInterval<int> rowrg = hor3d->geometry().rowRange( sid );
    const StepInterval<int> colrg = hor3d->geometry().colRange( sid, -1 );
    BinID step( rowrg.step, colrg.step );
//    BIDValSetArrAdapter adapter( bivs, cid, step );

    uiVariogramDlg varsettings( parent(), false );
    if ( !varsettings.go() )
	return false;

    int size = varsettings.getMaxRg();
    size /= SI().inlDistance() <= SI().crlDistance() ?
	int(SI().inlDistance()) : int(SI().crlDistance());
    size++;

    BufferString errmsg;
    bool msgiserror = true;

    HorVariogramComputer hvc( dpset, size, cid, varsettings.getMaxRg(),
	    		      varsettings.getFold(), errmsg, msgiserror );
    if ( !hvc.isOK() )
    {
	msgiserror ? uiMSG().error( errmsg.buf() )
	    	   : uiMSG().warning( errmsg.buf() );
	return false;
    }
    uiVariogramDisplay* uivv = new uiVariogramDisplay( parent(), hvc.getData(),
	    					       hvc.getXaxes(),
	    					       hvc.getLabels(),
	   					       varsettings.getMaxRg(),
						       true );
    variodlgs_ += uivv;
    uivv->draw();
    uivv->go();
    return true;
}


bool uiEMPartServer::changeAuxData( const EM::ObjectID& oid,
	const char* nm, bool interpolate, DataPointSet& dpset )
{
    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    const EM::SectionID sid = hor3d->sectionID( 0 );
    const StepInterval<int> rowrg = hor3d->geometry().rowRange( sid );
    const StepInterval<int> colrg = hor3d->geometry().colRange( sid, -1 );
    BinID step( rowrg.step, colrg.step );
    BIDValSetArrAdapter adapter( bivs, cid, step );

    PtrMan<Task> changer;
    uiTaskRunner execdlg( parent() );
    if ( interpolate )
    {
	uiSingleGroupDlg dlg( parent(),
		uiDialog::Setup( "Interpolate horizon Data",
				 "Interpolation parameters",
				  (const char*) 0 ) );

	uiArray2DInterpolSel* settings =
	    new uiArray2DInterpolSel( &dlg, false, false, true, 0 );

	dlg.setGroup( settings );
	if ( !dlg.go() ) return false;

	Array2DInterpol* interp = settings->getResult();
	if ( !interp )
	    return false;

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
		for ( int idx=mCast(int,mask->info().getTotalSz()-1); idx>=0; 
									idx-- )
		{
		    *maskptr = !mIsUdf(*arrptr);
		    maskptr++;
		    arrptr++;
		}
	    }

	    interp->setMask( mask, OD::TakeOverPtr );
	}

	if ( !interp->setArray( adapter, &execdlg ) )
	    return false;
    }
    else
    {
	uiArr2DFilterParsDlg dlg( parent() );
	if ( !dlg.go() ) return false;

	Array2DFilterer<float>* filter =
	    new Array2DFilterer<float>( adapter, dlg.getInput() );
	changer = filter;
    }

    if ( !TaskRunner::execute( &execdlg, *changer ) )
	return false;

    mDynamicCastGet(const Array2DInterpol*,interp,changer.ptr())
    const char* infomsg = interp ? interp->infoMsg() : 0;
    if ( infomsg && *infomsg )
	uiMSG().message( infomsg );

    return true;
}


bool uiEMPartServer::attr2Geom( const EM::ObjectID& oid,
	const char* nm, const DataPointSet& dpset )
{
    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorAttr2Geom dlg( parent(), *hor3d, dpset, cid );
    return dlg.go();
}


bool uiEMPartServer::geom2Attr( const EM::ObjectID& oid )
{
    EM::EMObject* object = em_.getObject( oid ); 
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorGeom2Attr dlg( parent(), *hor3d );
    return dlg.go();
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
    if ( !TaskRunner::execute( &exdlg, *exec ) )
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
	if ( hor3d )
	    hinfos += new SurfaceInfo( hor3d->name(), hor3d->multiID() );
    }
}


void uiEMPartServer::getAllSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos,
					bool is2d )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    FixedString groupstr = is2d
    	? EMHorizon2DTranslatorGroup::keyword()
        : EMHorizon3DTranslatorGroup::keyword();
    ObjectSet<IOObj> ioobjs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( strcmp(ioobj->translator(),"dGB") ) continue;
	if ( groupstr==ioobj->group() )
	    hinfos += new SurfaceInfo( ioobj->name(), ioobj->key() );
    }
}


void uiEMPartServer::getSurfaceDef3D( const TypeSet<EM::ObjectID>& selhorids,
				    BinIDValueSet& bivs,
				    const HorSampling& hs ) const
{
    bivs.setEmpty(); bivs.setNrVals( 2, false );

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
	    const EM::SubID subid = bid.toInt64();
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
		    bivs.add( bid, (float) z1pos[posidx].z, (float) z1pos[posidx].z );
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
			float dist_ = (float) ( z2pos[z2idx].z - z1pos[z1idx].z );
			if ( fabs(dist_) < dist )
			{
			    zintv.start = (float) z1pos[z1idx].z;
			    zintv.stop = (float) z2pos[z2idx].z;
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
				  const MultiID& linesetid,
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

	PtrMan<IOObj> ioobj = IOM().get( linesetid );
	if ( !ioobj ) return;

	PosInfo::GeomID geomid =
	    S2DPOS().getGeomID( ioobj->name(), selectlines.get(lidx).buf() );
	
	int lineidx = hor2d1->geometry().lineIndex( geomid );
	if ( lineidx<0 ) continue;

	const TypeSet<PosInfo::Line2DPos>& posns = ld->positions();
	for ( int trcidx=0; trcidx<posns.size(); trcidx++ )
	{
	    const PosInfo::Line2DPos& l2dp = posns[trcidx];
	    const float z1 = (float) hor2d1->getPos( 0, geomid, l2dp.nr_ ).z;
	    float z2 = mUdf(float);
	 
	    if ( issecondhor )
		z2 = (float) hor2d2->getPos( 0, geomid, l2dp.nr_ ).z;

	    if ( !mIsUdf(z1) && ( !issecondhor || !mIsUdf(z2) ) )
	    {
		Interval<float> zrg( z1, issecondhor ? z2 : z1 );	
		zrgs += zrg;
		coords += l2dp.coord_;
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
    if ( !hor )
	return;

    hor->ref();
    int idx = 0;
    while ( idx < ps.size() )
    {
	const BinID bid = SI().transform( ps[idx].pos_ );
	const EM::SubID subid = bid.toInt64();
	double zval = hor->getPos( hor->sectionID(0), subid ).z;
	if ( mIsUdf(zval) )
	{
	    const Geometry::BinIDSurface* geom = 
		hor->geometry().sectionGeometry( hor->sectionID(0) );
	    if ( geom )
		zval = geom->computePosition( Coord(bid.inl,bid.crl) ).z;
	    if ( mIsUdf(zval) )
	    {
		ps.removeSingle(idx);
		continue;
	    }
	}

	ps[idx].pos_.z = zval;
	idx++;
    }

    hor->unRef();
}


void uiEMPartServer::managePreLoad()
{
    uiHorizonPreLoadDlg dlg( appserv().parent() );
    dlg.go();
}
