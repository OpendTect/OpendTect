/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodhortreeitem.h"

#include "datapointset.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "emsurfaceauxdata.h"
#include "mpeengine.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "zaxistransform.h"

#include "uiattribpartserv.h"
#include "uicalcpoly2horvol.h"
#include "uidatapointsetpickdlg.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uihor2dfrom3ddlg.h"
#include "uihorizonrelations.h"
#include "uiisopachmaker.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodcontourtreeitem.h"
#include "uiodscenemgr.h"
#include "uiposprovider.h"
#include "uistrings.h"
#include "uivisemobj.h"
#include "uivispartserv.h"

#include "visemobjdisplay.h"
#include "vishorizon2ddisplay.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"


#define mAddIdx		0
#define mAddAtSectIdx	1
#define mAddCBIdx	2
#define mNewIdx		3
#define mCreateIdx	4
#define mSectIdx	5
#define mFullIdx	6
#define mSectFullIdx	7
#define mSortIdx	8

#define mTrackIdx	100
#define mConstIdx	10

static const char* sKeyContours = "Contours";


CNotifier<uiODHorizonParentTreeItem,uiMenu*>&
	uiODHorizonParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODHorizonParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODHorizonParentTreeItem::uiODHorizonParentTreeItem()
    : uiODParentTreeItem(tr("3D Horizon"))
    , handleMenu(this)
    , newmenu_(uiStrings::sNew())
    , trackitem_(m3Dots(tr("Auto and Manual Tracking")),mTrackIdx)
    , constzitem_(m3Dots(tr("With Constant Z")),mConstIdx)
{
    if ( SI().has3D() )
	newmenu_.addItem( &trackitem_ );

    newmenu_.addItem( &constzitem_ );
}


uiODHorizonParentTreeItem::~uiODHorizonParentTreeItem()
{
}


const char* uiODHorizonParentTreeItem::iconName() const
{ return "tree-horizon3d"; }


void uiODHorizonParentTreeItem::removeChild( uiTreeItem* itm )
{
    uiTreeItem::removeChild( itm );
}


static void setSectionDisplayRestoreForAllHors( const uiVisPartServer& visserv,
						bool yn )
{
    TypeSet<VisID> ids;
    visserv.findObject( typeid(visSurvey::HorizonDisplay), ids );
    for ( const auto& id : ids )
    {
	RefMan<visSurvey::HorizonDisplay> hd =
	    dCast( visSurvey::HorizonDisplay*, visserv.getObject(id) );
	if ( hd )
	    hd->setSectionDisplayRestore( yn );
    }
}


bool uiODHorizonParentTreeItem::showSubMenu()
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    const bool hastransform = visserv->getZAxisTransform( sceneID() );

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAddIdx );
    mnu.insertAction( new uiAction(m3Dots(tr("Add at Sections Only"))),
		    mAddAtSectIdx);
    mnu.insertAction( new uiAction(m3Dots(tr("Add Color Blended"))), mAddCBIdx);

    auto* newmenu = new uiMenu( newmenu_ );
    mnu.addMenu( newmenu );
    newmenu->setEnabled( !hastransform );
    showMenuNotifier().trigger( &mnu, this );

    if ( children_.size() )
    {
	mnu.insertAction( new uiAction(m3Dots(tr("Sort"))), mSortIdx );
	mnu.insertSeparator();
	auto* displaymnu = new uiMenu( getUiParent(), tr("Display All") );
	displaymnu->insertAction( new uiAction(tr("Only at Sections")),
				mSectIdx );
	displaymnu->insertAction( new uiAction(tr("In Full")), mFullIdx );
	displaymnu->insertAction( new uiAction(tr("At Sections and in Full")),
				mSectFullIdx );
	mnu.addMenu( displaymnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    handleMenu.trigger( mnuid );
    if ( mnuid == mAddIdx || mnuid==mAddAtSectIdx || mnuid==mAddCBIdx )
    {
	setSectionDisplayRestoreForAllHors( *visserv, true );

	RefObjectSet<EM::EMObject> objs;
	const ZDomain::Info* zinfo = nullptr;
	if ( !hastransform )
	    zinfo = &SI().zDomainInfo();

	applMgr()->EMServer()->selectHorizons( objs, false, nullptr, zinfo );

	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );
	    if ( MPE::engine().getTrackerByObject(objs[idx]->id()) != -1 )
	    {
		MPE::engine().addTracker( objs[idx] );
		applMgr()->visServer()->turnSeedPickingOn( true );
	    }

	    auto* itm = new uiODHorizonTreeItem( objs[idx]->id(),
				    mnuid==mAddCBIdx, mnuid==mAddAtSectIdx );
	    addChld( itm, false, false );
	}

	setSectionDisplayRestoreForAllHors( *visserv, false );
    }
    else if ( mnuid == mSortIdx )
    {
	uiHorizonRelationsDlg dlg( getUiParent(), false );
	dlg.go();
	sort();
    }
    else if ( mnuid == trackitem_.id )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
				applMgr()->attrServer()->curDescSet(false) );

	mps->addTracker( EM::Horizon3D::typeStr(), sceneID() );
	return true;
    }
    else if ( mnuid == mSectIdx || mnuid == mFullIdx || mnuid == mSectFullIdx )
    {
	const bool onlyatsection = mnuid == mSectIdx;
	const bool both = mnuid == mSectFullIdx;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	    if ( !itm || !itm->visEMObject() )
		continue;

	    const VisID displayid = itm->visEMObject()->id();
	    RefMan<visSurvey::HorizonDisplay> hd =
				    dCast( visSurvey::HorizonDisplay*,
					   visserv->getObject(displayid) );
	    if ( !hd )
		continue;

	    hd->displayIntersectionLines( both );
	    hd->setOnlyAtSectionsDisplay( onlyatsection );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else if ( mnuid == constzitem_.id )
    {
	applMgr()->EMServer()->createHorWithConstZ( false );
    }
    else
	handleStandardItems( mnuid );

    return true;
}


