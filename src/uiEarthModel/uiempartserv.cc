/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiempartserv.h"

#include "arrayndimpl.h"
#include "array2dinterpol.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "embody.h"
#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emfault3d.h"
#include "emhorizonpreload.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceio.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "variogramcomputers.h"
#include "varlenarray.h"

#include "uiarray2dchg.h"
#include "uiarray2dinterpol.h"
#include "uibulkfaultimp.h"
#include "uibulkhorizonimp.h"
#include "uibulk2dhorizonimp.h"
#include "uichangesurfacedlg.h"
#include "uicreatehorizon.h"
#include "uidlggroup.h"
#include "uiempreloaddlg.h"
#include "uiexpfault.h"
#include "uiexphorizon.h"
#include "uiexport2dhorizon.h"
#include "uigeninput.h"
#include "uihorgeom2attr.h"
#include "uihorinterpol.h"
#include "uihorsavefieldgrp.h"
#include "uihor3dfrom2ddlg.h"
#include "uitime2depthdlg.h"
#include "uiimpfault.h"
#include "uiimphorizon.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uiiosurfacedlg.h"
#include "uiisopachmaker.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultisurfaceread.h"
#include "uirandlinegen.h"
#include "uiselsimple.h"
#include "uisurfaceman.h"
#include "uitaskrunner.h"
#include "uivariogram.h"

#include <math.h>

static const char* sKeyPreLoad()		{ return "PreLoad"; }

int uiEMPartServer::evDisplayHorizon()		{ return 0; }
int uiEMPartServer::evRemoveTreeObject()	{ return 1; }

uiEMPartServer::uiEMPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , selectedrg_(false)
{
    mAttachCB( IOM().surveyChanged, uiEMPartServer::survChangedCB );
}


uiEMPartServer::~uiEMPartServer()
{
    detachAllNotifiers();
    EM::EMM().setEmpty();
    cleanup();
}


void uiEMPartServer::survChangedCB( CallBacker* )
{
    cleanup();
}


void uiEMPartServer::cleanup()
{
    closeAndNullPtr( imphorattrdlg_ );
    closeAndNullPtr( imphorgeomdlg_ );
    closeAndNullPtr( impzmapdlg_ );
    closeAndNullPtr( impfltdlg_ );
    closeAndNullPtr( impbulkfltdlg_ );
    closeAndNullPtr( impfltsetdlg_ );
    closeAndNullPtr( impbulkhordlg_ );
    closeAndNullPtr( exphordlg_ );
    closeAndNullPtr( exp2dhordlg_ );
    closeAndNullPtr( expfltdlg_ );
    closeAndNullPtr( expfltsetdlg_ );
    closeAndNullPtr( man3dhordlg_ );
    closeAndNullPtr( man2dhordlg_ );
    closeAndNullPtr( man3dfaultdlg_ );
    closeAndNullPtr( manfssdlg_ );
    closeAndNullPtr( manfltsetdlg_ );
    closeAndNullPtr( manbodydlg_ );
    closeAndNullPtr( crhordlg_ );
    closeAndNullPtr( impbulkfssdlg_ );
    closeAndNullPtr( impbulk2dhordlg_ );
    closeAndNullPtr( t2d3dhordlg_ );
    closeAndNullPtr( t2d2dhordlg_ );
    closeAndNullPtr( t2dfltdlg_ );
    closeAndNullPtr( t2dfltsetdlg_ );
    closeAndNullPtr( t2d2dfssdlg_ );
    closeAndNullPtr( t2d3dfssdlg_ );
    deepErase( variodlgs_ );
}


EM::uiTime2DepthDlg* uiEMPartServer::getTime2DepthEMDlg( uiParent* p,
			EM::ObjectType objtype, EM::uiTime2DepthDlg*& dlg )
{
    if ( !dlg || (dlg->parent() != p) )
    {
	IOPar par;
	bool usepar = false;
	if ( dlg )
	{
	    usepar = true;
	    dlg->fillPar( par );
	    delete dlg;
	}

	dlg = new EM::uiTime2DepthDlg( p, objtype );
	mAttachCB(dlg->objectToBeDeleted(),uiEMPartServer::dialogToBeDeleted);
	if ( usepar )
	    dlg->usePar( par );
    }

    return dlg;
}


void uiEMPartServer::dialogToBeDeleted( CallBacker* cb )
{
    if ( !cb )
	return;

    if ( cb == t2d3dfssdlg_ )
	t2d3dfssdlg_ = nullptr;
    else if ( cb == t2d2dfssdlg_ )
	t2d2dfssdlg_ = nullptr;
    else if ( cb == t2dfltsetdlg_ )
	t2dfltsetdlg_ = nullptr;
    else if ( cb == t2dfltdlg_ )
	t2dfltdlg_ = nullptr;
    else if ( cb == t2d3dhordlg_ )
	t2d3dhordlg_ = nullptr;
    else if ( cb == t2d2dhordlg_ )
	t2d2dhordlg_ = nullptr;
}


void uiEMPartServer::processTime2Depth( uiParent* p,
						    EM::ObjectType objtype )
{
    const uiRetVal ret =  EM::uiTime2DepthDlg::canTransform( objtype );
    if ( !ret.isOK() )
    {
	uiMSG().message( ret.messages().cat() );
	return;
    }

    EM::uiTime2DepthDlg* dlg = nullptr;
    if ( objtype == EM::ObjectType::Hor3D )
	dlg = getTime2DepthEMDlg( p, objtype, t2d3dhordlg_ );
    else if ( objtype == EM::ObjectType::Hor2D )
	dlg = getTime2DepthEMDlg( p, objtype, t2d2dhordlg_ );
    else if ( objtype == EM::ObjectType::Flt3D )
	dlg = getTime2DepthEMDlg( p, objtype, t2dfltdlg_ );
    else if ( objtype == EM::ObjectType::FltSet )
	dlg = getTime2DepthEMDlg( p, objtype, t2dfltsetdlg_ );
    else if ( objtype == EM::ObjectType::FltSS2D )
	dlg = getTime2DepthEMDlg( p, objtype, t2d2dfssdlg_ );
    else if ( objtype == EM::ObjectType::FltSS3D )
	dlg = getTime2DepthEMDlg( p, objtype, t2d3dfssdlg_ );


    if ( dlg )
	dlg->show();
}


