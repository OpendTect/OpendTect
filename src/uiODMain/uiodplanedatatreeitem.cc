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
#include "wellmanager.h"
#include "visselman.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, OD::SliceType o,
					      Probe& probe )
    : uiODSceneProbeTreeItem( probe )
    , orient_(o)
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
	visBase::DM().selMan().updateselnotifier.remove(
		mCB(this,uiODPlaneDataTreeItem,posChange) );
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

	visserv_->addObject( pdd, sceneID(), true );

	BufferString res;
	Settings::common().get( "dTect.Texture2D Resolution", res );
	for ( int idx=0; idx<pdd->nrResolutions(); idx++ )
	{
	    if ( res == pdd->getResolutionName(idx) )
		pdd->setResolution( idx, 0 );
	}

	pdd->setProbe( getProbe() );
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return false;

    pdd->ref();
    pdd->selection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->deSelection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->posChanged()->notify(
	    mCB(this,uiODPlaneDataTreeItem,posChange) );

    visserv_->getUiSlicePos()->positionChg.notify(
			mCB(this,uiODPlaneDataTreeItem,posChange) );

    return uiODSceneProbeTreeItem::init();
}


void uiODPlaneDataTreeItem::posChange( CallBacker* cb )
{
    Probe* probe = getProbe();
    if ( !probe )
    { pErrMsg( "Huh! Shared Object not of type Probe" ); return; }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd )
	return;

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,callerpdd,cb);
    if ( callerpdd )
    {
	pdd->annotateNextUpdateStage( true );
	pdd->acceptManipulation();
	pdd->annotateNextUpdateStage( true );
	pdd->resetManipulation();
    }
    else
    {
	uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
	if ( slicepos->getDisplayID() != displayid_ )
	    return;

	pdd->setTrcKeyZSampling( slicepos->getTrcKeyZSampling() );
    }

    probe->setPos( pdd->getTrcKeyZSampling() );
}


void uiODPlaneDataTreeItem::updateDisplay()
{
    const Probe* probe = getProbe();
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    if ( !pdd || !probe )
	return;

    pdd->setTrcKeyZSampling( probe->position() );
    pdd->annotateNextUpdateStage( false );
}


void uiODPlaneDataTreeItem::handleObjChanged( const ChangeData& )
{
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateDisplay();
}


void uiODPlaneDataTreeItem::selChg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    if ( pdd && pdd->isSelected() )
	visserv_->getUiSlicePos()->setDisplay( displayid_ );
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

    const bool isadd = mnuid == addinlitem_.id || mnuid == addcrlitem_.id ||
		       mnuid == addzitem_.id;
    if ( !isadd )
	return;

    TrcKeyZSampling newtkzs( true );
    RefMan<Probe> newprobe = 0;
    if ( mnuid == addinlitem_.id )
    {
	newtkzs.hsamp_.setLineRange( Interval<int>(tk.lineNr(),tk.lineNr()) );
	newprobe = new InlineProbe( newtkzs );
    }
    else if ( mnuid == addcrlitem_.id )
    {
	newtkzs.hsamp_.setTrcRange( Interval<int>(tk.trcNr(),tk.trcNr()) );
	newprobe = new CrosslineProbe( newtkzs );
    }
    else if ( mnuid == addzitem_.id )
    {
	newtkzs.zsamp_.start = newtkzs.zsamp_.stop = zpos;
	newprobe = new ZSliceProbe( newtkzs );
    }

    if ( !uiODSceneProbeParentTreeItem::addDefaultAttribLayer(*applMgr(),
							      *newprobe) ||
	 !ProbeMGR().store(*newprobe).isOK() )
	return;

    OD::ViewerID invalidvwrid;
    ProbePresentationInfo probeprinfo( ProbeMGR().getID(*newprobe) );
    IOPar probeinfopar;
    probeprinfo.fillPar( probeinfopar );
    OD::PrMan().request( invalidvwrid, OD::Add, probeinfopar );
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
    Probe* probe = getProbe();
    if ( !pdd || !probe ) return;

    TrcKeyZSampling newcs = positiondlg_->getTrcKeyZSampling();
    bool samepos = newcs == pdd->getTrcKeyZSampling();
    if ( positiondlg_->uiResult() && !samepos )
	probe->setPos( newcs );

    applMgr()->enableMenusAndToolBars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotifier()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(uiSliceSel*,slicesel,cb)
    Probe* probe = getProbe();
    if ( !slicesel || !probe ) return;

    probe->setPos( slicesel->getTrcKeyZSampling() );
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
    pErrMsg( "Deprecated , to be removed later" );
    return 0;
}


uiODPlaneDataParentTreeItem::uiODPlaneDataParentTreeItem( const uiString& nm )
    : uiODSceneProbeParentTreeItem(nm)
{
}


uiString uiODPlaneDataParentTreeItem::sAddAtWellLocation()
{ return m3Dots(tr("Add at Well Location")); }


