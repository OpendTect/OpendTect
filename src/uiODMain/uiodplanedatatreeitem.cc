/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodplanedatatreeitem.h"

#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uishortcutsmgr.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "uiwellpartserv.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "callback.h"
#include "coltabsequence.h"
#include "keyboardevent.h"
#include "settings.h"
#include "survinfo.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"
#include "welldata.h"
#include "wellman.h"
#include "zaxistransform.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;

static uiODPlaneDataTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODPlaneDataTreeItem::Default; break;
	case 1: return uiODPlaneDataTreeItem::Select; break;
	case 2: return uiODPlaneDataTreeItem::Empty; break;
	case 3: return uiODPlaneDataTreeItem::RGBA; break;
	default: return uiODPlaneDataTreeItem::Empty;
    }
}


uiString uiODPlaneDataTreeItem::sAddEmptyPlane()
{ return tr("Add Empty Plane"); }

uiString uiODPlaneDataTreeItem::sAddAndSelectData()
{ return m3Dots(tr("Add and Select Data")); }

uiString uiODPlaneDataTreeItem::sAddDefaultData()
{ return tr("Add Default Data"); }

uiString uiODPlaneDataTreeItem::sAddColorBlended()
{ return uiStrings::sAddColBlend(); }

uiString uiODPlaneDataTreeItem::sAddAtWellLocation()
{ return m3Dots(tr("Add at Well Location")); }


#define mParentShowSubMenu( treeitm, fromwell ) \
    uiMenu mnu( getUiParent(), uiStrings::sAction() ); \
    mnu.insertAction( \
	new uiAction(uiODPlaneDataTreeItem::sAddDefaultData()), 0 ); \
    mnu.insertAction( \
	new uiAction(uiODPlaneDataTreeItem::sAddAndSelectData()), 1 );\
    if ( fromwell ) \
	mnu.insertAction( \
	    new uiAction(uiODPlaneDataTreeItem::sAddAtWellLocation()), 2 ); \
    mnu.insertAction( \
	new uiAction(uiODPlaneDataTreeItem::sAddColorBlended()), 3 ); \
    showMenuNotifier().trigger( &mnu, this ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 || mnuid==1 || mnuid==3 ) \
    { \
	auto* newitm = new treeitm(VisID::udf(),getType(mnuid));\
	addChild( newitm, false ); \
    } \
    else if ( mnuid==2 ) \
    { \
	TypeSet<MultiID> wellids; \
	if ( !applMgr()->wellServer()->selectWells(wellids) ) \
	    return true; \
	for ( int idx=0; idx<wellids.size(); idx++ ) \
	{ \
	    ConstRefMan<Well::Data> wd = Well::MGR().get( wellids[idx] ); \
	    if ( !wd ) continue; \
	    auto* itm = new treeitm( VisID::udf(),\
				     uiODPlaneDataTreeItem::Empty ); \
	    setMoreObjectsToDoHint( idx<wellids.size()-1 ); \
	    addChild( itm, false ); \
	    itm->setAtWellLocation( *wd ); \
	    itm->displayDefaultData(); \
	} \
    } \
    handleStandardItems( mnuid ); \
    return true


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( const VisID& id,
					      OD::SliceType slicetp, Type tp )
    : orient_(slicetp)
    , type_(tp)
    , positionmnuitem_(m3Dots(tr("Position")),cPositionIdx)
    , gridlinesmnuitem_(m3Dots(tr("Gridlines")),cGridLinesIdx)
    , addinlitem_(tr("Add Inl-line"),10003)
    , addcrlitem_(tr("Add Crl-line"),10002)
    , addzitem_(tr("Add Z-slice"),10001)
{
    displayid_ = id;
    positionmnuitem_.iconfnm = "orientation64";
    gridlinesmnuitem_.iconfnm = "gridlines";
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	       uiODPlaneDataTreeItem::keyUnReDoPressedCB );
}


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    detachAllNotifiers();
    delete positiondlg_;
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODPlaneDataTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::PlaneDataDisplay> pdd =
					    new visSurvey::PlaneDataDisplay;
	displayid_ = pdd->id();
	if ( type_ == RGBA )
	{
	    RefMan<visBase::RGBATextureChannel2RGBA> text2rgba =
			    visBase::RGBATextureChannel2RGBA::create();
	    pdd->setChannels2RGBA( text2rgba.ptr() );
	    pdd->addAttrib();
	    pdd->addAttrib();
	    pdd->addAttrib();
	}

	pdd->setOrientation( orient_);
	visserv_->addObject( pdd.ptr(), sceneID(), true );

	BufferString res;
	Settings::common().get( "dTect.Texture2D Resolution", res );
	for ( int idx=0; idx<pdd->nrResolutions(); idx++ )
	{
	    if ( res == pdd->getResolutionName(idx) )
		pdd->setResolution( idx, 0 );
	}

	if ( type_ == Default )
	    displayDefaultData();
	if ( type_ == Select )
	    displayGuidance();
	if ( type_ == RGBA )
	    selectRGBA( Pos::GeomID::udf() );
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd )
	return false;

    mAttachCB( pdd->selection(), uiODPlaneDataTreeItem::selChg );
    mAttachCB( pdd->deSelection(), uiODPlaneDataTreeItem::selChg );
    mAttachCB( visserv_->getUiSlicePos()->positionChg,
	       uiODPlaneDataTreeItem::posChange );
    mAttachCB( visserv_->getUiSlicePos()->sliderReleased,
	       uiODPlaneDataTreeItem::sliderReleasedCB );

    pdd_ = pdd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::PlaneDataDisplay>
	uiODPlaneDataTreeItem::getDisplay() const
{
    return pdd_.get();
}