MultiID uiEMPartServer::getStorageID( const EM::ObjectID& emid ) const
{
    return EM::EMM().getMultiID(emid);
}


EM::ObjectID uiEMPartServer::getObjectID( const MultiID& mid ) const
{
    return EM::EMM().getObjectID(mid);
}


void uiEMPartServer::manageSurfaces( const char* typstr )
{
    uiSurfaceMan dlg( parent(), EM::ObjectTypeDef().parse(typstr) );
    dlg.go();
}


void uiEMPartServer::manage3DHorizons()
{
    delete man3dhordlg_;
    man3dhordlg_ = new uiSurfaceMan( parent(), EM::ObjectType::Hor3D );
    man3dhordlg_->go();
}


void uiEMPartServer::manage2DHorizons()
{
    delete man2dhordlg_;
    man2dhordlg_ = new uiSurfaceMan( parent(), EM::ObjectType::Hor2D );
    man2dhordlg_->go();
}


void uiEMPartServer::manage3DFaults()
{
    delete man3dfaultdlg_;
    man3dfaultdlg_ = new uiSurfaceMan( parent(), EM::ObjectType::Flt3D );
    man3dfaultdlg_->go();
}


void uiEMPartServer::manageFaultStickSets()
{
    delete manfssdlg_;
    manfssdlg_ = new uiSurfaceMan( parent(), EM::ObjectType::FltSS2D3D );
    manfssdlg_->go();
}


void uiEMPartServer::manageFaultSets()
{
    delete manfltsetdlg_;
    manfltsetdlg_ = new uiSurfaceMan( parent(), EM::ObjectType::FltSet );
    manfltsetdlg_->go();
}


void uiEMPartServer::manageBodies()
{
    delete manbodydlg_;
    manbodydlg_ = new uiSurfaceMan( parent(), EM::ObjectType::Body );
    manbodydlg_->go();
}


bool uiEMPartServer::import3DHorAttr()
{
    if ( imphorattrdlg_ )
	imphorattrdlg_->raise();
    else
    {
	imphorattrdlg_ = new uiImportHorizon( parent(), false );
	imphorattrdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB) );
    }

    return imphorattrdlg_->go();
}


bool uiEMPartServer::import3DHorGeom( bool bulk )
{
    if ( bulk )
    {
	if ( !impbulkhordlg_ )
	    impbulkhordlg_ = new uiBulkHorizonImport( parent() );

	return impbulkhordlg_->go();
    }

    if ( imphorgeomdlg_ )
	imphorgeomdlg_->raise();
    else
    {
	imphorgeomdlg_ = new uiImportHorizon( parent(), true );
	imphorgeomdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB) );
    }

    return imphorgeomdlg_->go();
}


bool uiEMPartServer::importHorFromZMap()
{
    if ( !impzmapdlg_ )
    {
	impzmapdlg_ = new uiImpHorFromZMap( parent() );
	impzmapdlg_->importReady.notify(
		mCB(this,uiEMPartServer,importReadyCB) );
    }
    else
	impzmapdlg_->raise();

    return impzmapdlg_->go();
}


bool uiEMPartServer::importBulk2DHorizon()
{
    if ( !impbulk2dhordlg_ )
	impbulk2dhordlg_ = new uiBulk2DHorizonImport( parent() );
    else
	impbulk2dhordlg_->raise();

    return impbulk2dhordlg_->go();
}


void uiEMPartServer::importReadyCB( CallBacker* cb )
{
    MultiID mid = MultiID::udf();
    mDynamicCastGet(uiImportHorizon*,dlg,cb)
    if ( dlg && dlg->saveButtonChecked() )
	mid = dlg->getSelID();

    mDynamicCastGet(uiImpHorFromZMap*,zmapdlg,cb)
    if ( zmapdlg && zmapdlg->saveButtonChecked() )
	mid = zmapdlg->getSelID();

    mDynamicCastGet(uiImportFault*,fltdlg,cb)
    if ( fltdlg && fltdlg->saveButtonChecked() )
	mid = fltdlg->getSelID();

    mDynamicCastGet(uiCreateHorizon*,crdlg,cb)
    if ( crdlg && crdlg->saveButtonChecked() )
	mid = crdlg->getSelID();

    if ( mid.isUdf() ) return;

    selemid_ = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( selemid_ );
    if ( emobj ) emobj->setFullyLoaded( true );

    sendEvent( evDisplayHorizon() );
}


bool uiEMPartServer::export2DHorizon( bool bulk )
{
    delete exp2dhordlg_;
    exp2dhordlg_ = new uiExport2DHorizon( parent(), bulk );
    exp2dhordlg_->setModal( false );
    return exp2dhordlg_->go();
}


bool uiEMPartServer::export3DHorizon( bool bulk )
{
    if ( exphordlg_ && exphordlg_->isBulk()!=bulk )
	closeAndNullPtr( exphordlg_ );

    if ( !exphordlg_ )
    {
	exphordlg_ = new uiExportHorizon( parent(), bulk );
	exphordlg_->setModal( false );
    }

    return exphordlg_->go();
}


bool uiEMPartServer::importBulkFaults()
{
    if ( !impbulkfltdlg_ )
	impbulkfltdlg_ = new uiBulkFaultImport( parent(),
				EMFault3DTranslatorGroup::sGroupName(), false );
    return impbulkfltdlg_->go();
}


bool uiEMPartServer::importFaultSet()
{
    if ( !impfltsetdlg_ )
	impfltsetdlg_ = new uiBulkFaultImport( parent(),
		EMFaultSet3DTranslatorGroup::sGroupName(), false );

    return impfltsetdlg_->go();
}


bool uiEMPartServer::importFault()
{
    if ( impfltdlg_ )
	impfltdlg_->raise();
    else
    {
	impfltdlg_ =
	    new uiImportFault3D( parent(),
				 EMFault3DTranslatorGroup::sGroupName());
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
				 EMFaultStickSetTranslatorGroup::sGroupName() );
	impfltstickdlg_->importReady.notify(
				mCB(this,uiEMPartServer,importReadyCB));
    }

    return impfltstickdlg_->go();
}


