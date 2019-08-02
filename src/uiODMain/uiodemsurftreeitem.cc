/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodemsurftreeitem.h"

#include "datapointset.h"
#include "datacoldef.h"
#include "emhorizon3d.h"
#include "emhorizonztransform.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "threadwork.h"
#include "timefun.h"

#include "uiattribpartserv.h"
#include "uiempartserv.h"
#include "uiemattribpartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uistrings.h"
#include "uiviscoltabed.h"
#include "uivisemobj.h"
#include "uivispartserv.h"
#include "visemobjdisplay.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"


uiODDataTreeItem* uiODEarthModelSurfaceTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().createSuitable( *as, parenttype ) : 0;
    if ( !res )
	res = new uiODEarthModelSurfaceDataTreeItem( emid_, uivisemobj_,
							     parenttype );
    return res;
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const DBKey& nemid )
    : uiODDisplayTreeItem()
    , emid_(nemid)
    , uivisemobj_(0)
    , createflatscenemnuitem_(tr("Create Flattened Scene"))
    , savemnuitem_(uiStrings::sSave(),-800)
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs()),-850)
    , enabletrackingmnuitem_(tr("Enable Tracking"))
    , changesetupmnuitem_(m3Dots(tr("Change Settings")))
    , reloadmnuitem_(uiStrings::sReload(),-750)
    , trackmenuitem_(uiStrings::sTracking())
    , starttrackmnuitem_(m3Dots(tr("Start Tracking")))
    , istrackingallowed_(true)
{
    savemnuitem_.iconfnm = "save";
    saveasmnuitem_.iconfnm = "saveas";
    enabletrackingmnuitem_.checkable = true;
    changesetupmnuitem_.iconfnm = "seedpicksettings";
    NotSavedPrompter::NSP().promptSaving.notify(
	    mCB(this,uiODEarthModelSurfaceTreeItem,askSaveCB));
}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{
    NotSavedPrompter::NSP().promptSaving.remove(
	    mCB(this,uiODEarthModelSurfaceTreeItem,askSaveCB));
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    emd,visserv_->getObject(displayid_));
    if ( emd )
    {
	emd->selection()->remove(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
	emd->deSelection()->remove(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
    }

    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().unRefTracker( emid_ );
    delete uivisemobj_;
}


#define mDelRet { delete uivisemobj_; uivisemobj_ = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    if ( !createUiVisObj() )
	return false;

    if ( !uiODDisplayTreeItem::init() )
	return false;

    if ( MPE::engine().hasTracker(emid_) )
	MPE::engine().refTracker( emid_ );

    //TODO PrIMPL this has to go to Probe stuff
    if ( visserv_->hasAttrib( displayid_ ) )
    {
	for ( int attrib=0; attrib<visserv_->getNrAttribs(displayid_); attrib++)
	{
	    const Attrib::SelSpec* as = visserv_->getSelSpec(displayid_,attrib);
	    uiODDataTreeItem* item = createAttribItem( as );
	    if ( item )
	    {
		addChild( item, false );
		item->setChecked( visserv_->isAttribEnabled(displayid_,attrib));
	    }
	}
    }

    EM::IOObjInfo eminfo( emid_ );
    timelastmodified_ = eminfo.timeLastModified();
    initNotify();
    return true;
}


bool uiODEarthModelSurfaceTreeItem::createUiVisObj()
{
    if ( uivisemobj_ )
	return true;

    if ( displayid_!=-1 )
    {
	uivisemobj_ = new uiVisEMObject( ODMainWin(), displayid_, visserv_ );
	if ( !uivisemobj_->isOK() )
	    mDelRet;

	emid_ = uivisemobj_->getObjectID();
    }
    else
    {
	uivisemobj_ = new uiVisEMObject( ODMainWin(), emid_, sceneID(),
					visserv_ );
	displayid_ = uivisemobj_->id();
	if ( !uivisemobj_->isOK() )
	    mDelRet;
    }

    mDynamicCastGet(visSurvey::EMObjectDisplay*,
		    emd,visserv_->getObject(displayid_));
    if ( emd )
    {
	emd->selection()->notify(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
	emd->deSelection()->notify(
		mCB(this,uiODEarthModelSurfaceTreeItem,selChg) );
    }

    return true;
}


void uiODEarthModelSurfaceTreeItem::selChg( CallBacker* )
{
    updateTrackingState();
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);
    updateTrackingState();
}


