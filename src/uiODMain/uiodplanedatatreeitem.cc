/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodplanedatatreeitem.h"

#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uishortcutsmgr.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "uiwellpartserv.h"
#include "uimain.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "coltabsequence.h"
#include "iodir.h"
#include "ioman.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "settings.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellman.h"
#include "zaxistransform.h"
#include "callback.h"
#include "keyboardevent.h"


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
    mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddDefaultData()), 0 ); \
    mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddAndSelectData()), 1 );\
    if ( fromwell ) \
	mnu.insertItem( \
	    new uiAction(uiODPlaneDataTreeItem::sAddAtWellLocation()), 2 ); \
    mnu.insertItem( \
	new uiAction(uiODPlaneDataTreeItem::sAddColorBlended()), 3 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 || mnuid==1 || mnuid==3 ) \
    { \
	treeitm* newitm = new treeitm(-1,getType(mnuid));\
	addChild( newitm, false ); \
    } \
    else if ( mnuid==2 ) \
    { \
	TypeSet<MultiID> wellids; \
	if ( !applMgr()->wellServer()->selectWells(wellids) ) \
	    return true; \
	for ( int idx=0; idx<wellids.size(); idx++ ) \
	{ \
	    Well::Data* wd = Well::MGR().get( wellids[idx] ); \
	    if ( !wd ) continue; \
	    treeitm* itm = new treeitm( -1, uiODPlaneDataTreeItem::Empty ); \
	    setMoreObjectsToDoHint( idx<wellids.size()-1 ); \
	    addChild( itm, false ); \
	    itm->setAtWellLocation( *wd ); \
	    itm->displayDefaultData(); \
	} \
    } \
    handleStandardItems( mnuid ); \
    return true


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, OD::SliceType o, Type t )
    : orient_(o)
    , type_(t)
    , positiondlg_(0)
    , positionmnuitem_(m3Dots(tr("Position")),cPositionIdx)
    , gridlinesmnuitem_(m3Dots(tr("Gridlines")),cGridLinesIdx)
    , addinlitem_(tr("Add Inl-line"),10003)
    , addcrlitem_(tr("Add Crl-line"),10002)
    , addzitem_(tr("Add Z-slice"),10001)
{
    displayid_ = did;
    positionmnuitem_.iconfnm = "orientation64";
    gridlinesmnuitem_.iconfnm = "gridlines";
    mAttachCB( uiMain::keyboardEventHandler().keyPressed,
	uiODPlaneDataTreeItem::keyUnReDoPressedCB );
}


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    detachAllNotifiers();
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( pdd )
    {
	pdd->selection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->deSelection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->unRef();
    }

    visserv_->getUiSlicePos()->positionChg.remove(
			mCB(this,uiODPlaneDataTreeItem,posChange) );

    visserv_->getUiSlicePos()->setDisplay( -1 );
    delete positiondlg_;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	RefMan<visSurvey::PlaneDataDisplay> pdd =
	    new visSurvey::PlaneDataDisplay;
	displayid_ = pdd->id();
	if ( type_ == RGBA )
	{
	    pdd->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    pdd->addAttrib();
	    pdd->addAttrib();
	    pdd->addAttrib();
	}

	pdd->setOrientation( orient_);
	visserv_->addObject( pdd, sceneID(), true );

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
	    selectRGBA( mUdfGeomID );
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return false;

    pdd->ref();
    pdd->selection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->deSelection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );

    visserv_->getUiSlicePos()->positionChg.notify(
			mCB(this,uiODPlaneDataTreeItem,posChange) );

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::setAtWellLocation( const Well::Data& wd )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return;

    const Coord surfacecoord = wd.info().surfacecoord;
    const BinID bid = SI().transform( surfacecoord );
    TrcKeyZSampling cs = pdd->getTrcKeyZSampling( true, true );
    if ( orient_ == OD::InlineSlice )
	cs.hsamp_.setInlRange( Interval<int>(bid.inl(),bid.inl()) );
    else
	cs.hsamp_.setCrlRange( Interval<int>(bid.crl(),bid.crl()) );

    pdd->setTrcKeyZSampling( cs );
    select();
}


