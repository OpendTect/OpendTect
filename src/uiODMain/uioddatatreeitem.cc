/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jul 2003
___________________________________________________________________

-*/

#include "uioddatatreeitem.h"

#include "uicolseqdisp.h"
#include "uicoltabsel.h"
#include "uifkspectrum.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uiseisamplspectrum.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribsel.h"
#include "attribprobelayer.h"
#include "probemanager.h"

//TODO:remove when Flattened scene ok for 2D Viewer
#include "emhorizonztransform.h"
#include "vissurvscene.h"


mImplClassFactory( uiODDataTreeItem, factory )

void uiODDataTreeItemFactory::addCreateFunc( CreateFunc crfn,
					const char* probelayertype,
					const char* probetype )
{
    const int probeidx = probetypes_.indexOf( probetype );
    if ( probeidx>=0 )
    {
	if ( !createfuncsset_.validIdx(probeidx) ||
	     !probelayertypesset_.validIdx(probeidx) )
	{ pErrMsg( "Probe Function Pointer Set not found" ); return; }

	TypeSet<CreateFunc>& probecrfuncs = createfuncsset_[probeidx];
	BufferStringSet& probelayertypes = probelayertypesset_[probeidx];
	const int prblayidx = probelayertypes.indexOf( probelayertype );
	if ( prblayidx>=0 )
	{
	    if ( !probecrfuncs.validIdx(prblayidx) )
	    { pErrMsg( "Probe Function Pointer not found" ); return; }

	    probecrfuncs[prblayidx] = crfn;
	    return;
	}

	probelayertypes.add( probelayertype );
	probecrfuncs += crfn;
	return;
    }

    TypeSet<CreateFunc> probefuncset;
    probefuncset += crfn;
    probetypes_.add( probetype );
    createfuncsset_ += probefuncset;
    BufferStringSet probelayertypes;
    probelayertypes.add( probelayertype );
    probelayertypesset_ += probelayertypes;
}


uiODDataTreeItem* uiODDataTreeItemFactory::create( ProbeLayer& probelayer )
{
    const Probe* parentprobe = probelayer.getProbe();
    if ( !parentprobe )
    { pErrMsg( "Parent Probe not set for ProbeLayer" ); return 0; }

    const int probeidx = probetypes_.indexOf( parentprobe->type() );
    if ( probeidx<0 )
	return 0;

    if ( !probelayertypesset_.validIdx(probeidx) ||
	 !createfuncsset_.validIdx(probeidx) )
	return 0;

    BufferStringSet& probelaytypes = probelayertypesset_[probeidx];
    TypeSet<CreateFunc>& probelaycrfuncs = createfuncsset_[probeidx];
    const int probelayidx = probelaytypes.indexOf( probelayer.layerType() );
    if ( probelayidx<0 )
	return 0;

    return (*probelaycrfuncs[probelayidx])( probelayer );
}


uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiODSceneTreeItem(uiString::empty())
    , parenttype_(parenttype)
    , visserv_(ODMainWin()->applMgr().visServer())
    , menu_(0)
    , ampspectrumwin_(0)
    , fkspectrumwin_(0)
    , movemnuitem_(uiStrings::sMove())
    , movetotopmnuitem_(tr("To Top"))
    , movetobottommnuitem_(tr("To Bottom"))
    , moveupmnuitem_(uiStrings::sUp())
    , movedownmnuitem_(uiStrings::sDown())
    , displaymnuitem_(uiStrings::sDisplay())
    , removemnuitem_(uiStrings::sRemove(),-1000)
    , changetransparencyitem_(m3Dots(tr("Change Transparency")))
    , statisticsitem_(m3Dots(uiStrings::sHistogram()))
    , amplspectrumitem_(m3Dots(tr("Amplitude Spectrum")))
    , fkspectrumitem_(m3Dots(tr("F-K Spectrum")))
    , view2dwvaitem_(tr("2D Viewer - Wiggle"))
    , view2dvditem_(tr("2D Viewer"))
    , coltabsel_(uiCOLTAB())
{
    statisticsitem_.iconfnm = "histogram";
    removemnuitem_.iconfnm = "remove";
    view2dwvaitem_.iconfnm = "wva";
    view2dvditem_.iconfnm = "vd";
    amplspectrumitem_.iconfnm = "amplspectrum";

    movetotopmnuitem_.iconfnm = "totop";
    moveupmnuitem_.iconfnm = "uparrow";
    movedownmnuitem_.iconfnm = "downarrow";
    movetobottommnuitem_.iconfnm = "tobottom";
    probelayer_ = new AttribProbeLayer;
    mAttachCB( probelayer_->objectChanged(),
	uiODDataTreeItem::probeLayerChangedCB );
    coltabsel_.seqChanged.notify( mCB(this,uiODDataTreeItem,colSeqChgCB) );
}