void uiODPlaneDataParentTreeItem::addMenuItems()
{
    uiODSceneProbeParentTreeItem::addMenuItems();
    if ( canAddFromWell() )
	menu_->insertItem(
	    new uiAction( uiODPlaneDataParentTreeItem::sAddAtWellLocation()),
			  sAddAtWellLOcationMenuID() );
}



bool uiODPlaneDataParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid==sAddAtWellLOcationMenuID() )
    {
	DBKeySet wellids;
	if ( !applMgr()->wellServer()->selectWells(wellids) )
	    return true;

	for ( int idx=0;idx<wellids.size(); idx++ )
	{
	    ConstRefMan<Well::Data> wd = Well::MGR().fetch( wellids[idx] );
	    if ( !wd ) continue;

	    if ( !setPosToBeAddedFromWell(*wd) )
		continue;

	    typetobeadded_ = uiODSceneProbeParentTreeItem::Default;
	    if ( !addChildProbe() )
		return false;
	}

	return true;
    }

    return uiODSceneProbeParentTreeItem::handleSubMenu( mnuid );
}


bool uiODPlaneDataParentTreeItem::setProbeToBeAddedParams( int mnuid )
{
    if ( mnuid==uiODSceneProbeParentTreeItem::sAddDefaultDataMenuID() ||
	 mnuid==uiODSceneProbeParentTreeItem::sAddAndSelectDataMenuID() ||
	 mnuid==uiODSceneProbeParentTreeItem::sAddColorBlendedMenuID() )
    {
	typetobeadded_ = uiODSceneProbeParentTreeItem::getType( mnuid );
	return setDefaultPosToBeAdded();
    }

    return false;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODPlaneDataParentTreeItem( uiStrings::sInline() )
{}


const char* uiODInlineParentTreeItem::iconName() const
{ return "tree-inl"; }


const char* uiODInlineParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


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


bool uiODInlineParentTreeItem::setDefaultPosToBeAdded()
{
    const Interval<int> inlrg = SI().sampling( true ).hsamp_.lineRange();
    Interval<int> definlrg( inlrg.center(), inlrg.center() );
    probetobeaddedpos_.hsamp_.setLineRange( definlrg );
    OD::PresentationManagedViewer* vwr = OD::PrMan().getViewer( getViewerID() );
    if ( !vwr )
    {
	pErrMsg( "Huh ?? No Scene found" );
	return false;
    }

    if ( !vwr->hasZAxisTransform() )
	return true;

    probetobeaddedpos_.zsamp_ = vwr->getZAxisTransform()->getZInterval( true );
    return true;
}


bool uiODInlineParentTreeItem::setPosToBeAddedFromWell( const Well::Data& wd )
{
    const Coord surfacecoord = wd.info().surfaceCoord();
    const BinID bid = SI().transform( surfacecoord );
    probetobeaddedpos_.hsamp_.setInlRange( Interval<int>(bid.inl(),bid.inl()) );
    return true;
}


Probe* uiODInlineParentTreeItem::createNewProbe() const
{
    return new InlineProbe( probetobeaddedpos_ );
}


uiPresManagedTreeItem* uiODInlineParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(InlineProbe*,inlprobe,probe.ptr())
    if ( !inlprobe )
	return 0;

    uiODInlineTreeItem* inlitem = new uiODInlineTreeItem( *probe );
    addChild( inlitem, false );
    return inlitem;
}


uiODInlineTreeItem::uiODInlineTreeItem( Probe& pr, int id )
    : uiODPlaneDataTreeItem( id, OD::InlineSlice, pr )
{}


uiODInlineAttribTreeItem::uiODInlineAttribTreeItem( const char* parenttype )
    : uiODAttribTreeItem(parenttype)
{
}

uiODDataTreeItem* uiODInlineAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiODInlineTreeItem).name();
    uiODInlineAttribTreeItem* attribtreeitem =
	new uiODInlineAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiODInlineAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	    create, AttribProbeLayer::sFactoryKey(),InlineProbe::sFactoryKey());

}


uiTreeItem*
    uiODCrosslineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    pErrMsg( "Deprecated , to be removed later" );
    return 0;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODPlaneDataParentTreeItem( uiStrings::sCrossline() )
{}


const char* uiODCrosslineParentTreeItem::iconName() const
{ return "tree-crl"; }


const char* uiODCrosslineParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


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


bool uiODCrosslineParentTreeItem::setDefaultPosToBeAdded()
{
    const Interval<int> crlrg = SI().sampling( true ).hsamp_.trcRange();
    Interval<int> defcrlrg( crlrg.center(), crlrg.center() );
    probetobeaddedpos_.hsamp_.setTrcRange( defcrlrg );
    OD::PresentationManagedViewer* vwr = OD::PrMan().getViewer( getViewerID() );
    if ( !vwr )
    {
	pErrMsg( "Huh ?? No Scene found" );
	return false;
    }

    if ( !vwr->hasZAxisTransform() )
	return true;

    probetobeaddedpos_.zsamp_ = vwr->getZAxisTransform()->getZInterval( true );
    return true;
}