void uiODPlaneDataTreeItem::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return;

    pdd->setTrcKeyZSampling( tkzs );
}


bool uiODPlaneDataTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid) )
	return false;

    return displayDataFromDesc( descid, true );
}


bool uiODPlaneDataTreeItem::displayGuidance()
{
    if ( !applMgr() || !applMgr()->attrServer() )
	return false; //safety

    Attrib::SelSpec* as = const_cast<Attrib::SelSpec*>(
					visserv_->getSelSpec( displayid_, 0 ));
    if ( !as ) return false;

    const Pos::GeomID geomid = visserv_->getGeomID( displayid_ );
    const ZDomain::Info* zdinf =
		    visserv_->zDomainInfo( visserv_->getSceneID(displayid_) );
    const bool issi = !zdinf || zdinf->def_.isSI();
    const bool selok = applMgr()->attrServer()->selectAttrib(
			*as, issi ? 0 : zdinf, geomid, tr("first layer" ) );
    if ( selok )
    {
	if ( as->isNLA()
	     || as->id().asInt()==Attrib::SelSpec::cOtherAttrib().asInt() )
	{
	    if ( as->isNLA() )
		visserv_->setSelSpec( displayid_, 0, *as );
	    else
		visserv_->setSelSpecs( displayid_, 0,
				 applMgr()->attrServer()->getTargetSelSpecs() );

	    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			    visserv_->getObject(displayid_))
	    if ( !pdd ) return false;

	    return displayDataFromOther( pdd->id() );

	}
	else
	    return displayDataFromDesc( as->id(), as->isStored() );
    }
    else
	return false;
}


bool uiODPlaneDataTreeItem::displayDataFromDesc( const Attrib::DescID& descid,
						 bool isstored )
{
    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, isstored );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );
    visserv_->setSelSpec( displayid_, 0, as );
    const bool res = visserv_->calculateAttrib( displayid_, 0, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return res;
}


bool uiODPlaneDataTreeItem::displayDataFromDataPack( DataPack::ID dpid,
					 const Attrib::SelSpec& as,
					 const FlatView::DataDispPars::VD& ddp )
{
    visserv_->setSelSpec( displayid_, 0, as );
    visserv_->setColTabMapperSetup( displayid_, 0, ddp.mappersetup_ );
    visserv_->setColTabSequence( displayid_, 0, ColTab::Sequence(ddp.ctab_) );
    visserv_->setDataPackID( displayid_, 0, dpid );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    return true;
}


bool uiODPlaneDataTreeItem::displayDataFromOther( int visid )
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

    movePlaneAndCalcAttribs( slicepos->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::selChg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    if ( pdd && pdd->isSelected() )
	visserv_->getUiSlicePos()->setDisplay( displayid_ );
}