void uiEMPartServer::import2DFaultStickset()
{
    if ( impfss2ddlg_ )
    {
	impfss2ddlg_->show();
	impfss2ddlg_->raise();
	return;
    }

    impfss2ddlg_ = new uiImportFaultStickSet2D( parent(),
				EMFaultStickSetTranslatorGroup::sGroupName() );
    impfss2ddlg_->importReady.notify( mCB(this,uiEMPartServer,importReadyCB) );
    impfss2ddlg_->show();
}


bool uiEMPartServer::importBulkFaultStickSet( bool is2d )
{
    impbulkfssdlg_ = new uiBulkFaultImport( parent(),
		EMFaultStickSetTranslatorGroup::sGroupName().buf(), is2d );

    return impbulkfssdlg_->go();
}


void uiEMPartServer::importBulk2DFaultStickset()
{
    importBulkFaultStickSet(true);
}


bool uiEMPartServer::exportFault( bool single )
{
    if ( expfltdlg_ && expfltdlg_->isBulk()==single )
	closeAndNullPtr( expfltdlg_ );

    if ( !expfltdlg_ )
	expfltdlg_ = new uiExportFault( parent(),
			EMFault3DTranslatorGroup::sGroupName(), !single );
    return expfltdlg_->go();
}


bool uiEMPartServer::exportFaultStickSet( bool single )
{
    if ( expfltstickdlg_ && expfltstickdlg_->isBulk()==single )
	closeAndNullPtr( expfltstickdlg_ );

    if ( !expfltstickdlg_ )
	expfltstickdlg_ = new uiExportFault( parent(),
			EMFaultStickSetTranslatorGroup::sGroupName(), !single );
    return expfltstickdlg_->go();
}


bool uiEMPartServer::exportFaultSet()
{
    if ( !expfltsetdlg_ )
	expfltsetdlg_ = new uiExportFault( parent(),
			EMFaultSet3DTranslatorGroup::sGroupName(), false );
    return expfltsetdlg_->go();
}


void uiEMPartServer::createHorWithConstZ( bool is2d )
{
    if ( !crhordlg_ )
    {
	crhordlg_ = new uiCreateHorizon( parent(), is2d );
	crhordlg_->ready.notify( mCB(this,uiEMPartServer,importReadyCB) );
    }

    crhordlg_->show();
}


void uiEMPartServer::computeIsochron()
{
    uiIsochronMakerBatch dlg( parent() );
    if ( !dlg.go() )
	return;
}


BufferString uiEMPartServer::getName( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( emobj && !emobj->name().isEmpty() )
	return emobj->name();

    return BufferString();
}


const char* uiEMPartServer::getType( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    return emobj ? emobj->getTypeStr() : nullptr;
}


uiString uiEMPartServer::getUiName( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( emobj )
    {
	const uiString uiobjnm = toUiString( emobj->name() );
	return uiobjnm.isEmpty() ? uiString::emptyString() : uiobjnm;
    }

    return uiString::emptyString();
}


uiString uiEMPartServer::getUiType( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    return emobj ? emobj->getUserTypeStr() : uiString::emptyString();
}


bool uiEMPartServer::isChanged( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject( emid );
    return emobj && emobj->isChanged();
}


bool uiEMPartServer::isGeometryChanged( const EM::ObjectID& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,EM::EMM().getObject(emid));
    return emsurf && emsurf->geometry().isChanged(0);
}


int uiEMPartServer::nrAttributes( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj ) return 0;

    EM::IOObjInfo eminfo( emobj->multiID() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    return eminfo.isOK() ? attrnms.size() : 0;
}


bool uiEMPartServer::isEmpty( const EM::ObjectID& emid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    return emobj && emobj->isEmpty();
}


bool uiEMPartServer::isFullResolution( const EM::ObjectID& emid ) const
{
    mDynamicCastGet(const EM::Surface*,emsurf,EM::EMM().getObject(emid));
    return emsurf && emsurf->geometry().isFullResolution();
}


bool uiEMPartServer::isFullyLoaded( const EM::ObjectID& emid ) const
{
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    return emobj && emobj->isFullyLoaded();
}


void uiEMPartServer::displayEMObject( const MultiID& mid )
{
    selemid_ = EM::EMM().getObjectID(mid);

    if ( !selemid_.isValid() )
    {
	loadSurface( mid );
	selemid_ = EM::EMM().getObjectID(mid);
    }

    if ( selemid_.isValid() )
    {
	mDynamicCastGet( EM::Horizon3D*,hor3d,EM::EMM().getObject( selemid_ ) )
	if ( hor3d )
	{
	    TrcKeySampling selrg;
	    selrg.setInlRange( hor3d->geometry().rowRange() );
	    selrg.setCrlRange( hor3d->geometry().colRange() );
	    setHorizon3DDisplayRange( selrg );
	}
	sendEvent( evDisplayHorizon() );
    }
}


void uiEMPartServer::displayOnCreateCB( CallBacker* cb )
{
    mDynamicCastGet(uiHorizonInterpolDlg*,interpoldlg,cb);
    mDynamicCastGet(uiFilterHorizonDlg*,filterdlg,cb);
    if ( !interpoldlg && !filterdlg )
	return;

    uiHorSaveFieldGrp* horfldsavegrp = interpoldlg ? interpoldlg->saveFldGrp()
						   : filterdlg->saveFldGrp();
    if ( horfldsavegrp->displayNewHorizon() )
	displayEMObject( horfldsavegrp->getNewHorizon()->multiID() );

    horfldsavegrp->overwriteHorizon();
}


bool uiEMPartServer::fillHoles( const EM::ObjectID& emid, bool is2d )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject(emid));
    uiHorizonInterpolDlg dlg( parent(), hor, is2d );
    dlg.horReadyForDisplay.notify(
	    mCB(this,uiEMPartServer,displayOnCreateCB) );
    return dlg.go();
}


bool uiEMPartServer::filterSurface( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(emid))
    uiFilterHorizonDlg dlg( parent(), hor3d );
    dlg.horReadyForDisplay.notify(
	    mCB(this,uiEMPartServer,displayOnCreateCB) );
    return dlg.go();
}


void uiEMPartServer::removeTreeObject( const EM::ObjectID& emid )
{
    selemid_ = emid;
    sendEvent( evRemoveTreeObject() );
}


EM::EMObject* uiEMPartServer::selEMObject()
{ return EM::EMM().getObject( selemid_ ); }