static uiTreeItem* gtItm( const MultiID& mid, ObjectSet<uiTreeItem>& itms )
{
    for ( int idx=0; idx<itms.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,itms[idx])
	const EM::ObjectID emid = itm && itm->visEMObject() ?
		     itm->visEMObject()->getObjectID() : EM::ObjectID::udf();
	if ( mid == EM::EMM().getMultiID(emid) )
	    return itms[idx];
    }

    return nullptr;
}


void uiODHorizonParentTreeItem::sort()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    TypeSet<MultiID> mids, sortedmids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	if ( !itm || !itm->visEMObject() )
	    continue;

	const EM::ObjectID emid = itm->visEMObject()->getObjectID();
	mids += EM::EMM().getMultiID( emid );
    }

    EM::IOObjInfo::sortHorizonsOnZValues( mids, sortedmids );
    uiTreeItem* previtm = nullptr;
    for ( int idx=sortedmids.size()-1; idx>=0; idx-- )
    {
	uiTreeItem* itm = gtItm( sortedmids[idx], children_ );
	if ( !itm ) continue;

	if ( !previtm )
	    itm->moveItemToTop();
	else
	    itm->moveItem( previtm );

	previtm = itm;
    }
}


bool uiODHorizonParentTreeItem::addChld( uiTreeItem* child, bool below,
					  bool downwards )
{
    bool res = uiTreeItem::addChld( child, below, downwards );
    if ( !getMoreObjectsToDoHint() )
	sort();

    return res;
}


uiTreeItem* uiODHorizonTreeItemFactory::createForVis( const VisID& visid,
						      uiTreeItem* ) const
{
    ConstRefMan<visSurvey::HorizonDisplay> hd =
	dCast( visSurvey::HorizonDisplay*,
	       ODMainWin()->applMgr().visServer()->getObject(visid) );
    if ( hd )
    {
	mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*, rgba,
			 hd->getChannels2RGBA() );
	const bool atsection = hd->displayedOnlyAtSections();
	return new uiODHorizonTreeItem( visid, rgba, atsection, true );
    }

    return nullptr;
}


// uiODHorizonTreeItem

uiODHorizonTreeItem::uiODHorizonTreeItem( const EM::ObjectID& emid, bool rgba,
					  bool atsect )
    : uiODEarthModelSurfaceTreeItem(emid)
    , rgba_(rgba)
    , atsections_(atsect)
{
    initMenuItems();
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const VisID& visid, bool rgba,
					  bool atsect, bool /* dummy */ )
    : uiODEarthModelSurfaceTreeItem(EM::ObjectID::udf())
    , rgba_(rgba)
    , atsections_(atsect)
{
    initMenuItems();
    displayid_ = visid;
}


uiODHorizonTreeItem::~uiODHorizonTreeItem()
{
    detachAllNotifiers();
    delete dpspickdlg_;
}


void uiODHorizonTreeItem::initMenuItems()
{
    hordatamnuitem_.text = tr("Horizon Data");
    algomnuitem_.text = uiStrings::sTools();
    workflowsmnuitem_.text = tr("Workflows");
    positionmnuitem_.text = m3Dots(uiStrings::sPosition());
    shiftmnuitem_.text = m3Dots(uiStrings::sShift());
    fillholesmnuitem_.text = m3Dots(uiStrings::sGridding());
    filterhormnuitem_.text = m3Dots(uiStrings::sFiltering());
    snapeventmnuitem_.text = m3Dots(tr("Snapping"));
    geom2attrmnuitem_.text = m3Dots(tr("Store Z as Attribute"));
    flatcubemnuitem_.text = m3Dots(tr("Write Flattened Cube"));
    isochronmnuitem_.text = m3Dots(tr("Calculate Isochron"));
    calcvolmnuitem_.text = m3Dots(tr("Calculate Volume"));
    pickdatamnuitem_.text = m3Dots(tr("Pick Horizon Data"));

    parentsrdlmnuitem_.text = tr("Show Parents Path");
    parentsmnuitem_.text = tr("Select Parents");
    childrenmnuitem_.text = tr("Select Children");
    delchildrenmnuitem_.text = tr("Delete Selected Children");
    lockmnuitem_.text = uiStrings::sLock();
    unlockmnuitem_.text = uiStrings::sUnlock();

    addinlitm_.text = tr("Add In-line");
    addinlitm_.placement = 10004;
    addcrlitm_.text = tr("Add Cross-line");
    addcrlitm_.placement = 10003;
    addzitm_.text = tr("Add Z-slice");
    addzitm_.placement = 10002;

    addcontouritm_.text = tr("Add Contour Display");
}