void uiODEarthModelSurfaceTreeItem::updateTrackingState()
{
    uiMPEPartServer* mps = applMgr()->mpeServer();
    const int trackerid = mps->getTrackerID( emid_ );
    if ( trackerid == -1 )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     emod, visserv_->getObject(displayid_) );

    const bool enabletracking = istrackingallowed_ && isChecked() &&
				emod && emod->isSelected();

    if ( mps->isTrackingEnabled(trackerid) != enabletracking )
	mps->enableTracking( trackerid, enabletracking );
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODEarthModelSurfaceTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    const bool hastransform = scene && scene->getZAxisTransform();

    uiMPEPartServer* mps = applMgr()->mpeServer();
    const bool hastracker = mps->getTrackerID(emid_)>=0;
    if ( !hastracker && !visserv_->isLocked(displayid_) )
    {
	mAddMenuItem( &trackmenuitem_, &starttrackmnuitem_, true, false );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
    }
    else if ( hastracker && !visserv_->isLocked(displayid_) &&
	      !hastransform )
    {
	mAddMenuItem( &trackmenuitem_, &starttrackmnuitem_, false, false );
	mAddMenuOrTBItem( istb, menu, &trackmenuitem_, &changesetupmnuitem_,
			  true, false );
	mAddMenuItem( &trackmenuitem_, &enabletrackingmnuitem_, true,
		      istrackingallowed_ );
    }
    else
    {
	mResetMenuItem( &starttrackmnuitem_ );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
    }

    const bool isshifted =
		!mIsZero( visserv_->getTranslation(displayID()).z_, 1e-5 );
    const bool enab = trackmenuitem_.nrItems() && !isshifted && isChecked()
			&& EM::canOverwrite( emid_ );
    mAddMenuOrTBItem( istb, 0, menu, &trackmenuitem_, enab, false );

    const EM::IOObjInfo eminfo( emid_ );
    const BufferString curtime = eminfo.timeLastModified();
    const bool isnewer = Time::isEarlierStamp( timelastmodified_, curtime );
    const bool allowreload = !hastracker && isnewer;
    mAddMenuOrTBItem( istb, 0, menu, &reloadmnuitem_, allowreload, false );

    mAddMenuOrTBItem( istb, menu, menu, &savemnuitem_,
		  applMgr()->EMServer()->isChanged(emid_) &&
		  applMgr()->EMServer()->isFullyLoaded(emid_) &&
		  EM::canOverwrite( emid_ ) &&
		  !isshifted, false );

    const bool istransformedandshifted = hastransform && isshifted;
    mAddMenuOrTBItem( istb, menu, menu, &saveasmnuitem_,
		      !istransformedandshifted, false );
    mResetMenuItem( &createflatscenemnuitem_ );
}