void uiEMPartServer::deriveHor3DFrom2D( const EM::ObjectID& emid )
{
    mDynamicCastGet(EM::Horizon2D*,h2d,EM::EMM().getObject(emid))
    uiHor3DFrom2DDlg dlg( parent(), *h2d, this );

    if ( dlg.go() && dlg.doDisplay() )
    {
	RefMan<EM::Horizon3D> hor = dlg.getHor3D();
	selemid_ = hor ? hor->id() : EM::ObjectID::udf();
	sendEvent( evDisplayHorizon() );
    }
}


bool uiEMPartServer::askUserToSave( const EM::ObjectID& emid,
				    bool withcancel ) const
{
    EM::EMObject* emobj = EM::EMM().getObject(emid);
    if ( !emobj || !emobj->isChanged() || !EM::canOverwrite(emobj->multiID()) )
	return true;

    PtrMan<IOObj> ioobj = IOM().get( getStorageID(emid) );
    if ( !ioobj && emobj->isEmpty() )
	return true;

    uiString msg = tr( "%1 '%2' has changed.\n\nDo you want to save it?" )
		   .arg( emobj->getTypeStr() ).arg( emobj->name() );

    const int ret = uiMSG().askSave( msg, withcancel );
    if ( ret == 1 )
    {
	const bool saveas = !ioobj || !isFullyLoaded(emid);
	return storeObject( emid, saveas );
    }

    return ret == 0;
}


void uiEMPartServer::selectHorizons( ObjectSet<EM::EMObject>& objs, bool is2d,
				uiParent* p, const ZDomain::Info* zinfo )
{
    selectSurfaces( p, objs, is2d ? EMHorizon2DTranslatorGroup::sGroupName()
			: EMHorizon3DTranslatorGroup::sGroupName(), zinfo );
}


void uiEMPartServer::selectFaults( ObjectSet<EM::EMObject>& objs, bool is2d,
				   uiParent* p, const ZDomain::Info* zinfo )
{
    if ( !is2d )
	selectSurfaces( p, objs,
			    EMFault3DTranslatorGroup::sGroupName(), zinfo );
}


void uiEMPartServer::selectFaultStickSets( ObjectSet<EM::EMObject>& objs,
				    uiParent* p, const ZDomain::Info* zinfo )
{
    selectSurfaces( p, objs,
			EMFaultStickSetTranslatorGroup::sGroupName(), zinfo );
}


static void selectEMObjects( uiParent* p, ObjectSet<EM::EMObject>& objs,
			    const IOObjContext& ctxt, const char* exectext,
			     const ZDomain::Info* zinfo )
{
    CtxtIOObj ctio( ctxt );
    ctio.ctxt_.forread_ = true;
    if ( zinfo )
    {
	const ZDomain::Info& siinfo = SI().zDomainInfo();
	ctio.ctxt_.requireZDomain( *zinfo, siinfo == *zinfo );
    }

    uiIOObjSelDlg::Setup sdsu;
    sdsu.multisel( true );
    uiIOObjSelDlg dlg( p, sdsu, ctio );
    if ( !dlg.go() )
	return;

    TypeSet<MultiID> mids;
    dlg.getChosen( mids );
    if ( mids.isEmpty() )
	return;

    ExecutorGroup loaders( exectext );
    for ( int idx=0; idx<mids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( mids[idx] );
	if ( !ioobj )
	    continue;

	BufferString typestr( ioobj->group() );
	if ( typestr == EMBodyTranslatorGroup::sGroupName() )
	    ioobj->pars().get( sKey::Type(), typestr );

	EM::EMObject* object = EM::EMM().createTempObject( typestr );
	if ( !object )
	    continue;

	object->ref();
	object->setMultiID( mids[idx] );
	objs += object;
	loaders.add( object->loader() );
    }

    uiTaskRunner execdlg( p );
    if ( !TaskRunner::execute( &execdlg, loaders ) )
    {
	deepUnRef( objs );
	return;
    }
}


void uiEMPartServer::selectFaultSets( ObjectSet<EM::EMObject>& objs,
				      uiParent* p, const ZDomain::Info* zinfo )
{
    if ( !p )
	p = parent();

    selectEMObjects( p, objs, EMFaultSet3DTranslatorGroup::ioContext(),
		     "Loading FaultSets", zinfo );
}


void uiEMPartServer::selectBodies( ObjectSet<EM::EMObject>& objs, uiParent* p )
{
    if ( !p )
	p = parent();

    selectEMObjects( p, objs, EMBodyTranslatorGroup::ioContext(),
		     "Loading Geobodies", nullptr );
}


void uiEMPartServer::selectSurfaces( uiParent* p, ObjectSet<EM::EMObject>& objs,
				const char* typ, const ZDomain::Info* zinfo )
{
    if ( !p )
	p = parent();


    uiMultiSurfaceReadDlg dlg( p, typ, zinfo );
    if ( !dlg.go() )
	return;

    TypeSet<MultiID> surfaceids;
    dlg.iogrp()->getSurfaceIds( surfaceids );

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd ), orisel( sd );
    dlg.iogrp()->getSurfaceSelection( sel );

    const bool hor3d = EMHorizon3DTranslatorGroup::sGroupName() == typ;

    if ( hor3d )
	selectedrg_ = sel.rg;

    TypeSet<MultiID> idstobeloaded;
    PtrMan<Executor> exec = EM::EMM().objectLoader( surfaceids,
						    hor3d ? &sel : &orisel,
						    &idstobeloaded);

    for ( int idx=0; idx<surfaceids.size(); idx++ )
    {
	EM::EMObject* obj = EM::EMM().getObject( surfaceids[idx] );
	if ( !obj )
	    continue;

	obj->ref();
	objs += obj;
    }

    if ( objs.isEmpty() )
	return;

    ObjectSet<EM::EMObject> objstobeloaded;
    for ( int idx=0; idx<idstobeloaded.size(); idx++ )
    {
	EM::EMObject* obj = EM::EMM().getObject( idstobeloaded[idx] );
	if ( !obj )
	    continue;

	obj->setBurstAlert( true );
	obj->ref();
	objstobeloaded += obj;
    }

    if ( exec )
    {
	uiTaskRunner execdlg( p );
	if ( !TaskRunner::execute( &execdlg, *exec ) )
	    deepUnRef( objs );
    }

    for ( int idx=0; idx<objstobeloaded.size(); idx++ )
	objstobeloaded[idx]->setBurstAlert( false );

    deepUnRef( objstobeloaded );
}