uiODDataTreeItem::~uiODDataTreeItem()
{
    detachAllNotifiers();

    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    delete ampspectrumwin_;

    MenuHandler* tb = visserv_->getToolBarHandler();
    tb->createnotifier.remove( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
    coltabsel_.seqChanged.remove( mCB(this,uiODDataTreeItem,colSeqChgCB) );
}


uiODDataTreeItemFactory& uiODDataTreeItem::fac()
{
    mDefineStaticLocalObject(uiODDataTreeItemFactory,datatreeitmfac_,);
    return datatreeitmfac_;
}


int uiODDataTreeItem::uiTreeViewItemType() const
{
    if ( visserv_->canHaveMultipleAttribs(displayID()) ||
	 visserv_->hasSingleColorFallback(displayID()) )
    {
	return uiTreeViewItem::CheckBox;
    }
    else
	return uiTreeItem::uiTreeViewItemType();
}


bool uiODDataTreeItem::init()
{
    if ( visserv_->canHaveMultipleAttribs(displayID()) ||
	 visserv_->hasSingleColorFallback(displayID()) )
    {
	getItem()->stateChanged.notify( mCB(this,uiODDataTreeItem,checkCB) );
	if ( uitreeviewitem_ )
	    uitreeviewitem_->setChecked( visserv_->isAttribEnabled(displayID(),
				     attribNr() ) );
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    tb->createnotifier.notify( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    visserv_->enableAttrib( displayID(), attribNr(), isChecked() );
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    return selid!=-1 && selid==displayID() &&
	   visserv_->getSelAttribNr()==attribNr();
}


int uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent_ );
    return odti ? odti->displayID() : -1;
}


int uiODDataTreeItem::attribNr() const
{
    const int nrattribs = visserv_->getNrAttribs( displayID() );
    const int attribnr = nrattribs-siblingIndex()-1;
    return attribnr<0 || attribnr>=nrattribs ? 0 : attribnr;
}


void uiODDataTreeItem::addToToolBarCB( CallBacker* cb )
{
    coltabsel_.asParent()->display( false );
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    createMenu( tb, true );
    bool enab = !visserv_->isLocked(displayID()) &&
			visserv_->canRemoveDisplay(displayID());
    mAddMenuItem( tb, &removemnuitem_, enab, false );
}


bool uiODDataTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );
    }

    return menu_->executeMenu( uiMenuHandler::fromTree() );
}


void uiODDataTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    createMenu( menu, false );
}


void uiODDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    const bool isfirst = !siblingIndex();
    const bool islast = siblingIndex()==visserv_->getNrAttribs( displayID())-1;

    const bool islocked = visserv_->isLocked( displayID() );

    if ( !islocked && (!isfirst || !islast) )
    {
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movetotopmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &moveupmnuitem_,
		      !islocked && !isfirst, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movedownmnuitem_,
		      !islocked && !islast, false );
	mAddMenuOrTBItem( istb, 0, &movemnuitem_, &movetobottommnuitem_,
		      !islocked && !islast, false );

	mAddMenuOrTBItem( istb, 0, menu, &movemnuitem_, true, false );
    }
    else
    {
	mResetMenuItem( &movetotopmnuitem_ );
	mResetMenuItem( &moveupmnuitem_ );
	mResetMenuItem( &movedownmnuitem_ );
	mResetMenuItem( &movetobottommnuitem_ );

	mResetMenuItem( &movemnuitem_ );
    }

    mAddMenuOrTBItem( istb, 0, menu, &displaymnuitem_, true, false );
    const DataPack::ID dpid =
	visserv_->getDisplayedDataPackID( displayID(), attribNr() );
    const bool hasdatapack = dpid.isValid();
    const bool isvert = visserv_->isVerticalDisp( displayID() );
    coltabsel_.asParent()->display( hasdatapack );
    if ( hasdatapack )
	mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
			  &statisticsitem_, true, false)
    else
	mResetMenuItem( &statisticsitem_ )

    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
    const bool hastransform = scene && scene->getZAxisTransform();
    if ( hasdatapack && isvert )
    {
	mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &amplspectrumitem_,
			  !hastransform, false )
	mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &fkspectrumitem_,
			  !hastransform, false )
    }
    else
    {
	mResetMenuItem( &amplspectrumitem_ )
	mResetMenuItem( &fkspectrumitem_ )
    }

    const bool enab = !islocked && visserv_->canRemoveDisplay( displayID());
    mAddMenuOrTBItem( istb, 0, menu, &removemnuitem_, enab, false );

    if ( visserv_->canHaveMultipleAttribs(displayID()) && hasTransparencyMenu())
	mAddMenuOrTBItem( istb, 0, &displaymnuitem_,
			  &changetransparencyitem_, true, false )
    else
	mResetMenuItem( &changetransparencyitem_ );

    if ( visserv_->canBDispOn2DViewer(displayID()) && hasdatapack )
    {
	const Attrib::SelSpec* as =
	    visserv_->getSelSpec( displayID(), attribNr() );
	const bool hasattrib =
	    as && as->id() != Attrib::SelSpec::cAttribNotSelID();

	mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &view2dvditem_,
			  hasattrib, false )
	const bool withwva = true;
	if ( withwva )
	{
	    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &view2dwvaitem_,
			      hasattrib, false )
	}
	else
	{
	    mResetMenuItem( &view2dwvaitem_ );
	}
    }
    else
    {
	mResetMenuItem( &view2dwvaitem_ );
	mResetMenuItem( &view2dvditem_ );
    }
}


bool uiODDataTreeItem::select()
{
    visserv_->setSelObjectId( displayID(), attribNr() );
    return true;
}


void uiODDataTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==movetotopmnuitem_.id )
    {
	const int nrattribs = visserv_->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx<nrattribs-1; idx++ )
	    visserv_->swapAttribs( displayID(), idx, idx+1 );

	moveItemToTop();
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==movetobottommnuitem_.id )
    {
	for ( int idx=attribNr(); idx; idx-- )
	    visserv_->swapAttribs( displayID(), idx, idx-1 );

	moveItem( parent_->lastChild() );
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==moveupmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr<visserv_->getNrAttribs( displayID() )-1 )
	{
	    const int targetattribnr = attribnr+1;
	    visserv_->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingAbove() );
	select();
	menu->setIsHandled(true);
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==movedownmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr )
	{
	    const int targetattribnr = attribnr-1;
	    visserv_->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingBelow() );
	select();
	menu->setIsHandled( true );
	applMgr()->updateColorTable( displayID(), attribNr() );
    }
    else if ( mnuid==changetransparencyitem_.id )
    {
	visserv_->showAttribTransparencyDlg( displayID(), attribNr() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==statisticsitem_.id || mnuid==amplspectrumitem_.id
	      || mnuid==fkspectrumitem_.id )
    {
	const int visid = displayID();
	const int attribid = attribNr();
	DataPack::ID dpid = visserv_->getDataPackID( visid, attribid );
	const DataPackMgr::ID dmid = visserv_->getDataPackMgrID( visid );
	const int version = visserv_->selectedTexture( visid, attribid );
	const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attribid );
	const FixedString dpname = DPM(dmid).nameOf( dpid );
	if ( as && dpname != as->userRef() )
	{
	    TypeSet<DataPack::ID> ids;
	    DPM(dmid).getPackIDs( ids );
	    for ( int idx=0; idx<ids.size(); idx++ )
	    {
		auto pack = DPM(dmid).getDP( ids[idx] );
		if ( !pack && pack->hasName(as->userRef()) )
		    { dpid = pack->id(); break; }
	    }
	}
	if ( mnuid==statisticsitem_.id )
	{
	    visserv_->displayMapperRangeEditForAttribs( visid, attribid );
	    menu->setIsHandled( true );
	}
	else if ( mnuid==amplspectrumitem_.id || mnuid==fkspectrumitem_.id )
	{
	    const bool isselmodeon = visserv_->isSelectionModeOn();
	    if ( !isselmodeon )
	    {
		if ( mnuid==amplspectrumitem_.id )
		{
		    delete ampspectrumwin_;
		    ampspectrumwin_ = new uiSeisAmplSpectrum(
				      applMgr()->applService().parent() );
		    ampspectrumwin_->setDataPackID( dpid, dmid, version );
		    ampspectrumwin_->show();
		}
		else
		{
		    fkspectrumwin_ =
			new uiFKSpectrum( applMgr()->applService().parent() );
		    fkspectrumwin_->setDataPackID( dpid, dmid, version );
		    fkspectrumwin_->show();
		}
	    }

	    menu->setIsHandled( true );
	}
    }
    else if ( mnuid==view2dwvaitem_.id || mnuid==view2dvditem_.id )
    {
	mDynamicCastGet(uiPresManagedTreeItem*,prmanitem,parent_);
	if ( !prmanitem )
	    return;

	PtrMan<Presentation::ObjInfo> prinfo = prmanitem->getObjPrInfo();
	if ( !prinfo )
	    return;

	RefMan<Probe> probe = ProbeMGR().fetchForEdit( prinfo->storedID() );
	if ( !probe )
	    return;

	ODMainWin()->viewer2DMgr().displayIn2DViewer( *probe,
						      probelayer_->getID() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr();
	prepareForShutdown();
	applMgr()->updateColorTable( displayID(), attribnr ? attribnr-1 : 0 );

	parent_->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    applMgr()->hideColorTable();
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


void uiODDataTreeItem::displayMiniCtab( const ColTab::Sequence* seq )
{
    setPixmap( uiODSceneMgr::cColorColumn(), seq );
}


void uiODDataTreeItem::setProbeLayer( ProbeLayer* newlayer )
{
    if ( probelayer_ == newlayer )
	return;

    if ( probelayer_ )
    {
	const Probe* prb = probelayer_->getProbe();
	if ( prb )
	    mDetachCB( prb->objectChanged(), uiODDataTreeItem::probeChangedCB );
    }

    replaceMonitoredRef( probelayer_, newlayer, this );

    if ( newlayer )
    {
	const Probe* prb = newlayer->getProbe();
	if ( !prb )
	    { pErrMsg("new ProbelLayer has no Probe"); }
	else
	    mAttachCB( prb->objectChanged(), uiODDataTreeItem::probeChangedCB );
    }

    updateDisplay();
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODDataTreeItem::probeLayerChangedCB( CallBacker* cb )
{
    updateDisplay();
    mCBCapsuleUnpack(Monitorable::ChangeData,cd,cb);
    if ( cd.isEntireObject()
      || cd.changeType()==AttribProbeLayer::cColSeqChange() )
	updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODDataTreeItem::probeChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpack(Monitorable::ChangeData,cd,cb);
    if ( cd.changeType()==Probe::cPositionChange() ||
	 cd.changeType()==Probe::cDimensionChange() )
	updateDisplay();
}


void uiODDataTreeItem::colSeqChgCB( CallBacker* cb )
{
    if ( isSelected() )
	colSeqChg( coltabsel_.sequence() );
}