int uiODEarthModelSurfaceTreeItem::reloadEMObject()
{
    uiEMPartServer* ems = applMgr()->EMServer();

    applMgr()->visServer()->removeObject( displayid_, sceneID() );
    delete uivisemobj_; uivisemobj_ = 0;

    if ( !ems->loadSurface(emid_) )
	return -1;

    uivisemobj_ = new uiVisEMObject( ODMainWin(), emid_, sceneID(), visserv_ );
    displayid_ = uivisemobj_->id();
    return displayid_;
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiEMPartServer* ems = applMgr()->EMServer();

    if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled( true );
	saveCB( 0 );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	const DBKey oldmid = emid_;
	mps->prepareSaveSetupAs( oldmid );

	DBKey storedmid;
	ems->storeObject( oldmid, true, storedmid,
		(float) visserv_->getTranslation(displayID()).z_);
	applMgr()->visServer()->setObjectName( displayid_,
						storedmid.name() );
	mps->saveSetupAs( storedmid );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==starttrackmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->enableMenusAndToolBars( false );
	applMgr()->enableTree( false );

	if ( mps->addTracker(emid_) != -1 )
	{
	    mps->useSavedSetupDlg( emid_ );
	    uivisemobj_->checkTrackingStatus();
	    addAuxDataItems();
	    applMgr()->visServer()->triggerTreeUpdate();
	    applMgr()->visServer()->turnSeedPickingOn( true );
	}

	applMgr()->enableMenusAndToolBars( true );
	applMgr()->enableTree( true );
    }
    else if ( mnuid==changesetupmnuitem_.id )
    {
	menu->setIsHandled(true);
	mps->showSetupDlg( emid_ );
    }
    else if ( mnuid==reloadmnuitem_.id )
    {
	menu->setIsHandled(true);
	reloadEMObject();
    }
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	istrackingallowed_ = !istrackingallowed_;
	updateTrackingState();
    }
    else if ( mnuid==createflatscenemnuitem_.id )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,
			emd,visserv_->getObject(displayid_))
	const DBKey objectid = emd->getObjectID();
	mDynamicCastGet(const EM::Horizon*,horizon,
			EM::MGR().getObject(objectid))
	if ( !horizon ) return;

	const uiString scenenm =
		tr("Flattened on '%1'").arg( horizon->getName() );
	RefMan<EM::HorizonZTransform> transform = new EM::HorizonZTransform;
	transform->setHorizon( *horizon );
	ODMainWin()->sceneMgr().tile();
	ODMainWin()->sceneMgr().addScene( true, transform, scenenm );
    }
}


bool uiODEarthModelSurfaceTreeItem::isHorReady( const DBKey& emid )
{
    EM::Object* emobj = EM::MGR().getObject(emid_);
    if ( !emobj )
	return false;

    if ( !emid_.isUsable() )
    {
	uiString msg = tr(" To continue, "
	    "%1'%2' has to be saved.\n\nDo you want to save it?")
	    .arg(emobj->getTypeStr()).arg(emobj->name());

	const int ret = mTIUiMsg().askSave( msg, false );
	if ( ret!=1 )
	    return false;

	saveCB( 0 );
    }

    return true;
}


void uiODEarthModelSurfaceTreeItem::askSaveCB( CallBacker* )
{
    uiEMPartServer* ems = applMgr()->EMServer();
    if ( !ems->isChanged( emid_ ) )
	return;

    bool savewithname = emid_.isInvalid();
    if ( !savewithname )
    {
	PtrMan<IOObj> ioobj = emid_.getIOObj();
	savewithname = !ioobj;
    }

    const uiString obj = toUiString("%1 \"%2\"")
	.arg( EM::MGR().objectType( emid_ ) ).arg(emid_.name());
    NotSavedPrompter::NSP().addObject(	obj,
		mCB( this, uiODEarthModelSurfaceTreeItem, saveCB ),
	        savewithname, 0 );

    Threads::WorkManager::twm().addWork(
	    Threads::Work( *new uiTreeItemRemover( parent_, this ), true ), 0,
	    NotSavedPrompter::NSP().queueID(), false );
}


void uiODEarthModelSurfaceTreeItem::saveCB( CallBacker* cb )
{
    uiMPEPartServer* mps = applMgr()->mpeServer();
    uiEMPartServer* ems = applMgr()->EMServer();
    const bool hastracker = MPE::engine().hasTracker(emid_);

    if ( !hastracker && ems->isGeometryChanged(emid_)
		     && ems->nrAttributes(emid_)>0 )
    {
	const bool res = mTIUiMsg().askSave(
		tr("Geometry has been changed. Saved 'Horizon Data' is\n"
		   "not valid anymore and will be removed now.\n"
		   "Continue saving?") );
	if ( !res )
	    return;
    }

    bool savewithname = emid_.isInvalid();
    if ( !savewithname )
    {
	PtrMan<IOObj> ioobj = emid_.getIOObj();
	savewithname = !ioobj;
    }

    if ( applMgr()->EMServer()->storeObject( emid_, savewithname ) && cb )
	NotSavedPrompter::NSP().reportSuccessfullSave();

    applMgr()->visServer()->setObjectName( displayid_, emid_.name() );
    mps->saveSetup( emid_ );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiODEarthModelSurfaceTreeItem::addAuxDataItems()
{
    mDynamicCastGet(const EM::Horizon3D*,hor3d,EM::MGR().getObject(emid_))
    if ( !hor3d )
	return;

    BufferStringSet attrnms;
    hor3d->auxdata.getUsableAuxDataNames( attrnms );

    applMgr()->EMServer()->loadAuxData( emid_, attrnms, true );

    for ( int idx=0; idx<hor3d->auxdata.nrAuxData(); idx++ )
    {
	RefMan<DataPointSet> dps = new DataPointSet( false, true );
	float shift;
	applMgr()->EMServer()->getAuxData( emid_, idx, *dps, shift );
	uiODDataTreeItem* itm = addAttribItem();
	mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,dataitm,itm);
	if ( !dataitm )
	    continue;

	dataitm->setDataPointSet( *dps );
	dataitm->setChecked( false, true );
    }
}