uiString uiODPlaneDataTreeItem::createDisplayName() const
{
    uiString res;
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd )
	return res;

    const TrcKeyZSampling cs = pdd->getTrcKeyZSampling(true,true);
    const OD::SliceType orientation = pdd->getOrientation();

    if ( orientation==OD::InlineSlice )
	res = toUiString(cs.hsamp_.start_.inl());
    else if ( orientation==OD::CrosslineSlice )
	res = toUiString(cs.hsamp_.start_.crl());
    else
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( !scene )
	    res = toUiString(cs.zsamp_.start);
	else
	{
	    const ZDomain::Def& zdef = scene->zDomainInfo().def_;
	    const int nrdec =
		Math::NrSignificantDecimals( cs.zsamp_.step*zdef.userFactor() );
	    const float zval = cs.zsamp_.start * zdef.userFactor();
	    res = toUiString( zval, nrdec );
	}
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
    if ( !mh || mh->menuID() != displayID() )
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

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd ) return;

    const Coord3 pickedpos = uimh->getPickedPos();
    TrcKey tk( SI().transform(pickedpos) );
    float zposf = mCast( float, pickedpos.z );
    snapToTkzs( pdd->getTrcKeyZSampling(), tk, zposf );
    const int zpos = mNINT32( zposf * SI().zDomain().userFactor() );

    addinlitem_.text = tr("Add In-line %1").arg( tk.lineNr() );
    addcrlitem_.text = tr("Add Cross-line %1").arg( tk.trcNr() );
    addzitem_.text = tr("Add Z-slice %1").arg( zpos );

    if ( orient_ == OD::InlineSlice )
    {
	mAddMenuItem( mh, &addcrlitem_, true, false );
	mAddMenuItem( mh, &addzitem_, true, false );
    }
    else if ( orient_ == OD::CrosslineSlice )
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
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd ) return;

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	delete positiondlg_;
	TrcKeyZSampling maxcs = SI().sampling(true);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene && scene->getZAxisTransform() )
	    maxcs = scene->getTrcKeyZSampling();

	positiondlg_ = new uiSliceSelDlg( getUiParent(),
				pdd->getTrcKeyZSampling(true,true), maxcs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos),
				(uiSliceSel::Type)orient_,scene->zDomainInfo());
	positiondlg_->windowClosed.notify(
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg_->go();
	pdd->getMovementNotifier()->notify(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
	applMgr()->enableMenusAndToolBars( false );
	applMgr()->visServer()->disabToolBars( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	uiGridLinesDlg gldlg( getUiParent(), pdd );
	gldlg.go();
    }

    mDynamicCastGet(uiMenuHandler*,uimh,menu);
    if ( !uimh ) return;

    const Coord3 pickedpos = uimh->getPickedPos();
    TrcKey tk( SI().transform(pickedpos) );
    float zpos = mCast( float, pickedpos.z );
    snapToTkzs( pdd->getTrcKeyZSampling(), tk, zpos );

    TrcKeyZSampling newtkzs( true );
    uiODPlaneDataTreeItem* itm = 0;
    if ( mnuid == addinlitem_.id )
    {
	itm = new uiODInlineTreeItem( -1, Empty );
	newtkzs.hsamp_.setLineRange( Interval<int>(tk.lineNr(),tk.lineNr()) );
    }
    else if ( mnuid == addcrlitem_.id )
    {
	itm = new uiODCrosslineTreeItem( -1, Empty );
	newtkzs.hsamp_.setTrcRange( Interval<int>(tk.trcNr(),tk.trcNr()) );
    }
    else if ( mnuid == addzitem_.id )
    {
	itm = new uiODZsliceTreeItem( -1, Empty );
	newtkzs.zsamp_.start = newtkzs.zsamp_.stop = zpos;
    }

    if ( itm )
    {
	parent_->addChild( itm, true );
	itm->setTrcKeyZSampling( newtkzs );
	itm->displayDataFromOther( pdd->id() );
    }
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd ) return;

    const TrcKeyZSampling newcs = pdd->getTrcKeyZSampling( true, true );
    positiondlg_->setTrcKeyZSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd ) return;

    TrcKeyZSampling newcs = positiondlg_->getTrcKeyZSampling();
    bool samepos = newcs == pdd->getTrcKeyZSampling();
    if ( positiondlg_->uiResult() && !samepos )
	movePlaneAndCalcAttribs( newcs );

    applMgr()->enableMenusAndToolBars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotifier()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(uiSliceSel*,slicesel,cb)
    if ( !slicesel ) return;

    movePlaneAndCalcAttribs( slicesel->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::movePlaneAndCalcAttribs(
	const TrcKeyZSampling& tkzs )
{ visserv_->movePlaneAndCalcAttribs( displayid_, tkzs ); }



void uiODPlaneDataTreeItem::keyUnReDoPressedCB( CallBacker* )
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*,pdd,
	visserv_->getObject(displayid_) )
	if ( !pdd )
	    return;

    if ( !uiMain::keyboardEventHandler().hasEvent() )
	return;

    const KeyboardEvent& kbe = uiMain::keyboardEventHandler().event();


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
    if ( !caps ) return;

    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    const BufferString act( scl.nameOf(caps->data) );
    const bool fwd = act == "Move slice forward";
    const bool bwd = fwd ? false : act == "Move slice backward";
    if ( !fwd && !bwd ) return;

    const int step = scl.valueOf( caps->data );
    caps->data.setKey( 0 );
    movePlane( fwd, step );
}