ConstRefMan<visSurvey::HorizonDisplay> uiODHorizonTreeItem::getDisplay() const
{
    return mSelf().getDisplay();
}


RefMan<visSurvey::HorizonDisplay> uiODHorizonTreeItem::getDisplay()
{
    if ( !visEMObject() || !visEMObject()->isOK() )
	return nullptr;

    return visEMObject()->getHorizon3DDisplay();
}


void uiODHorizonTreeItem::initNotify()
{
    ConstRefMan<visSurvey::HorizonDisplay> hd = getDisplay();
    if ( hd )
	mAttachCB( hd->changedisplay, uiODHorizonTreeItem::dispChangeCB );
}


uiString uiODHorizonTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		    const_cast<uiODHorizonTreeItem*>(this)->visserv_;

    uiString res = cvisserv->getUiObjectName( displayid_ );
    if ( res.isEmpty() )
    {
	pErrMsg("Horizon name not found");
	return res;
    }

    if (  uivisemobj_ && uivisemobj_->getShift() )
    {
	res.append( toUiString(" (%1)").arg(
	  toUiString(uivisemobj_->getShift() * SI().zDomain().userFactor())));
    }

    return res;
}


bool uiODHorizonTreeItem::init()
{
    if ( !createUiVisObj() )
	return false;

    RefMan<visSurvey::HorizonDisplay> hd = getDisplay();
    if ( rgba_ )
    {
	if ( !hd )
	    return false;

	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 hd->getChannels2RGBA() );
	if ( !rgba )
	{
	    RefMan<visBase::RGBATextureChannel2RGBA> txt2rgba =
				visBase::RGBATextureChannel2RGBA::create();
	    if ( !hd->setChannels2RGBA(txt2rgba.ptr()) )
		return false;

	    hd->addAttrib();
	    hd->addAttrib();
	    hd->addAttrib();
	}
    }

    if ( hd )
	hd->setOnlyAtSectionsDisplay( atsections_ );

    const bool res = uiODEarthModelSurfaceTreeItem::init();
    if ( !res )
	return res;

    mDynamicCastGet(const EM::Horizon3D*,hor3d,EM::EMM().getObject(emid_))
    if ( hor3d )
    {
	const int trackerid = MPE::engine().getTrackerByObject( hor3d->id() );
	const MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

	if ( !applMgr()->isRestoringSession() )
	{
	    hd->setDepthAsAttrib( 0 );
	    const bool istracking = tracker && tracker->isEnabled();
	    const int nrauxdata = hor3d->auxdata.nrAuxData();
	    for ( int idx=0; !rgba_ && idx<nrauxdata; idx++ )
	    {
		if ( !hor3d->auxdata.auxDataName(idx) )
		    continue;

		const EM::SurfaceAuxData::AuxDataType auxdatatype =
					    hor3d->auxdata.getAuxDataType(idx);

		if ( auxdatatype==EM::SurfaceAuxData::AutoShow ||
		     (auxdatatype==EM::SurfaceAuxData::Tracking && istracking) )
		{
		    RefMan<DataPointSet> vals = new DataPointSet( false, true );
		    float shift;
		    applMgr()->EMServer()->getAuxData(emid_, idx, *vals, shift);

		    uiODDataTreeItem* itm = addAttribItem();
		    mDynamicCastGet( uiODEarthModelSurfaceDataTreeItem*,
				     emitm, itm );
		    if ( emitm )
			emitm->setDataPointSet( *vals );
		}
	    }
	}

	if ( tracker )
	{
	    for ( int idx=0; idx<nrChildren(); idx++ )
		getChild(idx)->setChecked( false, true );
	}
    }

    if ( rgba_ && !applMgr()->isRestoringSession() )
	selectRGBA( Pos::GeomID::udf() );

    return res;
}