void uiEMPartServer::setHorizon3DDisplayRange( const TrcKeySampling& hs )
{
    selectedrg_ = hs;
}


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
			    const TypeSet<int>& selattribs, bool removeold )
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    if ( removeold )
	hor3d->auxdata.removeAll();

    bool retval = false;
    for ( int idx=0; idx<selattribs.size(); idx++ )
    {
	Executor* executor = hor3d->auxdata.auxDataLoader( selattribs[idx] );
	if ( !executor )
	    continue;

	uiTaskRunner runer( parent() );
	if ( runer.execute( *executor) )
	    retval = true;
    }

    return retval;
}



int uiEMPartServer::loadAuxData( const EM::ObjectID& id, const char* attrnm,
				 bool removeold )
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return -1;

    const MultiID mid = EM::EMM().getMultiID( id );
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


bool uiEMPartServer::loadAuxData( const EM::ObjectID& id,
			const BufferStringSet& selattrnms, bool removeold )
{
    const MultiID mid = EM::EMM().getMultiID( id );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    TypeSet<int> idxs;
    for ( int idx=0; idx<selattrnms.size(); idx++ )
    {
	const int selidx = attrnms.indexOf( selattrnms.get(idx) );
	if ( selidx < 0 ) continue;

	idxs += idx;
    }

    return loadAuxData( id, idxs, removeold );
}


bool uiEMPartServer::storeFaultAuxData( const EM::ObjectID& id,
	BufferString& auxdatanm, const Array2D<float>& data )
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Fault3D*, flt3d, object );
    if ( !flt3d )
	return false;

    EM::FaultAuxData* auxdata = flt3d->auxData();
    if ( !auxdata )
	return false;

    BufferStringSet atrrnms;
    auxdata->getAuxDataList( atrrnms );
    uiGetObjectName::Setup setup( toUiString("%1 %2").arg(uiStrings::sFault())
					    .arg(uiStrings::sData()), atrrnms );
    setup.inptxt_ = uiStrings::sAttribute(mPlural);
    uiGetObjectName dlg( parent(), setup );
    if ( !dlg.go() )
	return false;

    auxdatanm = dlg.text();
    if ( atrrnms.indexOf(auxdatanm.buf())>=0 )
    {
	uiString msg = tr("The name '%1' already exists, "
			  "do you really want to overwrite it?")
		     .arg(auxdatanm);
	if ( !uiMSG().askGoOn( msg ) )
	    return false;
    }

    const int sdidx = auxdata->setData( auxdatanm.buf(), &data, OD::UsePtr );
    if ( !auxdata->storeData(sdidx,false) )
    {
	uiMSG().error( auxdata->errMsg() );
	return false;
    }

    return true;
}


bool uiEMPartServer::showLoadFaultAuxDataDlg( const EM::ObjectID& id )
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Fault3D*, flt3d, object );
    if ( !flt3d )
	return false;

    EM::FaultAuxData* auxdata = flt3d->auxData();
    if ( !auxdata )
	return false;

    BufferStringSet atrrnms;
    auxdata->getAuxDataList( atrrnms );
    uiSelectFromList::Setup setup( tr("Fault Data"), atrrnms );
    setup.dlgtitle( tr("Select one attribute to be displayed") );
    uiSelectFromList dlg( parent(), setup );
    if ( !dlg.go() || !dlg.selFld() )
	return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getChosen( selattribs );
    auxdata->setSelected( selattribs );

    return true;
}


bool uiEMPartServer::showLoadAuxDataDlg( const EM::ObjectID& id )
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    const MultiID mid = EM::EMM().getMultiID( id );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet atrrnms;
    eminfo.getAttribNames( atrrnms );
    uiSelectFromList::Setup setup( tr("Horizon Data"), atrrnms );
    setup.dlgtitle( tr("1) Select one or more attributes and press \"OK\".\n"
	"2) Wait until the attributes are loaded and displayed\n"
	"3) Make sure the attribute tree-item is still selected\n"
	"4) Press the PageUp / PageDown key to scroll through"
	    " the individual attributes") );

    uiSelectFromList dlg( parent(), setup );
    if ( dlg.selFld() )
	dlg.selFld()->setMultiChoice( true );
    if ( !dlg.go() || !dlg.selFld() ) return false;

    TypeSet<int> selattribs;
    dlg.selFld()->getChosen( selattribs );
    if ( selattribs.isEmpty() ) return false;

    hor3d->auxdata.removeAll();
    ExecutorGroup exgrp( "Loading Horizon Data" );
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
    EM::EMObject* object = EM::EMM().getObject( id );
    if ( !object ) return false;

    mDynamicCastGet(EM::Surface*,surface,object);
    mDynamicCastGet(EM::Body*,body,object);

    PtrMan<Executor> exec = nullptr;
    MultiID key = object->multiID();

    if ( storeas )
    {
	if ( surface )
	{
	    uiWriteSurfaceDlg dlg( parent(), *surface, shift );
	    dlg.showAlwaysOnTop();
	    if ( !dlg.go() ) return false;

	    EM::SurfaceIOData sd;
	    EM::SurfaceIODataSelection sel( sd );
	    dlg.getSelection( sel );

	    key = dlg.ioObj() ? dlg.ioObj()->key() : MultiID::udf();
	    exec = surface->geometry().saver( &sel, &key );
	    if ( exec && dlg.replaceInTree() )
		    surface->setMultiID( key );

	    mDynamicCastGet( EM::dgbSurfaceWriter*, writer, exec.ptr() );
	    if ( writer ) writer->setShift( shift );
	}
	else
	{
	    CtxtIOObj ctio( body ? EMBodyTranslatorGroup::ioContext()
				 : object->getIOObjContext(),
				 IOM().get(object->multiID()) );

	    ctio.ctxt_.forread_ = false;

	    uiIOObjSelDlg dlg( parent(), ctio );
	    if ( !ctio.ioobj_ )
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
    if ( !ioobj->pars().hasKey( sKey::Type() ) )
    {
	ioobj->pars().set( sKey::Type(), object->getTypeStr() );
	if ( !IOM().commitChanges( *ioobj ) )
	{
	    uiMSG().error( uiStrings::phrCannotWriteDBEntry(ioobj->uiName()) );
	    return false;
	}
    }

    EM::EMObjectCallbackData cbdata;
    cbdata.event = EM::EMObjectCallbackData::NameChange;
    object->change.trigger( cbdata );
    object->resetChangedFlag();
    storagekey = key;
    uiTaskRunner exdlg( parent() );
    const bool ret = TaskRunner::execute( &exdlg, *exec );
    if( ret && surface )
	surface->saveDisplayPars();

    return ret;

}


bool uiEMPartServer::storeAuxData( const EM::ObjectID& id,
				   BufferString& auxdatanm, bool storeas ) const
{
    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet(EM::Horizon3D*,hor3d,object);
    if ( !hor3d )
	return false;

    const ObjectSet<BinIDValueSet>& datastor = hor3d->auxdata.getData();
    if ( datastor.isEmpty() )
	return false;

    bool hasdata = false;
    for ( int idx=0; idx<datastor.size(); idx++ )
    {
	const BinIDValueSet* bvs = datastor[idx];
	if ( bvs && !bvs->isEmpty() )
	{
	    hasdata = true;
	    break;
	}
    }

    if ( !hasdata )
	return false;

    uiTaskRunner exdlg( parent() );
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
	uiMSG().error( tr("Cannot save attribute") );
	return false;
    }

    return TaskRunner::execute( &exdlg, *saver );
}


