/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"

#include "uifkspectrum.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uishortcutsmgr.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uiseisamplspectrum.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribsel.h"

//TODO:remove when Flattened scene ok for 2D Viewer
#include "emhorizonztransform.h"
#include "vissurvscene.h"


mImplFactory2Param( uiODDataTreeItem, const Attrib::SelSpec&,
		     const char*, uiODDataTreeItem::factory )

uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiTreeItem(uiString::emptyString())
    , parenttype_(parenttype)
    , visserv_(ODMainWin()->applMgr().visServer())
    , menu_(0)
    , statswin_(0)
    , ampspectrumwin_(0)
    , fkspectrumwin_(0)
    , movemnuitem_(tr("Move"))
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
}


uiODDataTreeItem::~uiODDataTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    delete statswin_;
    delete ampspectrumwin_;

    MenuHandler* tb = visserv_->getToolBarHandler();
    tb->createnotifier.remove( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
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


uiODApplMgr* uiODDataTreeItem::applMgr() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr(), res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


bool uiODDataTreeItem::init()
{
    if ( visserv_->canHaveMultipleAttribs(displayID()) ||
	 visserv_->hasSingleColorFallback(displayID()) )
    {
	getItem()->stateChanged.notify( mCB(this,uiODDataTreeItem,checkCB) );
	uitreeviewitem_->setChecked( visserv_->isAttribEnabled(displayID(),
				     attribNr() ) );
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    tb->createnotifier.notify( mCB(this,uiODDataTreeItem,addToToolBarCB) );
    tb->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );

    if ( keyPressed() )
	keyPressed()->notify( mCB(this,uiODDataTreeItem,keyPressCB) );

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    visserv_->enableAttrib( displayID(), attribNr(), isChecked() );
}


void uiODDataTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleUnpack(uiKeyDesc,kd,cb);

    if ( kd.key()==OD::KB_PageUp || kd.key()==OD::KB_PageDown )
    {
	applMgr()->pageUpDownPressed( kd.key()==OD::KB_PageUp );
	if ( cbcaps )
	    cbcaps->data.setKey( 0 );
    }
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    const VisID visid( selid );
    return visid.isValid() && visid==displayID() &&
	   visserv_->getSelAttribNr()==attribNr();
}


SceneID uiODDataTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey(), sceneid );
    return SceneID(sceneid);
}


VisID uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent_ );
    return odti ? odti->displayID() : VisID::udf();
}


int uiODDataTreeItem::attribNr() const
{
    const int nrattribs = visserv_->getNrAttribs( displayID() );
    const int attribnr = nrattribs-siblingIndex()-1;
    return attribnr<0 || attribnr>=nrattribs ? 0 : attribnr;
}


void uiODDataTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID().asInt() || !isSelected() )
	return;

    createMenu( tb, true );
    const bool enab = !visserv_->isLocked(displayID()) &&
			visserv_->canRemoveAttrib(displayID());
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
    const DataPackID dpid =
	visserv_->getDisplayedDataPackID( displayID(), attribNr() );
    const bool hasdatapack = dpid.isValid() && dpid!=DataPack::cNoID();
    const bool isvert = visserv_->isVerticalDisp( displayID() );
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

    const bool enab = !islocked && visserv_->canRemoveAttrib( displayID());
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
	    as && as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();

	mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &view2dvditem_,
			  hasattrib, false )
	const bool withwva = isvert;
	if ( withwva )
	{
	    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &view2dwvaitem_,
			      hasattrib, false )
	}
	else
	{
	    mResetMenuItem( &view2dwvaitem_ )
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
	const VisID visid = displayID();
	const int attribid = attribNr();
	DataPackID dpid = visserv_->getDataPackID( visid, attribid );
	const DataPackMgr::MgrID dmid = visserv_->getDataPackMgrID( visid );
	const int version = visserv_->selectedTexture( visid, attribid );
	const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attribid );
	const StringView dpname = DPM(dmid).nameOf( dpid );
	if ( as && dpname != as->userRef() )
	{
	    TypeSet<DataPackMgr::PackID> packids;
	    DPM(dmid).getPackIDs( packids );
	    for ( const auto& packid : packids )
	    {
		const StringView tmpnm = DPM(dmid).nameOf( packid );
		if ( tmpnm == as->userRef() )
		{
		    dpid = packid;
		    break;
		}
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
	ODMainWin()->viewer2DMgr().displayIn2DViewer( displayID(), attribNr(),
						   mnuid==view2dwvaitem_.id );
	menu->setIsHandled( true );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr();
	prepareForShutdown();
	visserv_->removeAttrib( displayID(), attribnr );
	applMgr()->updateColorTable( displayID(), attribnr ? attribnr-1 : 0 );

	parent_->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::prepareForShutdown()
{
    uiTreeItem::prepareForShutdown();
    applMgr()->updateColorTable( VisID::udf(), -1 );
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


void uiODDataTreeItem::displayMiniCtab( const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODSceneMgr::cColorColumn() );
	return;
    }

    uitreeviewitem_->setPixmap( uiODSceneMgr::cColorColumn(), *seq );
}