void uiODHorizonTreeItem::dispChangeCB(CallBacker*)
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODHorizonTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODHorizonTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODEarthModelSurfaceTreeItem::createMenu( menu, istb );
    if ( istb )
	return;

    mDynamicCastGet(uiMenuHandler*,uimenu,menu)
    const bool hastransform = visserv_->getZAxisTransform( sceneID() );

    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    const bool islocked = visserv_->isLocked( displayID() );
    const bool canadd = !islocked && visserv_->canAddAttrib( displayID() );

    mAddMenuItem( &addmnuitem_, &addcontouritm_, canadd, false );
    mAddMenuItem( &addmnuitem_, &hordatamnuitem_, canadd, false );

    if ( hastransform )
    {
	mResetMenuItem( &positionmnuitem_ );
	mResetMenuItem( &shiftmnuitem_ );
	mResetMenuItem( &fillholesmnuitem_ );
	mResetMenuItem( &filterhormnuitem_ );
	mResetMenuItem( &snapeventmnuitem_ );
	mResetMenuItem( &geom2attrmnuitem_ );
	mResetMenuItem( &createflatscenemnuitem_ );
	mResetMenuItem( &flatcubemnuitem_ );
	mResetMenuItem( &isochronmnuitem_ );
	mResetMenuItem( &calcvolmnuitem_ );
	mResetMenuItem( &pickdatamnuitem_ );
	return;
    }

    mAddMenuItem( &displaymnuitem_, &positionmnuitem_, true, false );
    mAddMenuItem(
	menu, &algomnuitem_, !MPE::engine().trackingInProgress(), false );
    mAddMenuItem( &algomnuitem_, &filterhormnuitem_, !islocked, false );
    mAddMenuItem( &algomnuitem_, &fillholesmnuitem_, !islocked, false );
    mAddMenuItem( &algomnuitem_, &shiftmnuitem_, !islocked, false )
    mAddMenuItem( &algomnuitem_, &snapeventmnuitem_, !islocked, false );
    mAddMenuItem( &algomnuitem_, &geom2attrmnuitem_, !islocked, false );

    mAddMenuItem( menu, &workflowsmnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &createflatscenemnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &flatcubemnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &isochronmnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &calcvolmnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &pickdatamnuitem_, true, false );

    const bool hastracker = MPE::engine().getTrackerByObject(emid_) >= 0;
    MenuItem* trackmnu = menu->findItem(
	    uiStrings::sTracking().getFullString().buf() );
    if ( hastracker && trackmnu )
    {
	const Coord3& crd = uimenu->getPickedPos();
	if ( crd.isDefined() )
	{
	    mAddMenuItem( trackmnu, &parentsmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &parentsrdlmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &childrenmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &delchildrenmnuitem_, true, false );
	}
	else
	{
	    mResetMenuItem( &parentsmnuitem_ );
	    mResetMenuItem( &childrenmnuitem_ );
	    mResetMenuItem( &delchildrenmnuitem_ );
	    mResetMenuItem( &parentsrdlmnuitem_ );
	}

	mAddMenuItem( trackmnu, &lockmnuitem_, true, false );
	mAddMenuItem( trackmnu, &unlockmnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &parentsmnuitem_ );
	mResetMenuItem( &childrenmnuitem_ );
	mResetMenuItem( &delchildrenmnuitem_ );
	mResetMenuItem( &parentsrdlmnuitem_ );
	mResetMenuItem( &lockmnuitem_ );
	mResetMenuItem( &unlockmnuitem_ );
    }

    if ( uimenu->getMenuType() != uiMenuHandler::fromScene() )
    {
	mResetMenuItem( &addinlitm_ );
	mResetMenuItem( &addcrlitm_ );
	mResetMenuItem( &addzitm_ );
    }
    else
    {
	const Coord3 pickedpos = uimenu->getPickedPos();
	const TrcKey tk( SI().transform(pickedpos) );
	addinlitm_.text = tr("Add In-line %1").arg( tk.lineNr() );
	addcrlitm_.text = tr("Add Cross-line %1").arg( tk.trcNr() );

        float zpos( pickedpos.z_ );
	SI().snapZ( zpos );
	BufferString zposstr;
	zposstr.set( zpos, SI().nrZDecimals() );
	addzitm_.text = tr("Add Z-slice %1").arg( zposstr.buf() );

	mAddMenuItem( menu, &addinlitm_, true, false );
	mAddMenuItem( menu, &addcrlitm_, true, false );
	mAddMenuItem( menu, &addzitm_, true, false );
    }
}


#define mUpdateTexture() \
{ \
    for ( int idx=0; idx<hd->nrAttribs(); idx++ ) \
    { \
	if ( hd->hasDepth(idx) ) hd->setDepthAsAttrib( idx ); \
	else applMgr()->calcRandomPosAttrib( visid, idx ); \
    } \
}

void uiODHorizonTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller)
    mDynamicCastGet(uiMenuHandler*,uimenu,caller)
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

     if ( mnuid == fillholesmnuitem_.id || mnuid == snapeventmnuitem_.id ||
	  mnuid == filterhormnuitem_.id || mnuid == geom2attrmnuitem_.id )
     {
	if ( !askSave() )
	    return;
     }

     if ( !uivisemobj_ || !uivisemobj_->isOK() )
	 return;

    const VisID visid = displayID();
    RefMan<visSurvey::HorizonDisplay> hd = getDisplay();
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(emid_))
    if ( !hd || !hor3d )
	return;

    uiEMPartServer* emserv = applMgr()->EMServer();
    uiEMAttribPartServer* emattrserv = applMgr()->EMAttribServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool handled = true;
    if ( mnuid==fillholesmnuitem_.id )
    {
	const bool isoverwrite = emserv->fillHoles( emid_, false );
	if ( isoverwrite ) { mUpdateTexture(); }
    }
    else if ( mnuid==filterhormnuitem_.id )
    {
	const bool isoverwrite = emserv->filterSurface( emid_ );
	if ( isoverwrite ) { mUpdateTexture(); }
    }
    else if ( mnuid==snapeventmnuitem_.id )
	emattrserv->snapHorizon( emid_, false );
    else if ( mnuid==flatcubemnuitem_.id )
    {
	emattrserv->createHorizonOutput( uiEMAttribPartServer::FlattenSingle,
					 hor3d->multiID() );
    }
    else if ( mnuid==isochronmnuitem_.id )
    {
	uiIsochronMakerDlg dlg( getUiParent(), emid_ );
	if ( !dlg.go() )
	    return;

	uiODDataTreeItem* itm = addAttribItem();
	mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm)
	if ( emitm )
	    emitm->setDataPointSet( dlg.getDPS() );
    }
    else if ( mnuid==calcvolmnuitem_.id )
    {
	uiCalcHorPolyVol dlg( getUiParent(), *hor3d );
	dlg.go();
    }
    else if ( mnuid==pickdatamnuitem_.id )
    {
	delete dpspickdlg_;
	dpspickdlg_ = new uiEMDataPointSetPickDlg( getUiParent(), visserv_,
						   hd->getSceneID(), emid_ );
	mAttachCB( dpspickdlg_->readyForDisplay,
		   uiODHorizonTreeItem::dataReadyCB );
	dpspickdlg_->show();
    }
    else if ( mnuid==geom2attrmnuitem_.id )
    {
	if ( applMgr()->EMServer()->geom2Attr(emid_) )
	    mUpdateTexture();
    }
    else if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);

	visBase::HorizonSection* section = hd->getHorizonSection();
	if ( !section )
	    return;

	TrcKeyZSampling maxcs = SI().sampling(true);;
	ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	if ( scene && scene->getZAxisTransform() )
	{
	    const ZSampling zintv =
		scene->getZAxisTransform()->getZInterval( false );
	    maxcs.zsamp_.start_ = zintv.start_;
	    maxcs.zsamp_.stop_ = zintv.stop_;
	}

	TrcKeyZSampling curcs;
	curcs.zsamp_ = SI().zRange( true );
	curcs.hsamp_.set( section->displayedRowRange(),
		       section->displayedColRange() );

	uiPosProvider::Setup setup( false, true, false );
	setup.allownone_ = true;
	setup.seltxt( tr("Area subselection") );
	setup.tkzs_ = maxcs;

	uiDialog dlg( getUiParent(),
	uiDialog::Setup( uiStrings::sPosition(mPlural),
			    tr("Specify positions"),
				mODHelpKey(mPosProvSelHelpID) ) );
	uiPosProvider pp( &dlg, setup );

	IOPar displaypar;
	pp.fillPar( displaypar );	//Get display type
	curcs.fillPar( displaypar );	//Get display ranges
	pp.usePar( displaypar );

	if ( !dlg.go() )
	    return;

	MouseCursorChanger cursorlock( MouseCursor::Wait );
	pp.fillPar( displaypar );

	TrcKeyZSampling newcs;
	if ( pp.isAll() )
	    newcs = maxcs;
	else
	    newcs.usePar( displaypar );

	section->setUsingNeighborsInIsolatedLine( false );
	section->setDisplayRange( newcs.hsamp_.inlRange(),
				  newcs.hsamp_.crlRange() );
	emserv->setHorizon3DDisplayRange( newcs.hsamp_ );

	for ( int idx=0; idx<hd->nrAttribs(); idx++ )
	{
	    if ( hd->hasDepth(idx) )
		hd->setDepthAsAttrib( idx );
	    else
		applMgr()->calcRandomPosAttrib( visid, idx );
	}
    }
    else if ( mnuid==shiftmnuitem_.id )
    {
	BoolTypeSet isenabled;
	const int nrattrib = visserv_->getNrAttribs( visid );
	for ( int idx=0; idx<nrattrib; idx++ )
	    isenabled += visserv_->isAttribEnabled( visid, idx );

        float curshift = sCast(float,visserv_->getTranslation( visid ).z_);
	if ( mIsUdf(curshift) )
	    curshift = 0;

	emattrserv->setDescSet( attrserv->curDescSet(false) );
	emattrserv->showHorShiftDlg( emid_, visid, isenabled, curshift,
				     visserv_->canAddAttrib(visid,1) );
    }
    else if ( mnuid==hordatamnuitem_.id )
    {
	uiODDataTreeItem* itm = addAttribItem();
	mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm);
	if ( emitm ) emitm->selectAndLoadAuxData();
    }
    else if ( mnuid==parentsrdlmnuitem_.id )
    {
	const TrcKey tk( SI().transform( uimenu->getPickedPos() ) );
	applMgr()->addMPEParentPath( hd->id(), tk );
    }
    else if ( mnuid==parentsmnuitem_.id )
    {
	const TrcKey tk( SI().transform( uimenu->getPickedPos() ) );
	hd->selectParent( tk );
    }
    else if ( mnuid==childrenmnuitem_.id )
    {
	const TrcKey tk( SI().transform( uimenu->getPickedPos() ) );
	hor3d->selectChildren( tk );
    }
    else if ( mnuid==delchildrenmnuitem_.id )
    {
	hor3d->deleteChildren();
	hd->showSelections( false );
    }
    else if ( mnuid==lockmnuitem_.id )
	hor3d->lockAll();
    else if ( mnuid==unlockmnuitem_.id )
	hor3d->unlockAll();
    else if ( mnuid==addinlitm_.id ||
	      mnuid==addcrlitm_.id ||
	      mnuid==addzitm_.id )
    {
	const Coord3 pickedpos = uimenu->getPickedPos();
	if ( mnuid==addzitm_.id )
	{
            float zpos( pickedpos.z_ );
	    SI().snapZ( zpos );
	    TrcKeyZSampling tkzs = SI().sampling( true );
	    tkzs.zsamp_.set( zpos, zpos, SI().zStep() );
	    ODMainWin()->sceneMgr().addZSliceItem( tkzs, sceneID() );
	}
	else
	{
	    const TrcKey tk( SI().transform(pickedpos) );
	    const bool isinl = mnuid == addinlitm_.id;
	    ODMainWin()->sceneMgr().addInlCrlItem(
		    isinl ? OD::SliceType::Inline : OD::SliceType::Crossline,
		    isinl ? tk.inl() : tk.crl(), sceneID() );
	}
    }
    else if ( mnuid==addcontouritm_.id )
    {
	const MultiID mid = EM::EMM().getMultiID( emid_ );
	const EM::IOObjInfo eminfo( mid );
	if ( !eminfo.isOK() )
	{
	    uiMSG().error(
		    tr("Cannot find this horizon in project's database.\n"
		    "If this is a newly tracked horizon, please save first\n"
		    "before contouring.") );
	    return;
	}

	const BufferString attrnm =
		uiContourTreeItem::selectAttribute( getUiParent(), mid );
	if ( attrnm.isEmpty() )
	    return;

	const int attrib = visserv_->addAttrib( visid );
	Attrib::SelSpec spec( sKeyContours, Attrib::SelSpec::cAttribNotSel(),
			      false, 0 );
	spec.setZDomainKey( attrnm );
	//spec.setZDomainUnit( ?? );
	spec.setDefString( uiContourTreeItem::sKeyContourDefString() );
	visserv_->setSelSpec( visid, attrib, spec );

	auto* newitem = new uiContourTreeItem( typeid(*this).name() );
	newitem->setAttribName( attrnm );
	addChild( newitem, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else
	handled = false;

    menu->setIsHandled( handled );
}


VisID uiODHorizonTreeItem::reloadEMObject()
{
    const bool wasonlyatsections = uivisemobj_->isOnlyAtSections();
    uiEMPartServer* ems = applMgr()->EMServer();
    const MultiID mid = ems->getStorageID( emid_ );

    removeAllChildren();
    deleteAndNullPtr( uivisemobj_ );
    if ( !ems->loadSurface(mid,true) )
	return VisID::udf();

    emid_ = applMgr()->EMServer()->getObjectID(mid);
    uivisemobj_ = new uiVisEMObject( ODMainWin(), emid_, sceneID(), visserv_ );
    displayid_ = uivisemobj_->id();

    RefMan<visSurvey::HorizonDisplay> hd = getDisplay();
    if ( hd )
    {
	hd->setDepthAsAttrib( 0 );
	const Attrib::SelSpec* as = visserv_->getSelSpec(displayid_,0);
	uiODDataTreeItem* item = createAttribItem( as );
	if ( item )
	    addChild( item, false );
    }

    const EM::IOObjInfo eminfo( mid );
    timelastmodified_ = eminfo.timeLastModified( true );
    uivisemobj_->setOnlyAtSectionsDisplay( wasonlyatsections );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return displayid_;
}


void uiODHorizonTreeItem::dataReadyCB( CallBacker* )
{
    uiODDataTreeItem* itm = addAttribItem();
    mDynamicCastGet(uiODEarthModelSurfaceDataTreeItem*,emitm,itm);
    if ( emitm && dpspickdlg_ )
    {
	ConstRefMan<DataPointSet> dps = dpspickdlg_->getData();
	if ( dps )
	    emitm->setDataPointSet( *dps );
    }
}


// uiODHorizon2DParentTreeItem

CNotifier<uiODHorizon2DParentTreeItem,uiMenu*>&
	uiODHorizon2DParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODHorizon2DParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODHorizon2DParentTreeItem::uiODHorizon2DParentTreeItem()
    : uiODParentTreeItem(tr("2D Horizon"))
{}


uiODHorizon2DParentTreeItem::~uiODHorizon2DParentTreeItem()
{}


const char* uiODHorizon2DParentTreeItem::iconName() const
{ return "tree-horizon2d"; }


void uiODHorizon2DParentTreeItem::removeChild( uiTreeItem* itm )
{
    uiTreeItem::removeChild( itm );
}


bool uiODHorizon2DParentTreeItem::showSubMenu()
{
    RefMan<visSurvey::Scene> scene =
		    ODMainWin()->applMgr().visServer()->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction( m3Dots(uiStrings::sAdd()) ), 0 );
    auto* newmenu = new uiAction( m3Dots(tr("Track New")) );
    mnu.insertAction( newmenu, 1 );
    mnu.insertAction( new uiAction(m3Dots(tr("Create from 3D"))), 2 );
    newmenu->setEnabled( !hastransform );
    showMenuNotifier().trigger( &mnu, this );

    if ( children_.size() )
    {
	mnu.insertAction( new uiAction(m3Dots(tr("Sort"))), mSortIdx );
	mnu.insertSeparator();
	mnu.insertAction( new uiAction(tr("Display All Only at Sections")), 3 );
	mnu.insertAction( new uiAction(tr("Show All in Full")), 4 );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	ObjectSet<EM::EMObject> objs;
	const ZDomain::Info* zinfo = nullptr;
	if ( !hastransform )
	    zinfo = &SI().zDomainInfo();

	applMgr()->EMServer()->selectHorizons( objs, true, nullptr, zinfo );
	for ( int idx=0; idx<objs.size(); idx++ )
	{
	    setMoreObjectsToDoHint( idx<objs.size()-1 );

	    if ( MPE::engine().getTrackerByObject(objs[idx]->id()) != -1 )
	    {
		MPE::engine().addTracker( objs[idx] );
		applMgr()->visServer()->turnSeedPickingOn( true );
	    }
	    addChld( new uiODHorizon2DTreeItem(objs[idx]->id()), false, false);
	}

	deepUnRef( objs );
    }
    else if ( mnuid == 1 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet(
			applMgr()->attrServer()->curDescSet(true) );

	mps->addTracker( EM::Horizon2D::typeStr(), sceneID() );
	return true;
    }
    else if ( mnuid == 2 )
    {
	uiHor2DFrom3DDlg dlg( getUiParent() );
	if ( dlg.go() && dlg.doDisplay() )
	{
	    for ( const auto& objid : dlg.getEMObjIDs() )
		addChld( new uiODHorizon2DTreeItem(objid), true, false);
	}
    }
    else if ( mnuid == 3 || mnuid == 4 )
    {
	const bool onlyatsection = mnuid == 3;
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiODHorizon2DTreeItem*,itm,children_[idx])
	    if ( itm && itm->visEMObject() )
	    {
		itm->visEMObject()->setOnlyAtSectionsDisplay( onlyatsection );
		itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	    }
	}
    }
    else if ( mnuid == mSortIdx )
    {
	uiHorizonRelationsDlg dlg( getUiParent(), true );
	dlg.go();
	sort();
    }
    else
	handleStandardItems( mnuid );

    return true;
}