int uiEMPartServer::setAuxData( const EM::ObjectID& id, DataPointSet& data,
				const char* attribnm, int idx, float shift )
{
    if ( !data.size() )
    { uiMSG().error(tr("No data calculated")); return -1; }

    EM::EMObject* object = EM::EMM().getObject( id );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
    { uiMSG().error(tr("No horizon loaded")); return -1; }

    BufferString auxnm = attribnm;
    if ( auxnm.isEmpty() )
    {
	auxnm = "AuxData";
	auxnm += idx;
    }

    const BinIDValueSet& bivs = data.bivSet();
    const int nrdatavals = bivs.nrVals();
    if ( idx>=nrdatavals ) return -1;

    hor3d->auxdata.removeAll();
    const int auxdataidx = hor3d->auxdata.addAuxData( auxnm );
    hor3d->auxdata.setAuxDataShift( auxdataidx, shift );

    BinID bid;
    BinIDValueSet::SPos pos;
    EM::PosID posid( id );
    while ( bivs.next(pos) )
    {
	bid = bivs.getBinID( pos );
	const float* vals = bivs.getVals( pos );

	RowCol rc( bid.inl(), bid.crl() );
	EM::SubID subid = rc.toInt64();
	posid.setSubID( subid );
	hor3d->auxdata.setAuxDataVal( auxdataidx, posid, vals[idx] );
    }

    return auxdataidx;
}


bool uiEMPartServer::getAuxData( const EM::ObjectID& oid, int auxdataidx,
				 DataPointSet& data, float& shift ) const
{
    EM::EMObject* object = EM::EMM().getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    const StringView nm = hor3d->auxdata.auxDataName( auxdataidx );
    if ( !hor3d || !hor3d->geometry().geometryElement() || nm.isEmpty() )
	return false;

    shift = hor3d->auxdata.auxDataShift( auxdataidx );
    data.dataSet().add( new DataColDef(sKeySectionID()) );
    data.dataSet().add( new DataColDef(nm) );

    float auxvals[3];
    auxvals[1] = EM::SectionID::def().asInt();

    PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator();
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( !pid.isValid() )
	    break;

	auxvals[0] = (float) hor3d->getPos( pid ).z;
	auxvals[2] = hor3d->auxdata.getAuxDataVal( auxdataidx, pid );
	BinID bid = BinID::fromInt64( pid.subID() );
	data.bivSet().add( bid, auxvals );
    }

    data.dataChanged();
    return true;
}


bool uiEMPartServer::getAllAuxData( const EM::ObjectID& oid,
	DataPointSet& data, TypeSet<float>* shifts,
	const TrcKeyZSampling* cs ) const
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(oid))
    if ( !hor3d || !hor3d->geometry().geometryElement() )
	return false;

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

    data.bivSet().allowDuplicateBinIDs(false);
    mAllocVarLenArr( float, auxvals, nms.size()+2 );
    for ( int sidx=0; sidx<hor3d->nrSections(); sidx++ )
    {
	if ( !hor3d->geometry().geometryElement() )
	    continue;

	auxvals[0] = 0;
	auxvals[1] = EM::SectionID::def().asInt();
	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( cs );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( !pid.isValid() )
		break;

	    BinID bid = BinID::fromInt64( pid.subID() );
	    if ( cs )
	    {
		if ( !cs->hsamp_.includes(bid) )
		    continue;

		BinID diff = bid - cs->hsamp_.start_;
		if ( diff.inl() % cs->hsamp_.step_.inl() ||
		     diff.crl() % cs->hsamp_.step_.crl() )
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
    EM::EMObject* object = EM::EMM().getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid<0 || mIsUdf(cid) )
	return false;

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
	msgiserror ? uiMSG().error( mToUiStringTodo(errmsg.buf()) )
		   : uiMSG().warning( mToUiStringTodo(errmsg.buf()) );
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
    EM::EMObject* object = EM::EMM().getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d ) return false;

    BinIDValueSet& bivs = dpset.bivSet();
    if ( dpset.dataSet().nrCols() != bivs.nrVals() )
	return false;

    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    const StepInterval<int> rowrg = hor3d->geometry().rowRange();
    const StepInterval<int> colrg = hor3d->geometry().colRange( -1 );
    BinID step( rowrg.step, colrg.step );
    BIDValSetArrAdapter adapter( bivs, cid, step );

    PtrMan<Task> changer;
    uiTaskRunner execdlg( parent() );
    if ( interpolate )
    {
	uiSingleGroupDlg dlg( parent(),
		uiDialog::Setup( tr("Interpolate horizon Data"),
				 tr("Interpolation parameters"),
				  mNoHelpKey ) );

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

	PtrMan< Array2D<float> > arr = hor3d->createArray2D();
	const float* arrptr = arr ? arr->getData() : nullptr;
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
    const uiString infomsg =
			interp ? interp->infoMsg() : uiStrings::sEmptyString();
    if ( infomsg.isSet() )
	uiMSG().message( infomsg );

    return true;
}