RefMan<visSurvey::PlaneDataDisplay> uiODPlaneDataTreeItem::getDisplay()
{
    return pdd_.get();
}


void uiODPlaneDataTreeItem::setAtWellLocation( const Well::Data& wd )
{
    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( !pdd )
	return;

    const Coord surfacecoord = wd.info().surfacecoord_;
    const BinID bid = SI().transform( surfacecoord );
    TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling( true, true );
    if ( orient_ == OD::SliceType::Inline )
	tkzs.hsamp_.setInlRange( Interval<int>(bid.inl(),bid.inl()) );
    else
	tkzs.hsamp_.setCrlRange( Interval<int>(bid.crl(),bid.crl()) );

    pdd->setTrcKeyZSampling( tkzs );
    select();
}


void uiODPlaneDataTreeItem::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( pdd )
	pdd->setTrcKeyZSampling( tkzs );
}


bool uiODPlaneDataTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid) )
	return false;

    const Attrib::SelSpec* prevas = visserv_->getSelSpec( displayid_, 0 );
    const bool notsel = !prevas || prevas->id().isUnselInvalid();
    const bool res = displayDataFromDesc( descid, true );
    if ( res && notsel )
	applMgr()->useDefColTab( displayid_, 0 );

    return res;
}


bool uiODPlaneDataTreeItem::displayGuidance()
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false; //safety

    auto* as = const_cast<Attrib::SelSpec*>(
					visserv_->getSelSpec( displayid_, 0 ));
    if ( !as )
	return false;

    const bool notsel = as->id().isUnselInvalid();

    const Pos::GeomID geomid = visserv_->getGeomID( displayid_ );
    const ZDomain::Info* zdinf =
		    visserv_->zDomainInfo( visserv_->getSceneID(displayid_) );
    const bool issi = !zdinf || zdinf->def_.isSI();
    const bool selok = applMgr()->attrServer()->selectAttrib(
			*as, issi ? 0 : zdinf, geomid, tr("first layer" ) );
    if ( !selok )
	return false;

    if ( as->isNLA()
	 || as->id().asInt()==Attrib::SelSpec::cOtherAttrib().asInt() )
    {
	if ( as->isNLA() )
	    visserv_->setSelSpec( displayid_, 0, *as );
	else
	    visserv_->setSelSpecs( displayid_, 0,
			     applMgr()->attrServer()->getTargetSelSpecs() );

	RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
	if ( !pdd )
	{
	    mDynamicCast(visSurvey::PlaneDataDisplay*,pdd,
			 visserv_->getObject(displayid_));
	    if ( !pdd )
		return false;
	}

	return displayDataFromOther( pdd->id() );
    }

    const bool res = displayDataFromDesc( as->id(), as->isStored() );
    if ( res && notsel )
	applMgr()->useDefColTab( displayid_, 0 );

    return res;
}