void uiODHorizon2DParentTreeItem::sort()
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    TypeSet<MultiID> mids, sortedmids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiODEarthModelSurfaceTreeItem*,itm,children_[idx])
	if ( !itm || !itm->visEMObject() )
	    continue;

	const EM::ObjectID emid = itm->visEMObject()->getObjectID();
	mids += EM::EMM().getMultiID( emid );
    }

    EM::IOObjInfo::sortHorizonsOnZValues( mids, sortedmids );
    uiTreeItem* previtm = nullptr;
    for ( int idx=sortedmids.size()-1; idx>=0; idx-- )
    {
	uiTreeItem* itm = gtItm( sortedmids[idx], children_ );
	if ( !itm ) continue;

	if ( !previtm )
	    itm->moveItemToTop();
	else
	    itm->moveItem( previtm );

	previtm = itm;
    }
}


bool uiODHorizon2DParentTreeItem::addChld( uiTreeItem* child, bool below,
					    bool downwards )
{
    bool res = uiTreeItem::addChld( child, below, downwards );
    if ( !getMoreObjectsToDoHint() )
	sort();

    return res;
}


uiTreeItem* uiODHorizon2DTreeItemFactory::createForVis( const VisID& visid,
							uiTreeItem* ) const
{
    ConstRefMan<visSurvey::Horizon2DDisplay> hd =
	dCast( visSurvey::Horizon2DDisplay*,
	       ODMainWin()->applMgr().visServer()->getObject(visid) );
    return hd ? new uiODHorizon2DTreeItem( visid, true ) : nullptr;
}