void uiODPlaneDataTreeItem::movePlane( bool forward, int step )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd ) return;

    TrcKeyZSampling cs = pdd->getTrcKeyZSampling();
    const int dir = forward ? step : -step;

    if ( pdd->getOrientation() == OD::InlineSlice )
    {
	cs.hsamp_.start_.inl() += cs.hsamp_.step_.inl() * dir;
	cs.hsamp_.stop_.inl() = cs.hsamp_.start_.inl();
    }
    else if ( pdd->getOrientation() == OD::CrosslineSlice )
    {
	cs.hsamp_.start_.crl() += cs.hsamp_.step_.crl() * dir;
	cs.hsamp_.stop_.crl() = cs.hsamp_.start_.crl();
    }
    else if ( pdd->getOrientation() == OD::ZSlice )
    {
	cs.zsamp_.start += cs.zsamp_.step * dir;
	cs.zsamp_.stop = cs.zsamp_.start;
    }
    else
	return;

    movePlaneAndCalcAttribs( cs );
}


// In-line items
uiTreeItem*
    uiODInlineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::InlineSlice )
	return 0;

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
		     pdd->getChannels2RGBA() );

    return new uiODInlineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sInline() )
{}


uiODInlineParentTreeItem::~uiODInlineParentTreeItem()
{}


const char* uiODInlineParentTreeItem::iconName() const
{ return "tree-inl"; }


bool uiODInlineParentTreeItem::showSubMenu()
{
    if ( !SI().crlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled inline display") );
	return false;
    }

    mParentShowSubMenu( uiODInlineTreeItem, true );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::InlineSlice, tp )
{}


uiODInlineTreeItem::~uiODInlineTreeItem()
{}


// Cross-line items
uiTreeItem*
    uiODCrosslineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::CrosslineSlice )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());
    return new uiODCrosslineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sCrossline() )
{}


uiODCrosslineParentTreeItem::~uiODCrosslineParentTreeItem()
{}


const char* uiODCrosslineParentTreeItem::iconName() const
{ return "tree-crl"; }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    if ( !SI().inlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled cross-line display") );
	return false;
    }

    mParentShowSubMenu( uiODCrosslineTreeItem, true );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::CrosslineSlice, tp )
{}


uiODCrosslineTreeItem::~uiODCrosslineTreeItem()
{}


// Z-slice items
uiTreeItem*
    uiODZsliceTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::ZSlice )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    pdd->getChannels2RGBA());

    return new uiODZsliceTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODParentTreeItem( uiStrings::sZSlice() )
{}


uiODZsliceParentTreeItem::~uiODZsliceParentTreeItem()
{}


const char* uiODZsliceParentTreeItem::iconName() const
{ return "tree-zsl"; }


bool uiODZsliceParentTreeItem::showSubMenu()
{
     if ( !SI().inlRange(true).width() || !SI().crlRange(true).width() )
     {
	 uiMSG().warning( tr("Flat survey, disabled z display") );
	 return false;
     }

    mParentShowSubMenu( uiODZsliceTreeItem, false );
}


uiODZsliceTreeItem::uiODZsliceTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, OD::ZSlice, tp )
{
}


uiODZsliceTreeItem::~uiODZsliceTreeItem()
{}