bool uiODPlaneDataTreeItem::displayDataFromDesc( const Attrib::DescID& descid,
						 bool isstored )
{
    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, isstored );
    Attrib::SelSpec as( nullptr, descid, false, "" );
    as.setRefFromID( *ads );
    visserv_->setSelSpec( displayid_, 0, as );
    const bool res = visserv_->calculateAttrib( displayid_, 0, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return res;
}


bool uiODPlaneDataTreeItem::displayDataFromDataPack(
					RegularSeisDataPack& dp,
					const Attrib::SelSpec& as,
					const FlatView::DataDispPars::VD& ddp )
{
    visserv_->setSelSpec( displayid_, 0, as );
    visserv_->setColTabMapperSetup( displayid_, 0, ddp.mappersetup_ );
    visserv_->setColTabSequence( displayid_, 0, ColTab::Sequence(ddp.ctab_) );
    visserv_->setRegularSeisDataPack( displayid_, 0, &dp );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return true;
}


bool uiODPlaneDataTreeItem::displayDataFromOther( const VisID& visid )
{
    const int nrattribs = visserv_->getNrAttribs( visid );
    while ( nrattribs > visserv_->getNrAttribs(displayid_) )
	addAttribItem();

    for ( int attrib=0; attrib<nrattribs; attrib++ )
    {
	const TypeSet<Attrib::SelSpec>* as =visserv_->getSelSpecs(visid,attrib);
	if ( !as )
	    return displayDefaultData();

	visserv_->setSelSpecs( displayid_, attrib, *as );
	visserv_->calculateAttrib( displayid_, attrib, false );

	const ColTab::Sequence* ctseq =
		visserv_->getColTabSequence( visid, attrib );
	if ( ctseq )
	    visserv_->setColTabSequence( displayid_, attrib, *ctseq );

	const ColTab::MapperSetup* ctms =
		visserv_->getColTabMapperSetup( visid, attrib );
	if ( ctms )
	    visserv_->setColTabMapperSetup( displayid_, attrib, *ctms );
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return true;
}


void uiODPlaneDataTreeItem::posChange( CallBacker* )
{
    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != displayid_ )
	return;

    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( pdd && pdd->getOrientation()==OD::SliceType::Z )
	pdd->setSlideActive( slicepos->isSliderActive() );

    movePlaneAndCalcAttribs( slicepos->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::sliderReleasedCB( CallBacker* )
{
    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( !pdd || pdd->getOrientation()!=OD::SliceType::Z )
	return;

    pdd->setSlideActive( false );
}


void uiODPlaneDataTreeItem::selChg( CallBacker* )
{
    ConstRefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( pdd && pdd->isSelected() )
	visserv_->getUiSlicePos()->setDisplay( pdd->id() );
}


uiString uiODPlaneDataTreeItem::createDisplayName() const
{
    uiString res;
    if ( !pdd_ )
	return res;

    ConstRefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    const TrcKeyZSampling tkzs = pdd->getTrcKeyZSampling(true,true);
    const OD::SliceType orientation = pdd->getOrientation();

    if ( orientation==OD::SliceType::Inline )
	res = toUiString( tkzs.hsamp_.start_.inl() );
    else if ( orientation==OD::SliceType::Crossline )
	res = toUiString( tkzs.hsamp_.start_.crl() );
    else
    {
	ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	if ( scene )
	{
	    const ZDomain::Info& datazdom = scene ? scene->zDomainInfo()
						  : SI().zDomainInfo();
	    const ZDomain::Info& displayzdom = datazdom.isDepth()
					     ? ZDomain::DefaultDepth( true )
					     : datazdom;
	    const float zval = tkzs.zsamp_.start_ *
			FlatView::Viewer::userFactor( datazdom, &displayzdom );
	    const int nrzdec = displayzdom.nrDecimals( tkzs.zsamp_.step_ );
	    res = toUiStringDec( zval, nrzdec );
	}
	else
	    res = toUiString( tkzs.zsamp_.start_ );
    }

    return res;
}


static void snapToTkzs( const TrcKeyZSampling& tkzs, TrcKey& tk, float& z )
{
    tk = tkzs.hsamp_.getNearest( tk );
    z = tkzs.zsamp_.snap( z );
}


void uiODPlaneDataTreeItem::createMenu( MenuHandler* mh, bool istb )
{
    uiODDisplayTreeItem::createMenu( mh,istb );
    if ( !mh || !isDisplayID(mh->menuID()) )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuOrTBItem( istb, mh, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
    mAddMenuOrTBItem( istb, mh, &displaymnuitem_, &gridlinesmnuitem_,
		      true, false );

    mDynamicCastGet(uiMenuHandler*,uimh,mh)
    if ( !uimh || uimh->getMenuType()!=uiMenuHandler::fromScene() )
    {
	mResetMenuItem( &addinlitem_ );
	mResetMenuItem( &addcrlitem_ );
	mResetMenuItem( &addzitem_ );
	return;
    }

    if ( !pdd_ )
	return;

    ConstRefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    const Coord3 pickedpos = uimh->getPickedPos();
    TrcKey tk( SI().transform(pickedpos) );
    float zposf = mCast( float, pickedpos.z_ );
    snapToTkzs( pdd->getTrcKeyZSampling(), tk, zposf );
    const int zpos = mNINT32( zposf * SI().zDomain().userFactor() );
    const TrcKey& csttk = const_cast<const TrcKey&>( tk );

    addinlitem_.text = tr("Add In-line %1").arg( csttk.lineNr() );
    addcrlitem_.text = tr("Add Cross-line %1").arg( csttk.trcNr() );
    addzitem_.text = tr("Add Z-slice %1").arg( zpos );

    if ( orient_ == OD::SliceType::Inline )
    {
	mAddMenuItem( mh, &addcrlitem_, true, false );
	mAddMenuItem( mh, &addzitem_, true, false );
    }
    else if ( orient_ == OD::SliceType::Crossline )
    {
	mAddMenuItem( mh, &addinlitem_, true, false );
	mAddMenuItem( mh, &addzitem_, true, false );
    }
    else
    {
	mAddMenuItem( mh, &addinlitem_, true, false );
	mAddMenuItem( mh, &addcrlitem_, true, false );
    }
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    if ( !pdd_ )
	return;

    TrcKeyZSampling scenetkzs = SI().sampling(true);
    ConstRefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
    if ( scene )
	scenetkzs = scene->getTrcKeyZSampling();

    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	delete positiondlg_;
	positiondlg_ = new uiSliceSelDlg( getUiParent(),
				pdd->getTrcKeyZSampling(true,true), scenetkzs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos),
				(uiSliceSel::Type)orient_,scene->zDomainInfo());
	mAttachCB( positiondlg_->windowClosed,
		   uiODPlaneDataTreeItem::posDlgClosed );
	positiondlg_->go();
	mAttachCB( pdd->getMovementNotifier(),
		   uiODPlaneDataTreeItem::updatePositionDlg );
	applMgr()->enableMenusAndToolBars( false );
	applMgr()->visServer()->disabToolBars( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	uiGridLinesDlg gldlg( getUiParent(), pdd.ptr() );
	gldlg.go();
    }

    mDynamicCastGet(uiMenuHandler*,uimh,menu);
    if ( !uimh )
	return;

    if ( mnuid!=addinlitem_.id && mnuid!=addcrlitem_.id && mnuid!=addzitem_.id )
	return;

    MouseCursorChanger mcc( MouseCursor::Wait );
    const OD::SliceType slicetype =
	mnuid==addcrlitem_.id ? OD::SliceType::Crossline
			      : (mnuid==addzitem_.id ? OD::SliceType::Z
						     : OD::SliceType::Inline);
    RefMan<visSurvey::PlaneDataDisplay> newpdd =
		pdd->createTransverseSection( uimh->getPickedPos(), slicetype );
    if ( !newpdd )
	return;

    visserv_->addObject( newpdd.ptr(), sceneID(), true );
    uiODPlaneDataTreeItem* itm = nullptr;
    if ( slicetype == OD::SliceType::Inline )
	itm = new uiODInlineTreeItem( newpdd->id(), Empty );
    else if ( slicetype == OD::SliceType::Crossline )
	itm = new uiODCrosslineTreeItem( newpdd->id(), Empty );
    else // OD::ZSlice
	itm = new uiODZsliceTreeItem( newpdd->id(), Empty );

    parent_->addChild( itm, true );
    visserv_->calculateAllAttribs( newpdd->id() );
    itm->updateColumnText( uiODSceneMgr::cNameColumn() );
    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    ConstRefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( !pdd )
	return;

    const TrcKeyZSampling newcs = pdd->getTrcKeyZSampling( true, true );
    positiondlg_->setTrcKeyZSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( !pdd )
	return;

    TrcKeyZSampling newcs = positiondlg_->getTrcKeyZSampling();
    bool samepos = newcs == pdd->getTrcKeyZSampling();
    if ( positiondlg_->uiResult() && !samepos )
	movePlaneAndCalcAttribs( newcs );

    applMgr()->enableMenusAndToolBars( true );
    applMgr()->enableSceneManipulation( true );
    mDetachCB( pdd->getMovementNotifier(),
	       uiODPlaneDataTreeItem::updatePositionDlg );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(uiSliceSel*,slicesel,cb)
    if ( !slicesel )
	return;

    movePlaneAndCalcAttribs( slicesel->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::movePlaneAndCalcAttribs(
	const TrcKeyZSampling& tkzs )
{
    visserv_->movePlaneAndCalcAttribs( displayid_, tkzs );
}


void uiODPlaneDataTreeItem::keyUnReDoPressedCB( CallBacker* )
{
    if ( !pdd_ || !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();
    RefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( KeyboardEvent::isUnDo(kbe) )
    {
	if ( pdd->isSelected() )
	    pdd->undo().unDo();
    }

    if ( KeyboardEvent::isReDo(kbe) )
    {
	if ( pdd->isSelected() )
	    pdd->undo().reDo();
    }
}


void uiODPlaneDataTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleGet(uiKeyDesc,caps,cb)
    if ( !caps )
	return;

    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const BufferString act( scl.nameOf(caps->data) );
    const bool fwd = act == "Move slice forward";
    const bool bwd = fwd ? false : act == "Move slice backward";
    if ( !fwd && !bwd )
	return;

    const int step = scl.valueOf( caps->data );
    caps->data.setKey( 0 );
    movePlane( fwd, step );
}


void uiODPlaneDataTreeItem::movePlane( bool forward, int step )
{
    ConstRefMan<visSurvey::PlaneDataDisplay> pdd = getDisplay();
    if ( !pdd )
	return;

    TrcKeyZSampling cs = pdd->getTrcKeyZSampling();
    const int dir = forward ? step : -step;

    if ( pdd->getOrientation() == OD::SliceType::Inline )
    {
	cs.hsamp_.start_.inl() += cs.hsamp_.step_.inl() * dir;
	cs.hsamp_.stop_.inl() = cs.hsamp_.start_.inl();
    }
    else if ( pdd->getOrientation() == OD::SliceType::Crossline )
    {
	cs.hsamp_.start_.crl() += cs.hsamp_.step_.crl() * dir;
	cs.hsamp_.stop_.crl() = cs.hsamp_.start_.crl();
    }
    else if ( pdd->getOrientation() == OD::SliceType::Z )
    {
        cs.zsamp_.start_ += cs.zsamp_.step_ * dir;
        cs.zsamp_.stop_ = cs.zsamp_.start_;
    }
    else
	return;

    movePlaneAndCalcAttribs( cs );
}


// In-line items
uiTreeItem* uiODInlineTreeItemFactory::createForVis( const VisID& visid,
						     uiTreeItem* ) const
{
    ConstRefMan<visBase::DataObject> dispobj =
		ODMainWin()->applMgr().visServer()->getObject( visid );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dispobj.ptr() );
    if ( !pdd || pdd->getOrientation() != OD::SliceType::Inline )
	return nullptr;

    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*, rgba,
		     pdd->getChannels2RGBA() );

    return new uiODInlineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


CNotifier<uiODInlineParentTreeItem,uiMenu*>&
	uiODInlineParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODInlineParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sInline() )
{}


uiODInlineParentTreeItem::~uiODInlineParentTreeItem()
{}


const char* uiODInlineParentTreeItem::iconName() const
{
    return "tree-inl";
}


bool uiODInlineParentTreeItem::showSubMenu()
{
    if ( SI().crlRange(true).width()==0 ||
	 SI().zRange(true).width() < SI().zStep()*0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled inline display") );
	return false;
    }

    mParentShowSubMenu( uiODInlineTreeItem, true );
}


uiODInlineTreeItem::uiODInlineTreeItem( const VisID& id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::SliceType::Inline, tp )
{}


uiODInlineTreeItem::~uiODInlineTreeItem()
{}


// Cross-line items
uiTreeItem* uiODCrosslineTreeItemFactory::createForVis( const VisID& visid,
							uiTreeItem* ) const
{
    ConstRefMan<visBase::DataObject> dispobj =
		ODMainWin()->applMgr().visServer()->getObject( visid );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dispobj.ptr() );
    if ( !pdd || pdd->getOrientation() != OD::SliceType::Crossline )
	return nullptr;

    mDynamicCastGet(const visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());

    return new uiODCrosslineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


CNotifier<uiODCrosslineParentTreeItem,uiMenu*>&
	uiODCrosslineParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODCrosslineParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sCrossline() )
{}


uiODCrosslineParentTreeItem::~uiODCrosslineParentTreeItem()
{}


const char* uiODCrosslineParentTreeItem::iconName() const
{
    return "tree-crl";
}


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    if ( SI().inlRange(true).width()==0 ||
	 SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled cross-line display") );
	return false;
    }

    mParentShowSubMenu( uiODCrosslineTreeItem, true );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( const VisID& id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::SliceType::Crossline, tp )
{}


uiODCrosslineTreeItem::~uiODCrosslineTreeItem()
{}


// Z-slice items
uiTreeItem* uiODZsliceTreeItemFactory::createForVis( const VisID& visid,
						     uiTreeItem* ) const
{
    ConstRefMan<visBase::DataObject> dispobj =
		ODMainWin()->applMgr().visServer()->getObject( visid );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,dispobj.ptr() );
    if ( !pdd || pdd->getOrientation() != OD::SliceType::Z )
	return nullptr;

    mDynamicCastGet(const visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());

    return new uiODZsliceTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


CNotifier<uiODZsliceParentTreeItem,uiMenu*>&
	uiODZsliceParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODZsliceParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODParentTreeItem( uiStrings::sZSlice() )
{}


uiODZsliceParentTreeItem::~uiODZsliceParentTreeItem()
{}


const char* uiODZsliceParentTreeItem::iconName() const
{
    return "tree-zsl";
}


bool uiODZsliceParentTreeItem::showSubMenu()
{
     if ( SI().inlRange(true).width()==0 || SI().crlRange(true).width()==0 )
     {
	 uiMSG().warning( tr("Flat survey, disabled z display") );
	 return false;
     }

    mParentShowSubMenu( uiODZsliceTreeItem, false );
}


uiODZsliceTreeItem::uiODZsliceTreeItem( const VisID& id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::SliceType::Z, tp )
{
}


uiODZsliceTreeItem::~uiODZsliceTreeItem()
{}