// uiODHorizon2DTreeItem

uiODHorizon2DTreeItem::uiODHorizon2DTreeItem( const EM::ObjectID& objid )
    : uiODEarthModelSurfaceTreeItem( objid )
{
    initMenuItems();
}


uiODHorizon2DTreeItem::uiODHorizon2DTreeItem( const VisID& id, bool /*dummy */ )
    : uiODEarthModelSurfaceTreeItem( EM::ObjectID::udf() )
{
    initMenuItems();
    displayid_ = id;
}


uiODHorizon2DTreeItem::~uiODHorizon2DTreeItem()
{
    detachAllNotifiers();
}


void uiODHorizon2DTreeItem::initMenuItems()
{
    algomnuitem_.text = uiStrings::sTools();
    workflowsmnuitem_.text = tr("Workflows");
    shiftmnuitem_.text = m3Dots(uiStrings::sShift());
    derive3dhormnuitem_.text = m3Dots(tr("Derive 3D horizon"));
    snapeventmnuitem_.text = m3Dots(tr("Snapping"));
    interpolatemnuitem_.text = m3Dots(tr("Interpolate"));
}


ConstRefMan<visSurvey::Horizon2DDisplay>
uiODHorizon2DTreeItem::getDisplay() const
{
    return mSelf().getDisplay();
}


