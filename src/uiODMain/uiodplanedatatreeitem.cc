/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uiodplanedatatreeitem.h"

#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "uiwellpartserv.h"
#include "uimain.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"

#include "attribdescsetsholder.h"
#include "attribprobelayer.h"
#include "coltabsequence.h"
#include "probemanager.h"
#include "probeimpl.h"
#include "settings.h"
#include "welldata.h"
#include "wellinfo.h"
#include "wellman.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, OD::SliceType o,
					      Probe& probe )
    : orient_(o)
    , probe_(probe)
    , positiondlg_(0)
    , positionmnuitem_(m3Dots(tr("Position")),cPositionIdx)
    , gridlinesmnuitem_(m3Dots(tr("Gridlines")),cGridLinesIdx)
    , addinlitem_(tr("Add Inl-line"),10003)
    , addcrlitem_(tr("Add Crl-line"),10002)
    , addzitem_(tr("Add Z-slice"),10001)
{
    probe_.ref();
    //TODO Implement probeChangedCB
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
    probe_.unRef();
    delete positiondlg_;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	RefMan<visSurvey::PlaneDataDisplay> pdd =
	    new visSurvey::PlaneDataDisplay;
	displayid_ = pdd->id();

	visserv_->addObject( pdd, sceneID(), true );

	BufferString res;
	Settings::common().get( "dTect.Texture2D Resolution", res );
	for ( int idx=0; idx<pdd->nrResolutions(); idx++ )
	{
	    if ( res == pdd->getResolutionName(idx) )
		pdd->setResolution( idx, 0 );
	}

	pdd->setProbe( &probe_ );
	for ( int idx=0; idx<probe_.nrLayers(); idx++ )
	{
	    mDynamicCastGet( const AttribProbeLayer*,attrprlayer,
			     probe_.getLayerByIdx(idx) );
	    if ( !attrprlayer )
		continue;

	    if ( attrprlayer->getDispType()==AttribProbeLayer::RGB )
		pdd->setChannels2RGBA(
			visBase::RGBATextureChannel2RGBA::create() );
	    if ( idx )
		pdd->addAttrib();
	    visserv_->setSelSpec( displayid_, idx, attrprlayer->getSelSpec() );
	    visserv_->setColTabMapperSetup( displayid_, idx,
					    attrprlayer->getColTabMapper() );
	    visserv_->setColTabSequence( displayid_, idx,
					 attrprlayer->getColTab());
	    if ( !attrprlayer->getAttribDataPack().isInvalid() )
		visserv_->setDataPackID( displayid_, idx,
					 attrprlayer->getAttribDataPack() );
	    else
		visserv_->calculateAttrib( displayid_, idx, false );
	}
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


OD::ObjPresentationInfo* uiODPlaneDataTreeItem::getObjPRInfo() const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return 0;

    ProbePresentationInfo* prinfo =
	new ProbePresentationInfo( ProbeMGR().getID(probe_) );
    return prinfo;
}


void uiODPlaneDataTreeItem::setAtWellLocation( const Well::Data& wd )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return;

    const Coord surfacecoord = wd.info().surfaceCoord();
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
	    const float zval = cs.zsamp_.start * zdef.userFactor();
	    res = toUiString( zdef.isDepth() || zdef.userFactor()==1000
		    ? (float)(mNINT32(zval)) : zval );
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
    TrcKey tk( SI().transform(pickedpos.getXY()) );
    float zposf = mCast( float, pickedpos.z_ );
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
    TrcKey tk( SI().transform(pickedpos.getXY()) );
    float zpos = mCast( float, pickedpos.z_ );
    snapToTkzs( pdd->getTrcKeyZSampling(), tk, zpos );

    TrcKeyZSampling newtkzs( true );
    mDynamicCastGet(uiODProbeParentTreeItem*,probeparent,parent_)
    if ( !probeparent )
	return;

    Probe* newprobe = probeparent->createNewProbe();
    if ( !newprobe )
	return;

    if ( mnuid == addinlitem_.id )
	newtkzs.hsamp_.setLineRange( Interval<int>(tk.lineNr(),tk.lineNr()) );
    else if ( mnuid == addcrlitem_.id )
	newtkzs.hsamp_.setTrcRange( Interval<int>(tk.trcNr(),tk.trcNr()) );
    else if ( mnuid == addzitem_.id )
	newtkzs.zsamp_.start = newtkzs.zsamp_.stop = zpos;

    *newprobe = probe_;
    newprobe->setPos( newtkzs );
    ProbePresentationInfo probeprinfo( ProbeMGR().getID(*newprobe) );
    uiODPrManagedTreeItem* newitem = probeparent->addChildItem( probeprinfo );
    if ( !newitem )
	return;

    newitem->emitPRRequest( OD::Add );
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
{
    visserv_->movePlaneAndCalcAttribs( displayid_, tkzs );
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return;

    ChangeNotifyBlocker notblocker( probe_ );
    probe_.setPos( tkzs );
    for ( int iattr=0; iattr<pdd->nrAttribs(); iattr++ )
    {
	ProbeLayer* layer = probe_.getLayerByIdx( iattr );
	mDynamicCastGet(AttribProbeLayer*,attriblayer,layer)
	if ( !attriblayer )
	    continue;
	attriblayer->setAttribDataPack( pdd->getDataPackID(iattr) );
    }
}



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