bool uiEMPartServer::attr2Geom( const EM::ObjectID& oid,
	const char* nm, const DataPointSet& dpset )
{
    const int cid = getColID( dpset, nm );
    if ( cid < 0 )
	return false;

    EM::EMObject* object = EM::EMM().getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorAttr2Geom dlg( parent(), *hor3d, dpset, cid );
    if ( !dlg.go() )
	return false;

    if ( dlg.saveFldGrp()->displayNewHorizon() &&
	 dlg.saveFldGrp()->getNewHorizon( ) )
	displayEMObject( dlg.saveFldGrp()->getNewHorizon()->multiID() );

    return true;
}


bool uiEMPartServer::geom2Attr( const EM::ObjectID& oid )
{
    EM::EMObject* object = EM::EMM().getObject( oid );
    mDynamicCastGet( EM::Horizon3D*, hor3d, object );
    if ( !hor3d )
	return false;

    uiHorGeom2Attr dlg( parent(), *hor3d );
    return dlg.go();
}


void uiEMPartServer::removeUndo()
{
    EM::EMM().eraseUndoList();
}


bool uiEMPartServer::loadSurface( const MultiID& mid,
				const EM::SurfaceIODataSelection* newsel ) const
{
    if ( EM::EMM().getObject(EM::EMM().getObjectID(mid)) )
	return true;

    Executor* exec = EM::EMM().objectLoader( mid, newsel );
    if ( !exec )
    {
	PtrMan<IOObj> ioobj = IOM().get(mid);
	BufferString nm = ioobj ? ioobj->name().buf()
				: mid.toString().buf();
	uiString msg = tr( "Cannot load '%1'" ).arg( nm );
	uiMSG().error( msg );
	return false;
    }

    EM::EMObject* obj = EM::EMM().getObject( EM::EMM().getObjectID(mid) );
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


MultiID uiEMPartServer::genRandLine( int opt )
{
    MultiID key;
    if ( opt == 0 )
    {
	uiGenRanLinesByShift dlg( parent() );
	if ( dlg.go() )
	{
	    key = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else if ( opt == 1 )
    {
	uiGenRanLinesByContour dlg( parent() );
	if ( dlg.go() )
	{
	    key = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }
    else
    {
	uiGenRanLineFromPolygon dlg( parent() );
	if ( dlg.go() )
	{
	    key = dlg.getNewSetID();
	    disponcreation_ = dlg.dispOnCreation();
	}
    }

    return key;
}


ZAxisTransform* uiEMPartServer::getHorizonZAxisTransform( bool is2d ) const
{
    uiDialog dlg( parent(),
		 uiDialog::Setup(uiStrings::phrSelect(uiStrings::sHorizon()),
				 mNoDlgTitle,
				 mODHelpKey(mgetHorizonZAxisTransformHelpID)) );
    const IOObjContext ctxt = is2d
	? EMHorizon2DTranslatorGroup::ioContext()
	: EMHorizon3DTranslatorGroup::ioContext();
    uiIOObjSel* horfld = new uiIOObjSel( &dlg, ctxt );
    if ( !dlg.go() || !horfld->ioobj() )
	return nullptr;

    const MultiID hormid = horfld->key();
    EM::ObjectID emid = getObjectID( hormid );
    if ( !emid.isValid() || !isFullyLoaded(emid) )
    {
	if ( !loadSurface( hormid ) )
	    return nullptr;
    }

    emid = getObjectID( hormid );
    if ( !emid.isValid() )
	return nullptr;

    EM::EMObject* obj = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Horizon*,hor,obj)
    if ( !hor )
	return nullptr;

    EM::HorizonZTransform* transform = new EM::HorizonZTransform();
    transform->setHorizon( *hor );
    return transform;
}


void uiEMPartServer::getSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos ) const
{
    for ( int idx=0; idx<EM::EMM().nrLoadedObjects(); idx++ )
    {
	mDynamicCastGet(EM::Horizon3D*,hor3d,
			EM::EMM().getObject(EM::EMM().objectID(idx)))
	if ( hor3d )
	    hinfos += new SurfaceInfo( hor3d->name(), hor3d->multiID() );
    }
}


void uiEMPartServer::getAllSurfaceInfo( ObjectSet<SurfaceInfo>& hinfos,
					bool is2d, const ZDomain::Info* zinfo )
{
    const IODir iodir( IOObjContext::getStdDirData(IOObjContext::Surf)->id_ );
    StringView groupstr = is2d
	? EMHorizon2DTranslatorGroup::sGroupName()
	: EMHorizon3DTranslatorGroup::sGroupName();
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ioobj->translator() != "dGB" )
	    continue;

	if ( ioobj->group() == groupstr  )
	{
	    const ZDomain::Info* objzinfo = ZDomain::get( ioobj->pars() );
	    if ( zinfo )
	    {
		if ( objzinfo && zinfo->isCompatibleWith(*objzinfo) )
		    hinfos += new SurfaceInfo( ioobj->name(), ioobj->key() );
		else if ( !objzinfo &&
				zinfo->isCompatibleWith(SI().zDomainInfo()) )
		    hinfos += new SurfaceInfo( ioobj->name(), ioobj->key() );
	    }
	    else
		hinfos += new SurfaceInfo( ioobj->name(), ioobj->key() );
	}
    }
}