RefMan<visSurvey::Horizon2DDisplay> uiODHorizon2DTreeItem::getDisplay()
{
    if ( !visEMObject() || !visEMObject()->isOK() )
	return nullptr;

    return visEMObject()->getHorizon2DDisplay();
}


void uiODHorizon2DTreeItem::initNotify()
{
    ConstRefMan<visSurvey::Horizon2DDisplay> hd = getDisplay();
    if ( hd )
	mAttachCB( hd->changedisplay, uiODHorizon2DTreeItem::dispChangeCB );
}


uiString uiODHorizon2DTreeItem::createDisplayName() const
{
    uiString res = visserv_->getUiObjectName( displayid_ );
    if ( res.isEmpty() )
    {
	pErrMsg("Horizon name not found");
	return res;
    }

    const float curshift =
            sCast(float,visserv_->getTranslation( displayid_ ).z_);

    if ( !mIsZero(curshift,1e-6) )
	res.append( toUiString("(%1)").arg(
		curshift * SI().zDomain().userFactor()) );

    return res;
}


void uiODHorizon2DTreeItem::dispChangeCB(CallBacker*)
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODHorizon2DTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emid_, withcancel );
}


void uiODHorizon2DTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODEarthModelSurfaceTreeItem::createMenu( menu, istb );
    if ( istb ) return;

    ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
    const bool hastransform = scene && scene->getZAxisTransform();

    if ( hastransform || !menu || !isDisplayID(menu->menuID()) )
    {
	mResetMenuItem( &derive3dhormnuitem_ );
	mResetMenuItem( &createflatscenemnuitem_ );
	mResetMenuItem( &snapeventmnuitem_ );
	mResetMenuItem( &interpolatemnuitem_ );
	mResetMenuItem( &shiftmnuitem_ );
	return;
    }

    const bool islocked = visserv_->isLocked( displayID() );
    const bool isempty = applMgr()->EMServer()->isEmpty( emid_ );
    const bool enab = !islocked && !isempty;
    mAddMenuItem(
	menu, &algomnuitem_, !MPE::engine().trackingInProgress(), false );
    mAddMenuItem( &algomnuitem_, &snapeventmnuitem_, enab, false );
    mAddMenuItem( &algomnuitem_, &interpolatemnuitem_, enab, false );
    mAddMenuItem( &algomnuitem_, &shiftmnuitem_, !islocked, false )

    mAddMenuItem( menu, &workflowsmnuitem_, true, false );
    mAddMenuItem( &workflowsmnuitem_, &derive3dhormnuitem_, enab, false );
    mAddMenuItem( &workflowsmnuitem_, &createflatscenemnuitem_, enab,false);
}


void uiODHorizon2DTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODEarthModelSurfaceTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    const VisID visid = displayID();
    bool handled = true;
    if ( mnuid==interpolatemnuitem_.id )
    {
	if ( askSave() )
	    applMgr()->EMServer()->fillHoles( emid_, true );
    }
    else if ( mnuid==derive3dhormnuitem_.id )
	applMgr()->EMServer()->deriveHor3DFrom2D( emid_ );
    else if ( mnuid==snapeventmnuitem_.id )
	applMgr()->EMAttribServer()->snapHorizon( emid_,true );
    else if ( mnuid==shiftmnuitem_.id )
    {
	BoolTypeSet isenabled;
        float curshift = sCast(float,visserv_->getTranslation( visid ).z_);
	if ( mIsUdf(curshift) )
	    curshift = 0;

	applMgr()->EMAttribServer()->showHorShiftDlg(
				emid_, visid, isenabled, curshift, false );
    }
    else
	handled = false;

    menu->setIsHandled( handled );
}