// uiODEarthModelSurfaceDataTreeItem
uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							const DBKey& objid,
							uiVisEMObject* uv,
							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_(uiStrings::sZValue(mPlural))
    , savesurfacedatamnuitem_(m3Dots(tr("Save as Horizon Data")))
    , loadsurfacedatamnuitem_(m3Dots(tr("Horizon Data")))
    , algomnuitem_(uiStrings::sTools())
    , filtermnuitem_(m3Dots(uiStrings::sFiltering()))
    , fillholesmnuitem_(m3Dots(uiStrings::sGridding()))
    , horvariogrammnuitem_(m3Dots(tr("Variogram")))
    , attr2geommnuitm_(m3Dots(tr("Set Z values")))
    , changed_(false)
    , emid_(objid)
    , uivisemobj_(uv)
{
    horvariogrammnuitem_.iconfnm = "variogram";
}


void uiODEarthModelSurfaceDataTreeItem::createMenu( MenuHandler* menu,
						    bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
						     attribNr() );

    const bool islocked = visserv->isLocked( displayID() );
    const int nrsurfdata = applMgr()->EMServer()->nrAttributes( emid_ );
    uiString itmtxt = m3Dots(tr("Horizon Data (%1)").arg( nrsurfdata ));
    loadsurfacedatamnuitem_.text = itmtxt;
    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
		  !islocked && nrsurfdata>0, false );
    if ( uivisemobj_ )
    {
	mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id() ==Attrib::SelSpec::cNoAttribID() );
    }
    else
    {
	mResetMenuItem( &depthattribmnuitem_ );
    }

    const bool enabsave = changed_ ||
	(as && as->id()!=Attrib::SelSpec::cNoAttribID() &&
	 as->id()!=Attrib::SelSpec::cAttribNotSelID() );

    mAddMenuItem( menu, &savesurfacedatamnuitem_, enabsave, false );
    const bool enabletool = !MPE::engine().trackingInProgress();
    mAddMenuItem( menu, &algomnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &filtermnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &fillholesmnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &horvariogrammnuitem_, enabletool, false );
    mAddMenuItem( &algomnuitem_, &attr2geommnuitm_, enabletool, false );
}


void uiODEarthModelSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    const int visid = displayID();
    const int attribnr = attribNr();

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attribnr );
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	RefMan<DataPointSet> vals = new DataPointSet( false, true );
	vals->bivSet().setNrVals( 3 );
	visserv->getRandomPosCache( visid, attribnr, *vals );
	if ( !vals->isEmpty() )
	{
	    const float shift = (float) visserv->getTranslation( visid ).z_;
	    const int validx = visserv->selectedTexture( visid, attribnr ) + 2;
	    const int auxnr = applMgr()->EMServer()->setAuxData( emid_, *vals,
		    toString(name_), validx, shift );
	    if ( auxnr<0 )
	    {
		pErrMsg( "Cannot find Horizon Data." );
		return;
	    }

	    BufferString auxdatanm;
	    const bool saved =
		applMgr()->EMServer()->storeAuxData( emid_, auxdatanm, true );
	    if ( saved )
	    {
		const Attrib::SelSpec newas( auxdatanm,
			Attrib::SelSpec::cOtherAttribID() );
		visserv->setSelSpec( visid, attribnr, newas );
		BufferStringSet* userrefs = new BufferStringSet;
		userrefs->add( "Section ID" );
		userrefs->add( auxdatanm );
		visserv->setUserRefs( visid, attribnr, userrefs );
		updateColumnText( uiODSceneMgr::cNameColumn() );
	    }
	    changed_ = !saved;
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj_->setDepthAsAttrib( attribnr );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	applMgr()->updateColorTable( displayID(), attribnr );
	changed_ = false;
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	selectAndLoadAuxData();
	changed_ = false;
    }
    else if ( mnuid==fillholesmnuitem_.id || mnuid==filtermnuitem_.id
	   || mnuid==attr2geommnuitm_.id || mnuid==horvariogrammnuitem_.id )
    {
	menu->setIsHandled( true );

	if ( !as || as->id() != Attrib::SelSpec::cOtherAttribID() )
	{
	    mTIUiMsg().error(tr("This algorithm can only be applied on "
			    "'Horizon Data'.\nPlease save attribute first"));
	    return;
	}

	bool res = false;
	RefMan<DataPointSet> vals = new DataPointSet( false, true );
	visserv->getRandomPosCache( visid, attribnr, *vals );
	if ( mnuid==filtermnuitem_.id )
	    res = applMgr()->EMServer()->filterAuxData( emid_,
				    toString(name_), *vals);
	else if ( mnuid==fillholesmnuitem_.id )
	    res = applMgr()->EMServer()->
			interpolateAuxData( emid_, toString(name_), *vals );
	else if ( mnuid==horvariogrammnuitem_.id )
	    res = applMgr()->EMServer()->
		computeVariogramAuxData( emid_, toString(name_), *vals );
	else if ( mnuid==attr2geommnuitm_.id )
	    res = applMgr()->EMServer()->
		attr2Geom( emid_, toString(name_), *vals );

	if ( !res || mnuid==horvariogrammnuitem_.id )
	    return;

	visserv->setSelSpec( visid, attribnr,
		Attrib::SelSpec( toString(name_),
				 Attrib::SelSpec::cOtherAttribID()) );
	visserv->setRandomPosData( visid, attribnr, vals );
	changed_ = true;
    }
}


void uiODEarthModelSurfaceDataTreeItem::selectAndLoadAuxData()
{
    if ( !applMgr()->EMServer()->showLoadAuxDataDlg(emid_) )
	return;

    mDynamicCastGet(visSurvey::HorizonDisplay*,vishor,
		    applMgr()->visServer()->getObject(displayID()) );
    if ( !vishor )
	return;

    const StepInterval<int>& grrg = vishor->geometryRowRange();
    const StepInterval<int>& gcrg = vishor->geometryColRange();
    const visBase::HorizonSection* horsect = vishor->getSection( 0 );
    StepInterval<int> loadrrg = horsect ? horsect->displayedRowRange()
					: grrg;
    StepInterval<int> loadcrg = horsect ? horsect->displayedColRange()
					: gcrg;
    TrcKeyZSampling cs( true );
    cs.hsamp_.set( loadrrg, loadcrg );

    TypeSet<float> shifts;
    RefMan<DataPointSet> vals = new DataPointSet( false, true );
    applMgr()->EMServer()->getAllAuxData( emid_, *vals, &shifts, &cs );
    setDataPointSet( *vals );
    vishor->setAttribShift( attribNr(), shifts );

    updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiODEarthModelSurfaceDataTreeItem::setDataPointSet(
						const DataPointSet& vals )
{
    const int visid = displayID();
    const int attribnr = attribNr();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    FixedString attrnm = vals.nrCols()>1 ? vals.colName(1) : "";
    visserv->setSelSpec( visid, attribnr,
	    Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttribID()) );
    visserv->setRandomPosData( visid, attribnr, &vals );
    visserv->selectTexture( visid, attribnr, 0 );
    updateColumnText( uiODSceneMgr::cNameColumn() );
}


uiString uiODEarthModelSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );

    if ( as && as->id() == Attrib::SelSpec::cNoAttribID() )
        return uiStrings::sZValue(mPlural);

     if ( as && as->userRef() && *as->userRef() != 0 )
	 return toUiString( as->userRef() );

    return uiODAttribTreeItem::createDisplayName();
}