uiTreeItem*
    uiODInlineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::InlineSlice )
	return 0;

    Probe* probe = pdd->getProbe();
    if ( !probe )
	return 0;

    return new uiODInlineTreeItem( visid, *probe );
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODProbeParentTreeItem( uiStrings::sInline() )
{}


const char* uiODInlineParentTreeItem::iconName() const
{ return "tree-inl"; }


bool uiODInlineParentTreeItem::canShowSubMenu() const
{
    if ( !SI().crlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled inline display") );
	return false;
    }

    return true;
}


Probe* uiODInlineParentTreeItem::createNewProbe() const
{
    InlineProbe* newprobe = new InlineProbe();
    if ( !ProbeMGR().store(*newprobe).isOK() )
    {
	delete newprobe;
	return 0;
    }

    TrcKeyZSampling probepos = SI().sampling( true );
    const Interval<int> inlrg = probepos.hsamp_.lineRange();
    Interval<int> definlrg( inlrg.center(), inlrg.center() );
    probepos.hsamp_.setLineRange( definlrg );
    newprobe->setPos( probepos );

    return newprobe;
}


uiODPrManagedTreeItem* uiODInlineParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(InlineProbe*,inlprobe,probe.ptr())
    if ( !inlprobe )
	return 0;

    uiODInlineTreeItem* inlitem = new uiODInlineTreeItem( -1, *probe );
    addChild( inlitem, false );
    return inlitem;
}


uiODInlineTreeItem::uiODInlineTreeItem( int id, Probe& pr )
    : uiODPlaneDataTreeItem( id, OD::InlineSlice, pr )
{}



uiTreeItem*
    uiODCrosslineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::CrosslineSlice )
	return 0;

    Probe* probe = pdd->getProbe();
    if ( !probe )
	return 0;

    return new uiODCrosslineTreeItem( visid, *probe );
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODProbeParentTreeItem( uiStrings::sCrossline() )
{}


const char* uiODCrosslineParentTreeItem::iconName() const
{ return "tree-crl"; }


bool uiODCrosslineParentTreeItem::canShowSubMenu() const
{
    if ( !SI().inlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( tr("Flat survey, disabled cross-line display") );
	return false;
    }

    return true;
}

Probe* uiODCrosslineParentTreeItem::createNewProbe() const
{
    CrosslineProbe* newprobe = new CrosslineProbe();
    if ( !ProbeMGR().store(*newprobe).isOK() )
    {
	delete newprobe;
	return 0;
    }

    TrcKeyZSampling probepos = SI().sampling( true );
    const Interval<int> crlrg = probepos.hsamp_.trcRange();
    Interval<int> defcrlrg( crlrg.center(), crlrg.center() );
    probepos.hsamp_.setTrcRange( defcrlrg );
    newprobe->setPos( probepos );
    return newprobe;
}


uiODPrManagedTreeItem* uiODCrosslineParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(CrosslineProbe*,crlprobe,probe.ptr())
    if ( !crlprobe )
	return 0;

    uiODCrosslineTreeItem* crlitem =
	new uiODCrosslineTreeItem( -1, *probe );
    addChild( crlitem, false );
    return crlitem;
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id, Probe& pr )
    : uiODPlaneDataTreeItem( id, OD::CrosslineSlice, pr )
{}



uiTreeItem*
    uiODZsliceTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
		     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=OD::ZSlice )
	return 0;

    Probe* probe = pdd->getProbe();
    if ( !probe )
	return 0;

    return new uiODZsliceTreeItem( visid, *probe );
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODProbeParentTreeItem( uiStrings::sZSlice() )
{}


const char* uiODZsliceParentTreeItem::iconName() const
{ return "tree-zsl"; }


bool uiODZsliceParentTreeItem::canShowSubMenu() const
{
     if ( !SI().inlRange(true).width() || !SI().crlRange(true).width() )
     {
	 uiMSG().warning( tr("Flat survey, disabled z display") );
	 return false;
     }

     return true;
}


Probe* uiODZsliceParentTreeItem::createNewProbe() const
{
    ZSliceProbe* newprobe = new ZSliceProbe();
    if ( !ProbeMGR().store(*newprobe).isOK() )
    {
	delete newprobe;
	return 0;
    }

    TrcKeyZSampling probepos = SI().sampling( true );
    const StepInterval<float> zrg = probepos.zsamp_;
    StepInterval<float> defzrg( zrg.center(), zrg.center(), zrg.step );
    probepos.zsamp_ = defzrg;
    newprobe->setPos( probepos );

    return newprobe;
}


uiODPrManagedTreeItem* uiODZsliceParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(ZSliceProbe*,zsliceprobe,probe.ptr())
    if ( !zsliceprobe )
	return 0;

    uiODZsliceTreeItem* zslitem = new uiODZsliceTreeItem( -1, *probe );
    addChild( zslitem, false );
    return zslitem;
}


uiODZsliceTreeItem::uiODZsliceTreeItem( int id, Probe& probe )
    : uiODPlaneDataTreeItem( id, OD::ZSlice, probe )
{
}