bool uiODCrosslineParentTreeItem::setPosToBeAddedFromWell(const Well::Data& wd)
{
    const Coord surfacecoord = wd.info().surfaceCoord();
    const BinID bid = SI().transform( surfacecoord );
    probetobeaddedpos_.hsamp_.setCrlRange( Interval<int>(bid.crl(),bid.crl()) );
    return true;
}



Probe* uiODCrosslineParentTreeItem::createNewProbe() const
{
    return new CrosslineProbe( probetobeaddedpos_ );
}


uiPresManagedTreeItem* uiODCrosslineParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(CrosslineProbe*,crlprobe,probe.ptr())
    if ( !crlprobe )
	return 0;

    uiODCrosslineTreeItem* crlitem = new uiODCrosslineTreeItem( *probe );
    addChild( crlitem, false );
    return crlitem;
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( Probe& pr, int id )
    : uiODPlaneDataTreeItem( id, OD::CrosslineSlice, pr )
{}


uiODCrosslineAttribTreeItem::uiODCrosslineAttribTreeItem( const char* partype )
    : uiODAttribTreeItem(partype)
{
}

uiODDataTreeItem* uiODCrosslineAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiODCrosslineTreeItem).name();
    uiODCrosslineAttribTreeItem* attribtreeitem =
	new uiODCrosslineAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiODCrosslineAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	    create, AttribProbeLayer::sFactoryKey(),
	    CrosslineProbe::sFactoryKey() );

}



uiTreeItem*
    uiODZsliceTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    pErrMsg( "Deprecated , to be removed later" );
    return 0;
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODPlaneDataParentTreeItem( uiStrings::sZSlice() )
{}


const char* uiODZsliceParentTreeItem::iconName() const
{ return "tree-zsl"; }


const char* uiODZsliceParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


bool uiODZsliceParentTreeItem::canShowSubMenu() const
{
     if ( !SI().inlRange(true).width() || !SI().crlRange(true).width() )
     {
	 uiMSG().warning( tr("Flat survey, disabled z display") );
	 return false;
     }

     return true;
}

bool uiODZsliceParentTreeItem::setDefaultPosToBeAdded()
{
    OD::PresentationManagedViewer* vwr = OD::PrMan().getViewer( getViewerID() );
    if ( !vwr )
    {
	pErrMsg( "Huh ?? No Scene found" );
	return false;
    }

    const ZAxisTransform* transform = vwr->getZAxisTransform();
    const StepInterval<float> zrg = transform ? transform->getZInterval(true)
					      : SI().sampling(true).zsamp_;
    StepInterval<float> defzrg( zrg.center(), zrg.center(), zrg.step );
    probetobeaddedpos_.zsamp_ = defzrg;
    return true;
}



Probe* uiODZsliceParentTreeItem::createNewProbe() const
{
    return new ZSliceProbe( probetobeaddedpos_ );
}


uiPresManagedTreeItem* uiODZsliceParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(ZSliceProbe*,zsliceprobe,probe.ptr())
    if ( !zsliceprobe )
	return 0;

    uiODZsliceTreeItem* zslitem = new uiODZsliceTreeItem( *probe );
    addChild( zslitem, false );
    return zslitem;
}


uiODZsliceTreeItem::uiODZsliceTreeItem( Probe& probe, int id )
    : uiODPlaneDataTreeItem( id, OD::ZSlice, probe )
{
}


uiString uiODZsliceTreeItem::createDisplayName() const
{
    const Probe* probe = getProbe();
    if ( !probe )
    {
	pErrMsg( "Probe not found" );
	return uiString::emptyString();
    }

    OD::PresentationManagedViewer* vwr = OD::PrMan().getViewer( getViewerID() );
    if ( !vwr )
    {
	pErrMsg( "Viewer not found" );
	return uiString::emptyString();
    }

    const TrcKeyZSampling probepos = probe->position();
    const ZDomain::Info& scnezdinfo = vwr->zDomain();
    const float zposval = probepos.zsamp_.start * scnezdinfo.userFactor();
    return tr( "%1 (%2)" ).arg( zposval ).arg( scnezdinfo.unitStr() );
}


uiODZsliceAttribTreeItem::uiODZsliceAttribTreeItem( const char* parenttype )
    : uiODAttribTreeItem(parenttype)
{
}

uiODDataTreeItem* uiODZsliceAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiODZsliceTreeItem).name();
    uiODZsliceAttribTreeItem* attribtreeitem =
	new uiODZsliceAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiODZsliceAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	    create, AttribProbeLayer::sFactoryKey(),ZSliceProbe::sFactoryKey());

}