void uiEMPartServer::getSurfaceDef3D( const TypeSet<EM::ObjectID>& selhorids,
				    BinIDValueSet& bivs,
				    const TrcKeySampling& hs ) const
{
    bivs.setEmpty(); bivs.setNrVals( 2, false );

    const EM::ObjectID& id = selhorids[0];
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(id))
    if ( !hor3d ) return;
    hor3d->ref();

    EM::Horizon3D* hor3d2 = nullptr;
    if ( selhorids.size() > 1 )
    {
	hor3d2 = (EM::Horizon3D*)(EM::EMM().getObject(selhorids[1]));
	hor3d2->ref();
    }

    BinID bid;
    for ( bid.inl()=hs.start_.inl(); bid.inl()<=hs.stop_.inl();
	  bid.inl()+=hs.step_.inl() )
    {
	for ( bid.crl()=hs.start_.crl(); bid.crl()<=hs.stop_.crl();
	      bid.crl()+=hs.step_.crl() )
	{
	    const EM::SubID subid = bid.toInt64();
	    TypeSet<Coord3> z1pos, z2pos;
	    for ( int idx=hor3d->nrSections()-1; idx>=0; idx-- )
	    {
		if ( hor3d->isDefined( subid ) )
		    z1pos += hor3d->getPos( subid );
	    }

	    if ( z1pos.isEmpty() ) continue;

	    if ( !hor3d2 )
	    {
		for ( int posidx=0; posidx<z1pos.size(); posidx++ )
		    bivs.add( bid, (float) z1pos[posidx].z,
			      (float) z1pos[posidx].z );
	    }
	    else
	    {
		for ( int idx=hor3d2->nrSections()-1; idx>=0; idx-- )
		{
		    if ( hor3d2->isDefined( subid ) )
			z2pos += hor3d2->getPos( subid );
		}

		if ( z2pos.isEmpty() ) continue;

		Interval<float> zintv;
		float dist = 999999;
		for ( int z1idx=0; z1idx<z1pos.size(); z1idx++ )
		{
		    for ( int z2idx=0; z2idx<z2pos.size(); z2idx++ )
		    {
			float dist_ = (float) (z2pos[z2idx].z - z1pos[z1idx].z);
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
    if ( !id.isValid() || !isFullyLoaded(id) ) \
	loadSurface( horid ); \
    id = getObjectID(horid); \
}
void uiEMPartServer::getSurfaceDef2D( const ObjectSet<MultiID>& selhorids,
				  const BufferStringSet& selectlines,
				  TypeSet<Coord>& coords,
				  TypeSet< Interval<float> >& zrgs ) const
{
    TypeSet<BinID> dummybids;
    getSurfaceDef2D( selhorids, selectlines, coords, dummybids, zrgs );
}


void uiEMPartServer::getSurfaceDef2D( const ObjectSet<MultiID>& selhorids,
				  const BufferStringSet& selectlines,
				  TypeSet<Coord>& coords, TypeSet<BinID>& bids,
				  TypeSet< Interval<float> >& zrgs ) const
{
    if ( !selhorids.size() )
	return;

    EM::ObjectID id;
    const bool issecondhor = selhorids.size()>1;
    mGetObjId(0,id);
    mDynamicCastGet(EM::Horizon2D*,hor2d1,EM::EMM().getObject(id));

    EM::Horizon2D* hor2d2 = nullptr;
    if ( issecondhor )
    {
	mGetObjId(1,id);
	mDynamicCastGet(EM::Horizon2D*,hor,EM::EMM().getObject(id));
	hor2d2 = hor;
    }

    if ( !hor2d1 || ( issecondhor && !hor2d2 ) ) return;

    for ( int lidx=0; lidx<selectlines.size(); lidx++ )
    {
	int lineidx = hor2d1->geometry().lineIndex( selectlines.get(lidx) );
	if ( lineidx<0 ) continue;

	const Pos::GeomID geomid = hor2d1->geometry().geomID( lineidx );
	const StepInterval<int> trcrg = hor2d1->geometry().colRange( geomid );
	if ( trcrg.isUdf() ) continue;

	for ( int trcidx=0; trcidx<=trcrg.nrSteps(); trcidx++ )
	{
	    const int trcnr = trcrg.atIndex( trcidx );
	    const Coord3 pos1 = hor2d1->getPos( geomid, trcnr );
	    const float z1 = sCast( float, pos1.z );
	    float z2 = mUdf(float);

	    if ( issecondhor )
		z2 = sCast( float, hor2d2->getPos(geomid,trcnr).z );

	    if ( !mIsUdf(z1) && ( !issecondhor || !mIsUdf(z2) ) )
	    {
		Interval<float> zrg( z1, issecondhor ? z2 : z1 );
		zrgs += zrg;
		coords += pos1;
		bids += BinID( geomid.asInt(), trcnr );
	    }
	}
    }
}


void uiEMPartServer::fillPickSet( Pick::Set& ps, const MultiID& horid ) const
{
    EM::ObjectID id = getObjectID( horid );
    if ( !id.isValid() || !isFullyLoaded(id) )
	loadSurface( horid );

    id = getObjectID( horid );
    if ( !id.isValid() )
	return;

    EM::EMObject* obj = EM::EMM().getObject( id );
    mDynamicCastGet(EM::Horizon3D*,hor,obj)
    if ( !hor )
	return;

    hor->ref();
    int idx = 0;
    while ( idx < ps.size() )
    {
	const BinID bid = SI().transform( ps.get(idx).pos() );
	const EM::SubID subid = bid.toInt64();
	double zval = hor->getPos( subid ).z;
	if ( mIsUdf(zval) )
	{
	    const Geometry::BinIDSurface* geom =
		hor->geometry().geometryElement();
	    if ( geom )
		zval = geom->computePosition( Coord(bid.inl(),bid.crl()) ).z;

	    if ( mIsUdf(zval) )
	    {
		ps.remove(idx);
		continue;
	    }
	}

	ps.setZ( idx, zval );
	idx++;
    }

    hor->unRef();
}


void uiEMPartServer::managePreLoad()
{
    uiHorizonPreLoadDlg dlg( appserv().parent() );
    dlg.go();
}


void uiEMPartServer::fillPar( IOPar& par ) const
{
    const TypeSet<MultiID>& mids = EM::HPreL().getPreloadedIDs();
    for ( int idx=0; idx<mids.size(); idx++ )
	par.set( IOPar::compKey(sKeyPreLoad(),idx), mids[idx] );
}


bool uiEMPartServer::usePar( const IOPar& par )
{
    const int maxnr2pl = 1000;
    TypeSet<MultiID> mids;
    for ( int idx=0; idx<maxnr2pl; idx++ )
    {
	MultiID mid = MultiID::udf();
	par.get( IOPar::compKey(sKeyPreLoad(),idx), mid );
	if ( mid.isUdf() )
	    break;

	mids += mid;
    }

    uiTaskRunner uitr( parent() );
    EM::HPreL().load( mids, &uitr );
    return true;
}
